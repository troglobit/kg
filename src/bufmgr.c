/* ========================= Buffer management ============================== */

#include "def.h"

/* Synthetic syntax records for special modes. */
static struct editorSyntax ibuffer_syntax = {
	"IBuffer", NULL, NULL, "", "", "", 0
};
static struct editorSyntax text_syntax = {
	"Text", NULL, NULL, "", "", "", 0
};

/* Column offset of the filename field in a *Buffer List* data row.
 * Format: " %c  %-24s  %6d  %-14s  %s"
 *          1+1+2+24  +2+6  +2+14  +2 = 54  */
#define IBUF_FILENAME_OFFSET 54
#define IBUF_NAME "*Buffer List*"

struct editorBuffer buflist[MAX_BUFFERS];
int buf_current = 0;
int buf_count   = 0;

/* Save live E state (and global undostack) into buflist[idx]. */
static void bufSaveToSlot(int idx)
{
	struct editorBuffer *b = &buflist[idx];

	b->cx = E.cx;           b->cy = E.cy;
	b->rowoff = E.rowoff;   b->coloff = E.coloff;
	b->numrows = E.numrows;
	b->row = E.row;
	b->dirty = E.dirty;
	b->filename = E.filename;
	b->syntax = E.syntax;
	b->mark_set = E.mark_set;
	b->mark_row = E.mark_row;   b->mark_col = E.mark_col;
	b->undostack = undostack;   /* struct copy — pointer ownership moves here */
	b->readonly = E.readonly;
	b->active = 1;
}

/* Restore buflist[idx] into live E state (and global undostack). */
static void bufRestoreFromSlot(int idx)
{
	struct editorBuffer *b = &buflist[idx];

	E.cx = b->cx;           E.cy = b->cy;
	E.rowoff = b->rowoff;   E.coloff = b->coloff;
	E.numrows = b->numrows;
	E.row = b->row;
	E.dirty = b->dirty;
	E.filename = b->filename;
	E.syntax = b->syntax;
	E.mark_set = b->mark_set;
	E.mark_row = b->mark_row;   E.mark_col = b->mark_col;
	undostack = b->undostack;   /* struct copy */
	E.readonly = b->readonly;
	buf_current = idx;
	/* Keep the active window pointing at the newly-restored buffer. */
	if (win_count > 0)
		winlist[win_current].bufidx = idx;
}

/* Save current window view and buffer state before switching away. */
void bufSaveCurrentState(void)
{
	winSaveActiveView();
	bufSaveToSlot(buf_current);
}

/* Reset E to a clean empty state and initialise a fresh undo stack.
 * Used before loading a new file into E. */
static void bufResetE(void)
{
	E.cx = E.cy = 0;
	E.rowoff = E.coloff = 0;
	E.numrows = 0;
	E.row = NULL;
	E.dirty = 0;
	E.filename = NULL;
	E.syntax = NULL;
	E.mark_set = E.mark_row = E.mark_col = 0;
	E.cx_prefix = 0;
	E.paste_mode = 0;
	E.readonly = 0;
	undoInit();
}

/* Return the basename of a filename (part after last '/'), or the whole
 * string if no '/' is present.  Falls back to "[new]" for NULL. */
static const char *bufBasename(const char *filename)
{
	const char *base;
	if (!filename) return "[new]";
	base = strrchr(filename, '/');
	return base ? base+1 : filename;
}

/* Prompt the user for a line of text in the status bar.  Returns 0 on
 * confirmation (Enter) or -1 if cancelled (ESC / C-g).  buf is always
 * NUL-terminated on return. */
