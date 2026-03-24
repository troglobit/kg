/* ========================= Buffer management ============================== */

#include "def.h"

/* Synthetic syntax records for special modes. */
static struct editor_syntax ibuffer_syntax = {
	"IBuffer", NULL, NULL, "", "", "", 0
};
static struct editor_syntax text_syntax = {
	"Text", NULL, NULL, "", "", "", 0
};

/* Column offset of the filename field in a *Buffer List* data row.
 * Format: " %c  %-24s  %6d  %-14s  %s"
 *          1+1+2+24  +2+6  +2+14  +2 = 54  */
#define IBUF_FILENAME_OFFSET 54
#define IBUF_NAME "*Buffer List*"

struct editor_buffer buflist[MAX_BUFFERS];
int buf_current = 0;
int buf_count   = 0;

/* Save live editor state (and global undostack) into buflist[idx]. */
static void buf_save_to_slot(int idx)
{
	struct editor_buffer *b = &buflist[idx];

	b->cx = editor.cx;           b->cy = editor.cy;
	b->rowoff = editor.rowoff;   b->coloff = editor.coloff;
	b->numrows = editor.numrows;
	b->row = editor.row;
	b->dirty = editor.dirty;
	b->filename = editor.filename;
	b->syntax = editor.syntax;
	b->mark_set = editor.mark_set;
	b->mark_row = editor.mark_row;   b->mark_col = editor.mark_col;
	b->undostack = undostack;   /* struct copy — pointer ownership moves here */
	b->readonly = editor.readonly;
	b->active = 1;
}

/* Restore buflist[idx] into live editor state (and global undostack). */
static void buf_restore_from_slot(int idx)
{
	struct editor_buffer *b = &buflist[idx];

	editor.cx = b->cx;           editor.cy = b->cy;
	editor.rowoff = b->rowoff;   editor.coloff = b->coloff;
	editor.numrows = b->numrows;
	editor.row = b->row;
	editor.dirty = b->dirty;
	editor.filename = b->filename;
	editor.syntax = b->syntax;
	editor.mark_set = b->mark_set;
	editor.mark_row = b->mark_row;   editor.mark_col = b->mark_col;
	undostack = b->undostack;   /* struct copy */
	editor.readonly = b->readonly;
	buf_current = idx;
	/* Keep the active window pointing at the newly-restored buffer. */
	if (win_count > 0)
		winlist[win_current].bufidx = idx;
}

/* Save current window view and buffer state before switching away. */
void buf_save_current_state(void)
{
	win_save_active_view();
	buf_save_to_slot(buf_current);
}

/* Reset editor to a clean empty state and initialise a fresh undo stack.
 * Used before loading a new file into editor. */
static void buf_reset(void)
{
	editor.cx = editor.cy = 0;
	editor.rowoff = editor.coloff = 0;
	editor.numrows = 0;
	editor.row = NULL;
	editor.dirty = 0;
	editor.filename = NULL;
	editor.syntax = NULL;
	editor.mark_set = editor.mark_row = editor.mark_col = 0;
	editor.cx_prefix = 0;
	editor.paste_mode = 0;
	editor.readonly = 0;
	undo_init();
}

/* Return the basename of a filename (part after last '/'), or the whole
 * string if no '/' is present.  Falls back to "[new]" for NULL. */
static const char *buf_basename(const char *filename)
{
	const char *base;
	if (!filename) return "[new]";
	base = strrchr(filename, '/');
	return base ? base+1 : filename;
}

/* Prompt the user for a line of text in the status bar.  Returns 0 on
 * confirmation (Enter) or -1 if cancelled (ESC / C-g).  buf is always
 * NUL-terminated on return. */
int editor_read_line(int fd, const char *prompt, char *buf, int bufsize)
{
	int len = 0, c;

	buf[0] = '\0';
	while (1) {
		editor_set_status_message("%s%s", prompt, buf);
		editor_refresh_screen();
		c = editor_read_key(fd);
		if (c == DEL_KEY || c == CTRL_H || c == BACKSPACE) {
			if (len > 0) buf[--len] = '\0';
		} else if (c == ESC || c == CTRL_G) {
			editor_set_status_message("");
			return -1;
		} else if (c == ENTER) {
			return 0;
		} else if (isprint(c) && len < bufsize - 1) {
			buf[len++] = c;
			buf[len] = '\0';
		}
	}
}

