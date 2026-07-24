/* =============================== File I/O ================================= */

#include "def.h"

/* Refresh the on-disk metadata snapshot for the active buffer.  Called after
 * a successful open or save so the auto-revert poll has a baseline to
 * compare against.  If the file is not present (e.g. a freshly created
 * buffer that has never been saved) the snapshot is zeroed. */
void editor_snapshot_disk(void)
{
	struct stat st;

	editor.disk_changed = 0;
	if (editor.filename && stat(editor.filename, &st) == 0) {
		editor.disk_mtime = st.st_mtime;
		editor.disk_size  = st.st_size;
	} else {
		editor.disk_mtime = 0;
		editor.disk_size  = 0;
	}
}

/* Return 1 if the file at `path` exists and its mtime or size disagrees
 * with the supplied snapshot.  Used both by editor_save (where the snapshot
 * comes from the buffer's own last-seen state) and by autorevert_poll. */
int file_state_differs(const char *path, time_t mtime, off_t size)
{
	struct stat st;

	if (stat(path, &st) != 0) return 0;
	return st.st_mtime != mtime || st.st_size != size;
}

/* Load the specified program in the editor memory and returns 0 on success
 * or 1 on error. */
int editor_open(char *filename)
{
	ssize_t linelen;
	size_t linecap = 0;
	size_t fnlen = strlen(filename) + 1;
	char *line = NULL;
	FILE *fp;
	int ended_with_newline = 0;

	editor.dirty = 0;
	editor.backed_up = 0;
	free(editor.filename);
	editor.filename = malloc(fnlen);
	memcpy(editor.filename, filename, fnlen);

	fp = fopen(filename, "r");
	if (!fp) {
		if (errno != ENOENT) {
			perror("Opening file");
			exit(1);
		}
		editor_snapshot_disk();
		return 1;
	}

	while ((linelen = getline(&line, &linecap, fp)) != -1) {
		ended_with_newline = 0;
		if (linelen && (line[linelen-1] == '\n' || line[linelen-1] == '\r')) {
			line[--linelen] = '\0';
			ended_with_newline = 1;
		}
		editor_insert_row(editor.numrows, line, linelen);
	}
	/* A file ending in a newline has a trailing empty line, like GNU
	 * Emacs; represent it as an empty row so the newline round-trips. */
	if (ended_with_newline)
		editor_insert_row(editor.numrows, "", 0);
	free(line);
	fclose(fp);
	/* A file we can't write opens read-only, like GNU Emacs, so the mode
	 * line shows %%.  Only ever adds read-only; an explicit -R stays. */
	if (access(filename, W_OK) != 0)
		editor.readonly = 1;
	editor.dirty = 0;
	undo_mark_clean();  /* Mark initial file state as clean */
	editor_snapshot_disk();
	return 0;
}

/* Write `buf` (len bytes) to `path` atomically: create a temp file in
 * the target's directory, copy the target's permissions and owner onto
 * it, flush it to disk, then rename it over the target -- so a reader
 * only ever sees the old file or the complete new one, and a failed or
 * short write can't truncate the original.  A symlinked path is resolved
 * so the real file is replaced with the link left pointing at it.  When
 * `backup` is set, the file being replaced is first renamed to path~; that
 * leaves the target briefly absent, so the strict old-or-new guarantee above
 * holds only on the plain path.
 * Returns 0 on success, -1 on error with errno set.
 *
 * The rename gives the saved file a new inode, so hard links to the
 * original are not followed and setuid/setgid/sticky bits are dropped. */
