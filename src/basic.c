/* ========================= Editor events handling  ======================== */

#include "def.h"

/* Handle cursor position change because arrow keys were pressed. */
void editor_move_cursor(int key)
{
	erow *row = (editor.rowoff + editor.cy >= editor.numrows) ? NULL : &editor.row[editor.rowoff + editor.cy];
	int filerow = editor.rowoff + editor.cy;
	int filecol = editor.coloff + editor.cx;
	int rowlen;

	switch (key) {
	case HOME_KEY:
		editor.cx = 0;
		editor.coloff = 0;
		break;
	case END_KEY:
		if (row) {
			if (row->size > editor.screencols - 1) {
				editor.coloff = row->size - editor.screencols + 1;
				editor.cx = editor.screencols - 1;
			} else {
				editor.cx = row->size;
				editor.coloff = 0;
			}
		}
		break;
	case ARROW_LEFT:
		if (editor.cx == 0) {
			if (editor.coloff) {
				editor.coloff--;
			} else {
				if (filerow > 0) {
					editor.cy--;
					editor.cx = editor.row[filerow-1].size;
					if (editor.cx > editor.screencols - 1) {
						editor.coloff = editor.cx - editor.screencols + 1;
						editor.cx = editor.screencols - 1;
					}
				}
			}
		} else {
			editor.cx -= 1;
		}
		break;
	case ARROW_RIGHT:
		if (row && filecol < row->size) {
			if (editor.cx == editor.screencols - 1) {
				editor.coloff++;
			} else {
				editor.cx += 1;
			}
		} else if (row && filecol == row->size) {
			editor.cx = 0;
			editor.coloff = 0;
			if (editor.cy == editor.screenrows - 1) {
				editor.rowoff++;
			} else {
				editor.cy += 1;
			}
		}
		break;
	case ARROW_UP:
		if (editor.cy == 0) {
			if (editor.rowoff) editor.rowoff--;
		} else {
			editor.cy -= 1;
		}
		break;
	case ARROW_DOWN:
		if (filerow < editor.numrows) {
			if (editor.cy == editor.screenrows - 1) {
				editor.rowoff++;
			} else {
				editor.cy += 1;
			}
		}
		break;
	}
	/* Fix cx if the current line has not enough chars. */
	filerow = editor.rowoff + editor.cy;
	filecol = editor.coloff + editor.cx;
	row = (filerow >= editor.numrows) ? NULL : &editor.row[filerow];
	rowlen = row ? row->size : 0;
	if (filecol > rowlen) {
		editor.cx -= filecol - rowlen;
		if (editor.cx < 0) {
			editor.coloff += editor.cx;
			editor.cx = 0;
		}
	}
}

/* Move to the beginning of the document */
void editor_move_to_beginning(void)
{
	editor.cx = 0;
	editor.cy = 0;
	editor.rowoff = 0;
	editor.coloff = 0;
}

/* Jump to a specific line (1-based) and column (1-based, 0 = start). */
void editor_goto_line_direct(int line, int col)
{
	int filerow, filecol;
	erow *row;

	if (editor.numrows == 0) return;
	if (line < 1) line = 1;
	if (line > editor.numrows) line = editor.numrows;

	filerow = line - 1;
	filecol = (col > 1) ? col - 1 : 0;
	row = &editor.row[filerow];
	if (filecol > row->size) filecol = row->size;

	/* Centre the target line vertically. */
	editor.rowoff = filerow - editor.screenrows / 2;
	if (editor.rowoff < 0) editor.rowoff = 0;
	editor.cy = filerow - editor.rowoff;

	if (filecol > editor.screencols - 1) {
		editor.coloff = filecol - editor.screencols + 1;
		editor.cx = editor.screencols - 1;
	} else {
		editor.coloff = 0;
		editor.cx = filecol;
	}
}

/* Prompt for a line number (optionally "LINE:COL") and jump to it. */
void editor_goto_line(int fd)
{
	char buf[16];
	int line = 0, col = 1, n;

	if (editor_read_line(fd, "Goto line: ", buf, sizeof(buf)) < 0 || !buf[0])
		return;
	n = sscanf(buf, "%d:%d", &line, &col);
	if (n < 1) return;
	if (n < 2) col = 1;
	editor_goto_line_direct(line, col);
}

/* Move to the end of the document */
void editor_move_to_end(void)
{
	erow *row;
	int filerow;

	if (editor.numrows == 0) return;

	filerow = editor.numrows - 1;
	row = &editor.row[filerow];

	/* Update cursor position */
	if (filerow >= editor.rowoff + editor.screenrows) {
		editor.rowoff = filerow - editor.screenrows + 1;
		editor.cy = editor.screenrows - 1;
	} else {
		editor.cy = filerow - editor.rowoff;
	}

	/* Move to end of last line */
	if (row->size > editor.screencols - 1) {
		editor.coloff = row->size - editor.screencols + 1;
		editor.cx = editor.screencols - 1;
	} else {
		editor.cx = row->size;
		editor.coloff = 0;
	}
}
