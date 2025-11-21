/* ============================ Word movement =============================== */

#include "def.h"

/* Move cursor forward by one word */
void editorMoveWordForward(void) {
    int filerow = E.rowoff + E.cy;
    int filecol = E.coloff + E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    if (!row) return;

    /* Skip current word (alphanumeric or punctuation) */
    while (filecol < row->size && !isspace(row->chars[filecol])) {
        editorMoveCursor(ARROW_RIGHT);
        filecol = E.coloff + E.cx;
    }

    /* Skip whitespace */
    while (filecol < row->size && isspace(row->chars[filecol])) {
        editorMoveCursor(ARROW_RIGHT);
        filecol = E.coloff + E.cx;
    }
}

/* Move cursor backward by one word */
void editorMoveWordBackward(void) {
    int filerow = E.rowoff + E.cy;
    int filecol = E.coloff + E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

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