/* Load all command-line files into the buffer list, then start in buffer 0.
 * Called once from main() after init_editor().
 * Arguments of the form +LINE or +LINE:COL position the next file. */
void buf_load_args(int nfiles, char **filenames, int readonly)
{
	int i, slot = 0;
	int pending_line = 0, pending_col = 1;

	memset(buflist, 0, sizeof(buflist));
	buf_current = 0;
	buf_count = 0;

	if (nfiles == 0) {
		/* No files given: open an empty *scratch* buffer. */
		buf_reset();
		editor.filename = strdup("*scratch*");
		editor.syntax = &text_syntax;
		buf_save_to_slot(0);
		buflist[0].active = 1;
		buf_count = 1;
		buf_restore_from_slot(0);
		return;
	}

	for (i = 0; i < nfiles && slot < MAX_BUFFERS; i++) {
		if (filenames[i][0] == '+') {
			/* Position specifier: +LINE or +LINE:COL */
			pending_line = 0; pending_col = 1;
			sscanf(filenames[i] + 1, "%d:%d", &pending_line, &pending_col);
			continue;
		}
		buf_reset();
		editor.readonly = readonly;
		editor_select_syntax_highlight(filenames[i]);
		editor_open(filenames[i]);
		if (pending_line > 0) {
			editor_goto_line_direct(pending_line, pending_col);
			pending_line = 0; pending_col = 1;
		}
		buf_save_to_slot(slot);
		buflist[slot].active = 1;
		buf_count++;
		slot++;
	}
	buf_restore_from_slot(0);
}

/* Interactive buffer selector shown in the echo area (C-x b).
 * Displays all buffers except the current one in a ring; left/right arrows
 * rotate the selection; Enter switches, ESC/C-g cancels. */
void buf_select_interactive(int fd)
{
	int order[MAX_BUFFERS], n = 0, sel = 0;
	int i, c;
	char msg[256];
	int off;

	/* Build ring starting from the buffer after current (most natural default). */
	for (i = 1; i <= MAX_BUFFERS; i++) {
		int idx = (buf_current + i) % MAX_BUFFERS;
		if (buflist[idx].active) order[n++] = idx;
	}
	if (n == 0) {
		editor_set_status_message("No other buffers.");
		return;
	}

	while (1) {
		off = snprintf(msg, sizeof(msg), "Buffer: ");
		for (i = 0; i < n; i++) {
			const char *name = buf_basename(buflist[order[i]].filename);
			if (i == sel)
				off += snprintf(msg+off, sizeof(msg)-off, "[%s]  ", name);
			else
				off += snprintf(msg+off, sizeof(msg)-off, "%s  ", name);
		}
		editor_set_status_message("%s", msg);
		editor_refresh_screen();

		c = editor_read_key(fd);
		if (c == ARROW_RIGHT || c == CTRL_F) {
			sel = (sel + 1) % n;
		} else if (c == ARROW_LEFT || c == CTRL_B) {
			sel = (sel - 1 + n) % n;
		} else if (c == ENTER) {
			buf_save_current_state();
			buf_restore_from_slot(order[sel]);
			editor_set_status_message("");
			return;
		} else if (c == ESC || c == CTRL_G) {
			editor_set_status_message("");
			return;
		}
	}
}

/* Open a file in a new buffer, prompting for the filename.  If the file is
 * already open in an existing buffer, switch to it instead.
 * readonly: if 1, mark the buffer read-only after loading. */
