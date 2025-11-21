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

/* Move to the beginning of the previous paragraph (or beginning of buffer) */
void editorMoveParagraphBackward(void) {
    int filerow = E.rowoff + E.cy;
    erow *row;
    int found_blank = 0;

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
void editorMoveParagraphForward(void) {
    int filerow = E.rowoff + E.cy;
    erow *row;
    int found_blank = 0;

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
