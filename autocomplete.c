/* ============================ Autocompletion ============================= */

#include "def.h"

/* Define pairs of characters that should be autocompleted */
struct autopair {
    int open_char;  /* Opening character (like '{') */
    int close_char; /* Closing character (like '}') */
};

/* Array of autocomplete pairs. Can be extended with more character pairs. */
struct autopair autopairs[] = {
    {'{', '}'},
    {'[', ']'},
    {'(', ')'},
    {'\"', '\"'},
    {'\'', '\''},
    {'`', '`'},
    {'<', '>'},
};

#define AUTOPAIR_COUNT (sizeof(autopairs)/sizeof(autopairs[0]))

/* Find the matching closing character for the given opening character.
 * Returns the closing character if found, or 0 if no match exists. */
int editorFindCloseChar(int open_char) {
    for (size_t i = 0; i < AUTOPAIR_COUNT; i++) {
        if (autopairs[i].open_char == open_char) {
            return autopairs[i].close_char;
        }
    }
    return 0;
}

/* Handle character insertion with potential autocompletion.
 * Checks if the typed character has a closing pair and inserts it automatically
 * when appropriate. */
void editorInsertCharAutoComplete(int c) {
    int filerow = E.rowoff + E.cy;
    int filecol = E.coloff + E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    /* Check if we're at end of line or the next character is whitespace/symbol */
    int at_end = (!row || filecol >= row->size);
    int next_char_space = at_end || isspace(row->chars[filecol]) ||
                          strchr(",.()+-/*=~%[];{}", row->chars[filecol]);

    /* Find closing character if this is an opening bracket/quote */
    int close_char = editorFindCloseChar(c);

    /* Insert the character first */
    editorInsertChar(c);

    /* Skip autocompletion during paste operations */
    if (E.paste_mode)
        return;

    /* If this is a bracket/quote and we're either at end of line or
     * next character is whitespace/symbol, insert the closing character */
    if (close_char && next_char_space) {
        editorInsertChar(close_char);

        /* Move cursor back between the pair */
        editorMoveCursor(ARROW_LEFT);
    }
}