static void buf_open_file_ro(int fd, int readonly)
{
	char query[256];
	int i, slot;
	const char *prompt = readonly ? "Open file read-only: " : "Open file: ";

	if (editor_read_line(fd, prompt, query, sizeof(query)) < 0 || query[0] == '\0')
		return;

	/* Switch to existing buffer if the file is already open. */
	for (i = 0; i < MAX_BUFFERS; i++) {
		if (buflist[i].active && buflist[i].filename &&
		    strcmp(buflist[i].filename, query) == 0) {
			buf_save_current_state();
			buf_restore_from_slot(i);
			editor_set_status_message("%s", editor.filename);
			return;
		}
	}

	if (buf_count >= MAX_BUFFERS) {
		editor_set_status_message("Too many open buffers (%d max).", MAX_BUFFERS);
		return;
	}

	/* Find a free slot. */
	slot = -1;
	for (i = 0; i < MAX_BUFFERS; i++) {
		if (!buflist[i].active) { slot = i; break; }
	}
	if (slot < 0) return; /* should not happen given buf_count check above */

	buf_save_current_state();
	buf_reset();
	editor.readonly = readonly;
	editor_select_syntax_highlight(query);
	editor_open(query);
	buf_save_to_slot(slot);
	buf_restore_from_slot(slot);
	buf_count++;
	editor_set_status_message("%s%s", editor.filename ? editor.filename : "[new]",
		readonly ? " [read-only]" : "");
}

void buf_open_file(int fd)     { buf_open_file_ro(fd, 0); }
void buf_open_file_read_only(int fd) { buf_open_file_ro(fd, 1); }

/* Write a buffer slot's rows directly to its file without switching to it.
 * Returns 0 on success, 1 on error (errno set). */
static int write_slot(struct editor_buffer *b)
{
	char *buf;
	int len, fd;

	buf = editor_rows_to_string(b->row, b->numrows, &len);
	fd = open(b->filename, O_RDWR|O_CREAT, 0644);
	if (fd == -1) { free(buf); return 1; }
	if (ftruncate(fd, len) == -1 || write(fd, buf, len) != len) {
		close(fd); free(buf); return 1;
	}
	close(fd);
	free(buf);
	return 0;
}

/* Save all modified non-special buffers, prompting for each (C-x s). */
void buf_save_all(int fd)
{
	int i;

	buf_save_to_slot(buf_current); /* flush current edits into slot */

	for (i = 0; i < MAX_BUFFERS; i++) {
		struct editor_buffer *b = &buflist[i];
		int answer;

		if (!b->active || !b->dirty) continue;
		if (is_special_buffer(b->filename)) continue;

		editor_set_status_message("Save %s? (y/n) ", b->filename);
		editor_refresh_screen();
		answer = editor_read_key(fd);
		if (answer != 'y' && answer != 'Y') continue;

		if (write_slot(b) == 0) {
			b->dirty = 0;
			if (i == buf_current) {
				editor.dirty = 0;
				undo_mark_clean();
			}
			editor_set_status_message("Wrote %s", b->filename);
		} else {
			editor_set_status_message("Error writing %s: %s",
				b->filename, strerror(errno));
		}
	}
}

/* Kill (close) the current buffer, prompting if modified. */
void buf_kill(int fd)
{
	int i;

	if (editor.dirty) {
		int answer;
		editor_set_status_message("Buffer modified, really kill? (y/n) ");
		editor_refresh_screen();
		answer = editor_read_key(fd);
		if (answer != 'y' && answer != 'Y') {
			editor_set_status_message("");
			return;
		}
	}

	/* Free current buffer's memory. */
	for (i = 0; i < editor.numrows; i++)
		editor_free_row(&editor.row[i]);
	free(editor.row);
	free(editor.filename);
	undo_free();

	buflist[buf_current].active = 0;
	buflist[buf_current].row = NULL;
	buflist[buf_current].filename = NULL;
	buf_count--;

	if (buf_count == 0) {
		running = 0;
		return;
	}

	/* Switch to the nearest remaining buffer. */
	for (i = 0; i < MAX_BUFFERS; i++) {
		if (buflist[i].active) { buf_restore_from_slot(i); break; }
	}
	editor_set_status_message("%s", editor.filename ? editor.filename : "[new]");
}

/* Open (or refresh) a *Buffer List* buffer in the current window (C-x C-b).
 * Lists all open buffers with modification flag, name, size, mode, and path.
 * Press q or C-x k to close. */