int editorReadLine(int fd, const char *prompt, char *buf, int bufsize)
{
	int len = 0, c;

	buf[0] = '\0';
	while (1) {
		editorSetStatusMessage("%s%s", prompt, buf);
		editorRefreshScreen();
		c = editorReadKey(fd);
		if (c == DEL_KEY || c == CTRL_H || c == BACKSPACE) {
			if (len > 0) buf[--len] = '\0';
		} else if (c == ESC || c == CTRL_G) {
			editorSetStatusMessage("");
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
 * Called once from main() after initEditor().
 * Arguments of the form +LINE or +LINE:COL position the next file. */
void bufLoadArgs(int nfiles, char **filenames, int readonly)
{
	int i, slot = 0;
	int pending_line = 0, pending_col = 1;

	memset(buflist, 0, sizeof(buflist));
	buf_current = 0;
	buf_count = 0;

	if (nfiles == 0) {
		/* No files given: open an empty *scratch* buffer. */
		bufResetE();
		E.filename = strdup("*scratch*");
		E.syntax = &text_syntax;
		bufSaveToSlot(0);
		buflist[0].active = 1;
		buf_count = 1;
		bufRestoreFromSlot(0);
		return;
	}

	for (i = 0; i < nfiles && slot < MAX_BUFFERS; i++) {
		if (filenames[i][0] == '+') {
			/* Position specifier: +LINE or +LINE:COL */
			pending_line = 0; pending_col = 1;
			sscanf(filenames[i] + 1, "%d:%d", &pending_line, &pending_col);
			continue;
		}
		bufResetE();
		E.readonly = readonly;
		editorSelectSyntaxHighlight(filenames[i]);
		editorOpen(filenames[i]);
		if (pending_line > 0) {
			editorGotoLineDirect(pending_line, pending_col);
			pending_line = 0; pending_col = 1;
		}
		bufSaveToSlot(slot);
		buflist[slot].active = 1;
		buf_count++;
		slot++;
	}
	bufRestoreFromSlot(0);
}

/* Interactive buffer selector shown in the echo area (C-x b).
 * Displays all buffers except the current one in a ring; left/right arrows
 * rotate the selection; Enter switches, ESC/C-g cancels. */
void bufSelectInteractive(int fd)
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
		editorSetStatusMessage("No other buffers.");
		return;
	}

	while (1) {
		off = snprintf(msg, sizeof(msg), "Buffer: ");
		for (i = 0; i < n; i++) {
			const char *name = bufBasename(buflist[order[i]].filename);
			if (i == sel)
				off += snprintf(msg+off, sizeof(msg)-off, "[%s]  ", name);
			else
				off += snprintf(msg+off, sizeof(msg)-off, "%s  ", name);
		}
		editorSetStatusMessage("%s", msg);
		editorRefreshScreen();

		c = editorReadKey(fd);
		if (c == ARROW_RIGHT || c == CTRL_F) {
			sel = (sel + 1) % n;
		} else if (c == ARROW_LEFT || c == CTRL_B) {
			sel = (sel - 1 + n) % n;
		} else if (c == ENTER) {
			bufSaveCurrentState();
			bufRestoreFromSlot(order[sel]);
			editorSetStatusMessage("");
			return;
		} else if (c == ESC || c == CTRL_G) {
			editorSetStatusMessage("");
			return;
		}
	}
}

/* Open a file in a new buffer, prompting for the filename.  If the file is
 * already open in an existing buffer, switch to it instead.
 * readonly: if 1, mark the buffer read-only after loading. */
static void bufOpenFileRO(int fd, int readonly)
{
	char query[256];
	int i, slot;
	const char *prompt = readonly ? "Open file read-only: " : "Open file: ";

	if (editorReadLine(fd, prompt, query, sizeof(query)) < 0 || query[0] == '\0')
		return;

	/* Switch to existing buffer if the file is already open. */
	for (i = 0; i < MAX_BUFFERS; i++) {
		if (buflist[i].active && buflist[i].filename &&
		    strcmp(buflist[i].filename, query) == 0) {
			bufSaveCurrentState();
			bufRestoreFromSlot(i);
			editorSetStatusMessage("%s", E.filename);
			return;
		}
	}

	if (buf_count >= MAX_BUFFERS) {
		editorSetStatusMessage("Too many open buffers (%d max).", MAX_BUFFERS);
		return;
	}

	/* Find a free slot. */
	slot = -1;
	for (i = 0; i < MAX_BUFFERS; i++) {
		if (!buflist[i].active) { slot = i; break; }
	}
	if (slot < 0) return; /* should not happen given buf_count check above */

	bufSaveCurrentState();
	bufResetE();
	E.readonly = readonly;
	editorSelectSyntaxHighlight(query);
	editorOpen(query);
	bufSaveToSlot(slot);
	bufRestoreFromSlot(slot);
	buf_count++;
	editorSetStatusMessage("%s%s", E.filename ? E.filename : "[new]",
		readonly ? " [read-only]" : "");
}

void bufOpenFile(int fd)     { bufOpenFileRO(fd, 0); }
void bufOpenFileReadOnly(int fd) { bufOpenFileRO(fd, 1); }

/* Write a buffer slot's rows directly to its file without switching to it.
 * Returns 0 on success, 1 on error (errno set). */
static int writeSlot(struct editorBuffer *b)
{
	char *buf;
	int len, fd;

	buf = editorRowsToString(b->row, b->numrows, &len);
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
void bufSaveAll(int fd)
{
	int i;

	bufSaveToSlot(buf_current); /* flush current edits into slot */

	for (i = 0; i < MAX_BUFFERS; i++) {
		struct editorBuffer *b = &buflist[i];
		int answer;

		if (!b->active || !b->dirty) continue;
		if (isSpecialBuffer(b->filename)) continue;

		editorSetStatusMessage("Save %s? (y/n) ", b->filename);
		editorRefreshScreen();
		answer = editorReadKey(fd);
		if (answer != 'y' && answer != 'Y') continue;

		if (writeSlot(b) == 0) {
			b->dirty = 0;
			if (i == buf_current) {
				E.dirty = 0;
				undoMarkClean();
			}
			editorSetStatusMessage("Wrote %s", b->filename);
		} else {
			editorSetStatusMessage("Error writing %s: %s",
				b->filename, strerror(errno));
		}
	}
}

/* Kill (close) the current buffer, prompting if modified. */
void bufKill(int fd)
{
	int i;

	if (E.dirty) {
		int answer;
		editorSetStatusMessage("Buffer modified, really kill? (y/n) ");
		editorRefreshScreen();
		answer = editorReadKey(fd);
		if (answer != 'y' && answer != 'Y') {
			editorSetStatusMessage("");
			return;
		}
	}

	/* Free current buffer's memory. */
	for (i = 0; i < E.numrows; i++)
		editorFreeRow(&E.row[i]);
	free(E.row);
	free(E.filename);
	undoFree();

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
		if (buflist[i].active) { bufRestoreFromSlot(i); break; }
	}
	editorSetStatusMessage("%s", E.filename ? E.filename : "[new]");
}

/* Open (or refresh) a *Buffer List* buffer in the current window (C-x C-b).
 * Lists all open buffers with modification flag, name, size, mode, and path.
 * Press q or C-x k to close. */
void bufOpenList(void)
{
	int i, j, slot = -1, existing = -1;
	char line[256];
	int len, size;
	const char *modename;

	/* Save current state so the snapshot is up-to-date. */
	bufSaveCurrentState();

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
		bufRestoreFromSlot(existing);
		undoInit(); /* content is rebuilt from scratch; don't keep stale ops */
	} else {
		if (buf_count >= MAX_BUFFERS) {
			editorSetStatusMessage("Too many open buffers (%d max).", MAX_BUFFERS);
			return;
		}
		if (slot < 0) return;
		bufResetE();
		E.filename = strdup(IBUF_NAME);
	}

	/* Clear existing rows. */
	for (i = 0; i < E.numrows; i++) editorFreeRow(&E.row[i]);
	free(E.row);
	E.row = NULL;
	E.numrows = 0;

	/* Header. */
	len = snprintf(line, sizeof(line), " M  %-24s  %6s  %-14s  %s",
		"Buffer", "Size", "Mode", "File");
	editorInsertRow(E.numrows, line, len);
	len = snprintf(line, sizeof(line), " -  %-24s  %6s  %-14s  %s",
		"------", "------", "----", "----");
	editorInsertRow(E.numrows, line, len);

	/* One row per buffer (using the just-saved snapshot in buflist[]). */
	for (i = 0; i < MAX_BUFFERS; i++) {
		struct editorBuffer *b = &buflist[i];
		if (!b->active) continue;

		modename = b->syntax ? b->syntax->name : "Fundamental";
		size = 0;
		for (j = 0; j < b->numrows; j++) size += b->row[j].size;

		len = snprintf(line, sizeof(line), " %c  %-24s  %6d  %-14s  %s",
			b->dirty ? '*' : ' ',
			bufBasename(b->filename),
			size, modename,
			b->filename ? b->filename : "");
		editorInsertRow(E.numrows, line, len);
	}

	E.cx = E.cy = E.rowoff = E.coloff = 0;
	E.dirty = 0;
	E.readonly = 1;
	E.syntax = &ibuffer_syntax;

	if (existing >= 0) {
		bufSaveToSlot(existing);
	} else {
		bufSaveToSlot(slot);
		bufRestoreFromSlot(slot);
		buf_count++;
	}

	editorSetStatusMessage("Buffer list — RET to open, q or C-x k to close.");
}

/* Open the buffer named on the current IBuffer line.
 * Line format: " M  <24-char name>  <6-char size>  <14-char mode>  <filename>"
 * The full filename starts at byte offset 54. */
void bufIbufferSelect(void)
{
	int filerow = E.rowoff + E.cy;
	const char *filename;
	int i;

	if (E.syntax != &ibuffer_syntax) return; /* only valid in IBuffer mode */
	if (filerow < 2 || filerow >= E.numrows) return; /* skip header rows */
	if (E.row[filerow].size <= IBUF_FILENAME_OFFSET) return;

	filename = E.row[filerow].chars + IBUF_FILENAME_OFFSET;
	if (!filename[0]) return;
	if (strcmp(filename, IBUF_NAME) == 0) return; /* don't recurse */

	for (i = 0; i < MAX_BUFFERS; i++) {
		if (!buflist[i].active || !buflist[i].filename) continue;
		if (strcmp(buflist[i].filename, filename) == 0) {
			bufSaveCurrentState();
			bufRestoreFromSlot(i);
			editorSetStatusMessage("%s", E.filename ? E.filename : "[new]");
			return;
		}
	}
	editorSetStatusMessage("Buffer not found: %s", filename);
}
