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

	editor.dirty = 0;
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
		if (linelen && (line[linelen-1] == '\n' || line[linelen-1] == '\r'))
			line[--linelen] = '\0';
		editor_insert_row(editor.numrows, line, linelen);
	}
	free(line);
	fclose(fp);
	editor.dirty = 0;
	undo_mark_clean();  /* Mark initial file state as clean */
	editor_snapshot_disk();
	return 0;
}

/* Save the current file on disk. Return 0 on success, 1 on error.
 * Special buffers (filename is NULL or starts with '*') prompt for a name. */
int editor_save(int fd)
{
	struct stat st;
	char *newfilename;
	char *buf;
	int len;
	int filefd;
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

	buf = editor_rows_to_string(editor.row, editor.numrows, &len);
	filefd = open(editor.filename, O_RDWR|O_CREAT, 0644);
	if (filefd == -1) goto writeerr;

	/* Use truncate + a single write(2) call in order to make saving
	 * a bit safer, under the limits of what we can do in a small editor. */
	if (ftruncate(filefd, len) == -1) goto writeerr;
	if (write(filefd, buf, len) != len) goto writeerr;

	close(filefd);
	free(buf);
	editor.dirty = 0;
	undo_mark_clean();  /* Mark this state as clean for undo tracking */
	editor_snapshot_disk();
	editor_set_status_message("Wrote %s (%d bytes)", editor.filename, len);
	return 0;

writeerr:
	free(buf);
	if (filefd != -1)
		close(filefd);

	editor_set_status_message("Error writing %s: %s", editor.filename, strerror(errno));
	return 1;
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
