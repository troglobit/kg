/* ========================= Buffer management ============================== */

#include <string.h>
#include <ctype.h>
#include "def.h"

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
	buf_current = idx;
	/* Keep the active window pointing at the newly-restored buffer. */
	if (win_count > 0)
		winlist[win_current].bufidx = idx;
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
	undoInit();
}

/* Prompt the user for a line of text in the status bar.  Returns 0 on
 * confirmation (Enter) or -1 if cancelled (ESC / C-g).  buf is always
 * NUL-terminated on return. */
static int readLine(int fd, const char *prompt, char *buf, int bufsize)
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
 * Called once from main() after initEditor(). */
void bufLoadArgs(int nfiles, char **filenames)
{
	int i;

	memset(buflist, 0, sizeof(buflist));
	buf_current = 0;
	buf_count = 0;

	for (i = 0; i < nfiles && i < MAX_BUFFERS; i++) {
		bufResetE();
		editorSelectSyntaxHighlight(filenames[i]);
		editorOpen(filenames[i]);
		bufSaveToSlot(i);
		buflist[i].active = 1;
		buf_count++;
	}
	bufRestoreFromSlot(0);
}

/* Cycle to the next active buffer. */
void bufCycleNext(void)
{
	int i, next = -1;

	if (buf_count <= 1) {
		editorSetStatusMessage("No other buffers.");
		return;
	}
	for (i = 1; i <= MAX_BUFFERS; i++) {
		int idx = (buf_current + i) % MAX_BUFFERS;
		if (buflist[idx].active) { next = idx; break; }
	}
	if (next < 0) return;
	winSaveActiveView();
	bufSaveToSlot(buf_current);
	bufRestoreFromSlot(next);
	editorSetStatusMessage("[%d/%d] %s", buf_current+1, buf_count,
		E.filename ? E.filename : "[new]");
}

/* Open a file in a new buffer, prompting for the filename.  If the file is
 * already open in an existing buffer, switch to it instead. */
void bufOpenFile(int fd)
{
	char query[256];
	int i, slot;

	if (readLine(fd, "Open file: ", query, sizeof(query)) < 0 || query[0] == '\0')
		return;

	/* Switch to existing buffer if the file is already open. */
	for (i = 0; i < MAX_BUFFERS; i++) {
		if (buflist[i].active && buflist[i].filename &&
		    strcmp(buflist[i].filename, query) == 0) {
			winSaveActiveView();
			bufSaveToSlot(buf_current);
			bufRestoreFromSlot(i);
			editorSetStatusMessage("[%d/%d] %s", buf_current+1, buf_count,
				E.filename);
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

	winSaveActiveView();
	bufSaveToSlot(buf_current);
	bufResetE();
	editorSelectSyntaxHighlight(query);
	editorOpen(query);
	bufSaveToSlot(slot);
	bufRestoreFromSlot(slot);
	buf_count++;
	editorSetStatusMessage("[%d/%d] %s", buf_current+1, buf_count,
		E.filename ? E.filename : "[new]");
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
	editorSetStatusMessage("[%d/%d] %s", buf_current+1, buf_count,
		E.filename ? E.filename : "[new]");
}

/* Show a one-line buffer list in the status message. */
void bufListMessage(void)
{
	char msg[80];
	int off = 0, i;
	const char *name, *base;

	bufSaveToSlot(buf_current); /* ensure buflist reflects current state */
	for (i = 0; i < MAX_BUFFERS && off < (int)sizeof(msg)-2; i++) {
		if (!buflist[i].active) continue;
		name = buflist[i].filename ? buflist[i].filename : "[new]";
		base = strrchr(name, '/');
		base = base ? base+1 : name;
		off += snprintf(msg+off, sizeof(msg)-off, "%s%s%s ",
			i == buf_current ? "*" : " ",
			base,
			buflist[i].dirty ? "+" : "");
	}
	editorSetStatusMessage("Buffers: %s", msg);
}
