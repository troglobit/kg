/* ============================ Word movement =============================== */

#include "def.h"

/* Move cursor forward by one word (to start of next word). */
void editorMoveWordForward(void)
{
	erow *row = (E.rowoff + E.cy >= E.numrows) ? NULL : &E.row[E.rowoff + E.cy];
	int filecol = E.coloff + E.cx;

	if (!row) return;

	/* If on whitespace, skip it first so we always land at the same
	 * relative position (start of next word) regardless of entry point. */
	if (filecol < row->size && isspace((unsigned char)row->chars[filecol])) {
		while (filecol < row->size && isspace((unsigned char)row->chars[filecol])) {
			editorMoveCursor(ARROW_RIGHT);
			filecol = E.coloff + E.cx;
		}
		return;
	}

	/* Skip current word */
	while (filecol < row->size && !isspace((unsigned char)row->chars[filecol])) {
		editorMoveCursor(ARROW_RIGHT);
		filecol = E.coloff + E.cx;
	}

	/* Skip whitespace to land at start of next word */
	while (filecol < row->size && isspace((unsigned char)row->chars[filecol])) {
		editorMoveCursor(ARROW_RIGHT);
		filecol = E.coloff + E.cx;
	}
}

/* Move cursor backward by one word */
void editorMoveWordBackward(void)
{
	erow *row = (E.rowoff + E.cy >= E.numrows) ? NULL : &E.row[E.rowoff + E.cy];
	int filerow = E.rowoff + E.cy;
	int filecol = E.coloff + E.cx;

	if (!row) return;
	if (filecol == 0) {
		/* Move to end of previous line */
		if (filerow > 0) {
			editorMoveCursor(ARROW_LEFT);
		}
		return;
	}

	/* Move back one position to check current position */
	editorMoveCursor(ARROW_LEFT);
	filerow = E.rowoff + E.cy;
	filecol = E.coloff + E.cx;
	row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

	if (!row) return;

	/* Skip whitespace */
	while (filecol > 0 && isspace(row->chars[filecol])) {
		editorMoveCursor(ARROW_LEFT);
		filecol = E.coloff + E.cx;
	}

	/* Skip word characters */
	while (filecol > 0 && !isspace(row->chars[filecol - 1])) {
		editorMoveCursor(ARROW_LEFT);
		filecol = E.coloff + E.cx;
	}
}

/* Kill from cursor to start of next word, saving text to kill ring (M-d). */
void editorKillWordForward(void)
{
	int filerow = E.rowoff + E.cy;
	int filecol = E.coloff + E.cx;
	int start_col = filecol;
	int kill_len;
	char *text;
	erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

	if (!row) return;

	/* Mirror editorMoveWordForward: skip whitespace OR word+whitespace */
	if (filecol < row->size && isspace((unsigned char)row->chars[filecol])) {
		while (filecol < row->size && isspace((unsigned char)row->chars[filecol]))
			filecol++;
	} else {
		while (filecol < row->size && !isspace((unsigned char)row->chars[filecol]))
			filecol++;
		while (filecol < row->size && isspace((unsigned char)row->chars[filecol]))
			filecol++;
	}

	kill_len = filecol - start_col;
	if (kill_len <= 0) return;

	text = malloc(kill_len + 1);
	if (!text) return;
	memcpy(text, row->chars + start_col, kill_len);
	text[kill_len] = '\0';

	killRingSet(text, kill_len);
	undoPush(UNDO_KILL_TEXT, filerow, start_col, 0, text, kill_len);
	free(text);

	memmove(row->chars + start_col, row->chars + start_col + kill_len,
	        row->size - start_col - kill_len + 1);
	row->size -= kill_len;
	editorUpdateRow(row);
	E.dirty++;
}

/* Kill from start of current word back to cursor, saving text to kill ring (M-Backspace). */
void editorKillWordBackward(void)
{
	int filerow = E.rowoff + E.cy;
	int filecol = E.coloff + E.cx;
	int end_col = filecol;
	int kill_len;
	char *text;
	erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

	if (!row || filecol == 0) return;

	/* Mirror editorMoveWordBackward: skip whitespace then word chars */
	while (filecol > 0 && isspace((unsigned char)row->chars[filecol - 1]))
		filecol--;
	while (filecol > 0 && !isspace((unsigned char)row->chars[filecol - 1]))
		filecol--;

	kill_len = end_col - filecol;
	if (kill_len <= 0) return;

	text = malloc(kill_len + 1);
	if (!text) return;
	memcpy(text, row->chars + filecol, kill_len);
	text[kill_len] = '\0';

	killRingSet(text, kill_len);
	undoPush(UNDO_KILL_TEXT, filerow, filecol, 0, text, kill_len);
	free(text);

	if (filecol < E.coloff) {
		E.coloff = filecol;
		E.cx = 0;
	} else {
		E.cx = filecol - E.coloff;
	}

	memmove(row->chars + filecol, row->chars + filecol + kill_len,
	        row->size - filecol - kill_len + 1);
	row->size -= kill_len;
	editorUpdateRow(row);
	E.dirty++;
}

/* Move to the beginning of the previous paragraph (or beginning of buffer) */
void editorMoveParagraphBackward(void)
{
	int filerow = E.rowoff + E.cy;
	int found_blank = 0;
	erow *row;

	/* If we're at the first line, we can't go back */
	if (filerow == 0) {
		editorMoveCursor(HOME_KEY);
		return;
	}

	/* Move up one line to start search */
	filerow--;

	/* Skip any blank lines we're currently on */
	while (filerow >= 0) {
		row = &E.row[filerow];
		if (row->size != 0)
			break;
		filerow--;
	}

	/* Now find the next blank line (paragraph separator) */
	while (filerow >= 0) {
		row = &E.row[filerow];
		if (row->size == 0) {
			found_blank = 1;
			break;
		}
		filerow--;
	}

	/* Position cursor at the line after the blank line, or at beginning */
	if (found_blank && filerow < E.numrows - 1) {
		filerow++;
	} else if (!found_blank) {
		filerow = 0;
	}

	/* Update cursor position */
	if (filerow < E.rowoff) {
		E.rowoff = filerow;
		E.cy = 0;
	} else {
		E.cy = filerow - E.rowoff;
	}
	E.cx = 0;
	E.coloff = 0;
}

/* Move to the beginning of the next paragraph (or end of buffer) */
void editorMoveParagraphForward(void)
{
	int filerow = E.rowoff + E.cy;
	int found_blank = 0;
	erow *row;

	/* If we're at the last line, we can't go forward */
	if (filerow >= E.numrows - 1) {
		editorMoveCursor(END_KEY);
		return;
	}

	/* Move down one line to start search */
	filerow++;

	/* Skip any blank lines we're currently on */
	while (filerow < E.numrows) {
		row = &E.row[filerow];
		if (row->size != 0)
			break;
		filerow++;
	}

	/* Now find the next blank line (paragraph separator) */
	while (filerow < E.numrows) {
		row = &E.row[filerow];
		if (row->size == 0) {
			found_blank = 1;
			break;
		}
		filerow++;
	}

	/* Position cursor at the line after the blank line, or at end */
	if (found_blank && filerow < E.numrows - 1) {
		filerow++;
	} else if (!found_blank && filerow >= E.numrows) {
		filerow = E.numrows - 1;
	}

	/* Update cursor position */
	if (filerow >= E.rowoff + E.screenrows) {
		E.rowoff = filerow - E.screenrows + 1;
		E.cy = E.screenrows - 1;
	} else {
		E.cy = filerow - E.rowoff;
	}
	E.cx = 0;
	E.coloff = 0;
}