int write_file_atomic(const char *path, const char *buf, int len, int backup)
{
	char real[PATH_MAX];
	char tmp[PATH_MAX];
	char bpath[PATH_MAX];
	struct stat st;
	const char *target = path;
	int have_meta = 0;
	int do_backup;
	int tmpfd;
	char *slash;
	int off;

	/* Resolve a symlink so we replace its target, not the link itself;
	 * lstat has already captured a non-symlink's own metadata. */
	if (lstat(path, &st) == 0) {
		if (S_ISLNK(st.st_mode)) {
			if (realpath(path, real) != NULL &&
			    stat(real, &st) == 0) {
				target = real;
				have_meta = 1;
			}
		} else {
			have_meta = 1;
		}
	}
	do_backup = backup && have_meta;

	/* Temp file beside the target, on the same filesystem so the rename
	 * is atomic. */
	if (snprintf(tmp, sizeof(tmp), "%s", target) >= (int)sizeof(tmp)) {
		errno = ENAMETOOLONG;
		return -1;
	}
	slash = strrchr(tmp, '/');
	if (slash)
		snprintf(slash + 1, sizeof(tmp) - (slash + 1 - tmp), ".kg-XXXXXX");
	else
		snprintf(tmp, sizeof(tmp), ".kg-XXXXXX");
	tmpfd = mkstemp(tmp);
	if (tmpfd == -1)
		return -1;

	if (have_meta) {
		/* Reapply the original mode and, where permitted, its owner;
		 * a non-root save of someone else's file keeps its own owner. */
		if (fchmod(tmpfd, st.st_mode & 0777) == -1)
			goto fail;
		if (fchown(tmpfd, st.st_uid, st.st_gid) == -1 && errno != EPERM)
			goto fail;
	} else {
		/* New file: match open(..., 0644) masked by the umask. */
		mode_t um = umask(0);
		umask(um);
		if (fchmod(tmpfd, 0644 & ~um) == -1)
			goto fail;
	}

	for (off = 0; off < len; ) {
		ssize_t n = write(tmpfd, buf + off, len - off);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			goto fail;
		}
		if (n == 0) {
			errno = EIO;
			goto fail;
		}
		off += n;
	}

	if (fsync(tmpfd) == -1)
		goto fail;
	if (close(tmpfd) == -1) {
		tmpfd = -1;
		goto fail;
	}
	/* When asked, preserve the file being replaced as path~ (Emacs-style
	 * make-backup-files).  Renaming keeps the original's metadata on the
	 * backup; if the final rename fails, put it back so the target is
	 * never left missing. */
	if (do_backup) {
		if (snprintf(bpath, sizeof(bpath), "%s~", target) >= (int)sizeof(bpath)) {
			errno = ENAMETOOLONG;
			goto fail;
		}
		if (rename(target, bpath) == -1)
			goto fail;
	}
	if (rename(tmp, target) == -1) {
		if (do_backup)
			rename(bpath, target);
		tmpfd = -1;
		goto fail;
	}
	return 0;

fail:
	{
		int saved = errno;
		if (tmpfd != -1)
			close(tmpfd);
		unlink(tmp);
		errno = saved;
	}
	return -1;
}

/* Save the current file on disk. Return 0 on success, 1 on error.
 * Special buffers (filename is NULL or starts with '*') prompt for a name. */