void buf_open_list(void)
{
	int i, j, slot = -1, existing = -1;
	char line[256];
	int len, size;
	const char *modename;

	/* Save current state so the snapshot is up-to-date. */
	buf_save_current_state();

	/* Find an existing *Buffer List* slot to reuse, or a free slot. */
	for (i = 0; i < MAX_BUFFERS; i++) {
		if (!buflist[i].active) {
			if (slot < 0) slot = i;
			continue;
		}
		if (buflist[i].filename &&
		    strcmp(buflist[i].filename, IBUF_NAME) == 0)
			existing = i;
	}

	if (existing >= 0) {
		/* Reuse: switch to it, then rebuild content below. */
		buf_restore_from_slot(existing);
		undo_init(); /* content is rebuilt from scratch; don't keep stale ops */
	} else {
		if (buf_count >= MAX_BUFFERS) {
			editor_set_status_message("Too many open buffers (%d max).", MAX_BUFFERS);
			return;
		}
		if (slot < 0) return;
		buf_reset();
		editor.filename = strdup(IBUF_NAME);
	}

	/* Clear existing rows. */
	for (i = 0; i < editor.numrows; i++) editor_free_row(&editor.row[i]);
	free(editor.row);
	editor.row = NULL;
	editor.numrows = 0;

	/* Header. */
	len = snprintf(line, sizeof(line), " M  %-24s  %6s  %-14s  %s",
		"Buffer", "Size", "Mode", "File");
	editor_insert_row(editor.numrows, line, len);
	len = snprintf(line, sizeof(line), " -  %-24s  %6s  %-14s  %s",
		"------", "------", "----", "----");
	editor_insert_row(editor.numrows, line, len);

	/* One row per buffer (using the just-saved snapshot in buflist[]). */
	for (i = 0; i < MAX_BUFFERS; i++) {
		struct editor_buffer *b = &buflist[i];
		if (!b->active) continue;

		modename = b->syntax ? b->syntax->name : "Fundamental";
		size = 0;
		for (j = 0; j < b->numrows; j++) size += b->row[j].size;

		len = snprintf(line, sizeof(line), " %c  %-24s  %6d  %-14s  %s",
			b->dirty ? '*' : ' ',
			buf_basename(b->filename),
			size, modename,
			b->filename ? b->filename : "");
		editor_insert_row(editor.numrows, line, len);
	}

	editor.cx = editor.cy = editor.rowoff = editor.coloff = 0;
	editor.dirty = 0;
	editor.readonly = 1;
	editor.syntax = &ibuffer_syntax;

	if (existing >= 0) {
		buf_save_to_slot(existing);
	} else {
		buf_save_to_slot(slot);
		buf_restore_from_slot(slot);
		buf_count++;
	}

	editor_set_status_message("Buffer list — RET to open, q or C-x k to close.");
}

/* Open the buffer named on the current IBuffer line.
 * Line format: " M  <24-char name>  <6-char size>  <14-char mode>  <filename>"
 * The full filename starts at byte offset 54. */
void buf_ibuffer_select(void)
{
	int filerow = editor.rowoff + editor.cy;
	const char *filename;
	int i;

	if (editor.syntax != &ibuffer_syntax) return; /* only valid in IBuffer mode */
	if (filerow < 2 || filerow >= editor.numrows) return; /* skip header rows */
	if (editor.row[filerow].size <= IBUF_FILENAME_OFFSET) return;

	filename = editor.row[filerow].chars + IBUF_FILENAME_OFFSET;
	if (!filename[0]) return;
	if (strcmp(filename, IBUF_NAME) == 0) return; /* don't recurse */

	for (i = 0; i < MAX_BUFFERS; i++) {
		if (!buflist[i].active || !buflist[i].filename) continue;
		if (strcmp(buflist[i].filename, filename) == 0) {
			buf_save_current_state();
			buf_restore_from_slot(i);
			editor_set_status_message("%s", editor.filename ? editor.filename : "[new]");
			return;
		}
	}
	editor_set_status_message("Buffer not found: %s", filename);
}
