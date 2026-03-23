/* ========================= Editor events handling  ======================== */

#include "def.h"

/* Handle cursor position change because arrow keys were pressed. */
void editorMoveCursor(int key)
{
	erow *row = (E.rowoff + E.cy >= E.numrows) ? NULL : &E.row[E.rowoff + E.cy];
	int filerow = E.rowoff + E.cy;
	int filecol = E.coloff + E.cx;
	int rowlen;

	switch (key) {
	case HOME_KEY:
		E.cx = 0;
		E.coloff = 0;
		break;
	case END_KEY:
		if (row) {
			if (row->size > E.screencols - 1) {
				E.coloff = row->size - E.screencols + 1;
				E.cx = E.screencols - 1;
			} else {
				E.cx = row->size;
				E.coloff = 0;
			}
		}
		break;
	case ARROW_LEFT:
		if (E.cx == 0) {
			if (E.coloff) {
				E.coloff--;
			} else {
				if (filerow > 0) {
					E.cy--;
					E.cx = E.row[filerow-1].size;
					if (E.cx > E.screencols - 1) {
						E.coloff = E.cx - E.screencols + 1;
						E.cx = E.screencols - 1;
					}
				}
			}
		} else {
			E.cx -= 1;
		}
		break;
	case ARROW_RIGHT:
		if (row && filecol < row->size) {
			if (E.cx == E.screencols - 1) {
				E.coloff++;
			} else {
				E.cx += 1;
			}
		} else if (row && filecol == row->size) {
			E.cx = 0;
			E.coloff = 0;
			if (E.cy == E.screenrows - 1) {
				E.rowoff++;
			} else {
				E.cy += 1;
			}
		}
		break;
	case ARROW_UP:
		if (E.cy == 0) {
			if (E.rowoff) E.rowoff--;
		} else {
			E.cy -= 1;
		}
		break;
	case ARROW_DOWN:
		if (filerow < E.numrows) {
			if (E.cy == E.screenrows - 1) {
				E.rowoff++;
			} else {
				E.cy += 1;
			}
		}
		break;
	}
	/* Fix cx if the current line has not enough chars. */
	filerow = E.rowoff + E.cy;
	filecol = E.coloff + E.cx;
	row = (filerow >= E.numrows) ? NULL : &E.row[filerow];
	rowlen = row ? row->size : 0;
	if (filecol > rowlen) {
		E.cx -= filecol - rowlen;
		if (E.cx < 0) {
			E.coloff += E.cx;
			E.cx = 0;
		}
	}
}

/* Move to the beginning of the document */
void editorMoveToBeginning(void)
{
	E.cx = 0;
	E.cy = 0;
	E.rowoff = 0;
	E.coloff = 0;
}

/* Jump to a specific line (1-based) and column (1-based, 0 = start). */
void editorGotoLineDirect(int line, int col)
{
	int filerow, filecol;
	erow *row;

	if (E.numrows == 0) return;
	if (line < 1) line = 1;
	if (line > E.numrows) line = E.numrows;

	filerow = line - 1;
	filecol = (col > 1) ? col - 1 : 0;
	row = &E.row[filerow];
	if (filecol > row->size) filecol = row->size;

	/* Centre the target line vertically. */
	E.rowoff = filerow - E.screenrows / 2;
	if (E.rowoff < 0) E.rowoff = 0;
	E.cy = filerow - E.rowoff;

	if (filecol > E.screencols - 1) {
		E.coloff = filecol - E.screencols + 1;
		E.cx = E.screencols - 1;
	} else {
		E.coloff = 0;
		E.cx = filecol;
	}
}

/* Prompt for a line number (optionally "LINE:COL") and jump to it. */
void editorGotoLine(int fd)
{
	char buf[16];
	int line = 0, col = 1, n;

	if (editorReadLine(fd, "Goto line: ", buf, sizeof(buf)) < 0 || !buf[0])
		return;
	n = sscanf(buf, "%d:%d", &line, &col);
	if (n < 1) return;
	if (n < 2) col = 1;
	editorGotoLineDirect(line, col);
}

/* Move to the end of the document */
void editorMoveToEnd(void)
{
	erow *row;
	int filerow;

	if (E.numrows == 0) return;

	filerow = E.numrows - 1;
	row = &E.row[filerow];

	/* Update cursor position */
	if (filerow >= E.rowoff + E.screenrows) {
		E.rowoff = filerow - E.screenrows + 1;
		E.cy = E.screenrows - 1;
	} else {
		E.cy = filerow - E.rowoff;
	}

	/* Move to end of last line */
	if (row->size > E.screencols - 1) {
		E.coloff = row->size - E.screencols + 1;
		E.cx = E.screencols - 1;
	} else {
		E.cx = row->size;
		E.coloff = 0;
	}
}