int editor_save(int fd)
{
	struct stat st;
	char *newfilename;
	char *buf;
	int len;
	int answer;

	if (is_special_buffer(editor.filename)) {
		char newname[256];

		editor_prompt_prefill_dir(newname, sizeof(newname));
		if (editor_read_line_path(fd, "Write file: ", newname, sizeof(newname)) < 0
		    || !newname[0])
			return 1;

		/* If the entered path already exists, warn before clobbering — and
		 * do it *before* mutating the buffer's filename or syntax so that
		 * a "no" answer leaves the buffer untouched. */
		if (stat(newname, &st) == 0) {
			editor_set_status_message("File %s exists.  Overwrite? (y/n) ", newname);
			editor_refresh_screen();
			answer = editor_read_key(fd);
			if (answer != 'y' && answer != 'Y') {
				editor_set_status_message("Save aborted");
				return 1;
			}
		}

		newfilename = strdup(newname);
		if (!newfilename) {
			editor_set_status_message("Out of memory");
			return 1;
		}
		free(editor.filename);
		editor.filename = newfilename;
		editor_select_syntax_highlight(editor.filename);
	} else if (file_state_differs(editor.filename,
	                              editor.disk_mtime, editor.disk_size)) {
		editor_set_status_message("File %s changed on disk.  Save anyway? (y/n) ",
		                          editor.filename);
		editor_refresh_screen();
		answer = editor_read_key(fd);
		if (answer != 'y' && answer != 'Y') {
			editor_set_status_message("Save aborted");
			return 1;
		}
	}

	/* require-final-newline: give the buffer a trailing empty row so the
	 * saved file ends in a newline, visibly, like GNU Emacs. */
	if (require_final_newline && editor.numrows > 0 &&
	    editor.row[editor.numrows - 1].size > 0)
		editor_insert_row(editor.numrows, "", 0);

	buf = editor_rows_to_string(editor.row, editor.numrows, &len);
	if (write_file_atomic(editor.filename, buf, len,
	                      make_backup_files && !editor.backed_up) == -1) {
		free(buf);
		editor_set_status_message("Error writing %s: %s",
		                          editor.filename, strerror(errno));
		return 1;
	}

	free(buf);
	editor.dirty = 0;
	editor.backed_up = 1;
	undo_mark_clean();  /* Mark this state as clean for undo tracking */
	editor_snapshot_disk();
	editor_set_status_message("Wrote %s (%d bytes)", editor.filename, len);
	return 0;
}

/* Prompt for a new filename, write the buffer there, and adopt that name.
 * This is Emacs C-x C-w (write-file / save as). */
void editor_write_file(int fd)
{
	char newname[256];
	char *newfilename;

	editor_prompt_prefill_dir(newname, sizeof(newname));
	if (editor_read_line_path(fd, "Write file: ", newname, sizeof(newname)) < 0 || !newname[0])
		return;
	newfilename = strdup(newname);
	if (!newfilename)
		return;
	free(editor.filename);
	editor.filename = newfilename;
	editor.backed_up = 0;
	editor_select_syntax_highlight(editor.filename);
	editor_save(fd);
}

/* Prompt for a filename and insert its contents at point.
 * This is Emacs C-x i (insert-file). */
void editor_insert_file(int fd)
{
	char filename[256];
	char *buf = NULL;
	size_t buflen = 0, bufcap = 0;
	char tmp[4096];
	size_t n;
	int filerow, filecol;
	FILE *fp;

	if (editor_readonly_blocked())
		return;

	editor_prompt_prefill_dir(filename, sizeof(filename));
	if (editor_read_line_path(fd, "Insert file: ", filename, sizeof(filename)) < 0 || !filename[0])
		return;

	fp = fopen(filename, "r");
	if (!fp) {
		editor_set_status_message("Cannot open %s: %s", filename, strerror(errno));
		return;
	}

	while ((n = fread(tmp, 1, sizeof(tmp), fp)) > 0) {
		if (buflen + n >= bufcap) {
			char *newbuf;

			bufcap = (buflen + n) * 2 + 1;
			newbuf = realloc(buf, bufcap);
			if (!newbuf) {
				free(buf);
				fclose(fp);
				editor_set_status_message("Out of memory reading %s", filename);
				return;
			}
			buf = newbuf;
		}
		memcpy(buf + buflen, tmp, n);
		buflen += n;
	}
	fclose(fp);

	if (!buflen) {
		free(buf);
		editor_set_status_message("(empty file)");
		return;
	}

	/* Strip a single trailing newline to avoid inserting a spurious blank
	 * line — most text files end with \n but we're inserting mid-buffer. */
	if (buf[buflen - 1] == '\n')
		buflen--;

	filerow = editor.rowoff + editor.cy;
	filecol = editor.coloff + editor.cx;
	undo_push(UNDO_YANK_TEXT, filerow, filecol, 0, buf, buflen);
	editor_insert_text_raw(buf, buflen);
	free(buf);

	editor_set_status_message("Inserted %s", filename);
}
