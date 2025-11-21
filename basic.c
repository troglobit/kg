/* ========================= Editor events handling  ======================== */

#include "def.h"

/* Handle cursor position change because arrow keys were pressed. */
void editorMoveCursor(int key) {
    int filerow = E.rowoff+E.cy;
    int filecol = E.coloff+E.cx;
    int rowlen;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    switch(key) {
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
                    if (E.cx > E.screencols-1) {
                        E.coloff = E.cx-E.screencols+1;
                        E.cx = E.screencols-1;
                    }
                }
            }
        } else {
            E.cx -= 1;
        }
        break;
    case ARROW_RIGHT:
        if (row && filecol < row->size) {
            if (E.cx == E.screencols-1) {
                E.coloff++;
            } else {
                E.cx += 1;
            }
        } else if (row && filecol == row->size) {
            E.cx = 0;
            E.coloff = 0;
            if (E.cy == E.screenrows-1) {
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
            if (E.cy == E.screenrows-1) {
                E.rowoff++;
            } else {
                E.cy += 1;
            }
        }
        break;
    }
    /* Fix cx if the current line has not enough chars. */
    filerow = E.rowoff+E.cy;
    filecol = E.coloff+E.cx;
    row = (filerow >= E.numrows) ? NULL : &E.row[filerow];
    rowlen = row ? row->size : 0;
    if (filecol > rowlen) {
        E.cx -= filecol-rowlen;
        if (E.cx < 0) {
            E.coloff += E.cx;
            E.cx = 0;
        }
    }
}

/* Move to the beginning of the document */
void editorMoveToBeginning(void) {
    E.cx = 0;
    E.cy = 0;
    E.rowoff = 0;
    E.coloff = 0;
}

/* Move to the end of the document */
void editorMoveToEnd(void) {
    if (E.numrows == 0) return;

    int filerow = E.numrows - 1;

    /* Update cursor position */
    if (filerow >= E.rowoff + E.screenrows) {
        E.rowoff = filerow - E.screenrows + 1;
        E.cy = E.screenrows - 1;
    } else {
        E.cy = filerow - E.rowoff;
    }

    /* Move to end of last line */
    erow *row = &E.row[filerow];
    if (row->size > E.screencols - 1) {
        E.coloff = row->size - E.screencols + 1;
        E.cx = E.screencols - 1;
    } else {
        E.cx = row->size;
        E.coloff = 0;
    }
}
