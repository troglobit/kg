/* ======================== Keyboard event handling ========================= */

#include "def.h"

/* Process events arriving from the standard input, which is, the user
 * is typing stuff on the terminal. */
void editorProcessKeypress(int fd) {
    int c = editorReadKey(fd);

    /* Paste mode detection: if characters arrive very quickly (< 30ms apart),
     * we're likely in a paste operation, so disable autocompletion */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if (tv.tv_sec == E.last_char_time.tv_sec) {
        long elapsed = (tv.tv_usec - E.last_char_time.tv_usec);
        if (elapsed < 30000)
            E.paste_mode = 1; /* 30ms threshold */
    } else if (tv.tv_sec - E.last_char_time.tv_sec > 0) {
        E.paste_mode = 0;
    }
    E.last_char_time = tv;

    /* Handle C-x prefix commands */
    if (E.cx_prefix) {
        E.cx_prefix = 0;
        switch(c) {
        case CTRL_C:    /* C-x C-c: Quit */
            if (E.dirty) {
                editorSetStatusMessage("Modified buffer, really quit? (y/n) ");
                editorRefreshScreen();
                int answer = editorReadKey(fd);
                if (answer != 'y' && answer != 'Y') {
                    editorSetStatusMessage("");
                    return;
                }
            }
            running = 0;
            break;
        case CTRL_S:    /* C-x C-s: Save */
            editorSave();
            break;
        case CTRL_G:    /* C-x C-g: Cancel C-x prefix */
            editorSetStatusMessage("");
            break;
        default:
            editorSetStatusMessage("C-x %c is undefined", c);
            break;
        }
        return;
    }

    /* Regular key processing */
    switch(c) {
    case KEY_NULL:      /* Ctrl+Space - set mark */
        editorSetMark();
        break;
    case ENTER:         /* Enter */
        editorInsertNewline();
        break;
    case CTRL_A:        /* Beginning of line */
        editorMoveCursor(HOME_KEY);
        break;
    case CTRL_B:        /* Backward char */
        editorMoveCursor(ARROW_LEFT);
        break;
    case CTRL_D:        /* Delete char */
        editorDelChar();
        break;
    case CTRL_E:        /* End of line */
        editorMoveCursor(END_KEY);
        break;
    case CTRL_F:        /* Forward char */
        editorMoveCursor(ARROW_RIGHT);
        break;
    case CTRL_G:        /* Keyboard quit / cancel */
        editorSetStatusMessage("");
        break;
    case CTRL_K:        /* Kill line */
        editorKillLine();
        break;
    case CTRL_N:        /* Next line */
        editorMoveCursor(ARROW_DOWN);
        break;
    case CTRL_P:        /* Previous line */
        editorMoveCursor(ARROW_UP);
        break;
    case CTRL_S:        /* Incremental search */
        editorFind(fd);
        break;
    case CTRL_R:        /* Incremental search */
        editorFind(fd);
        break;
    case CTRL_V:        /* Page down */
        if (E.cy != E.screenrows-1)
            E.cy = E.screenrows-1;
        {
        int times = E.screenrows;
        while(times--)
            editorMoveCursor(ARROW_DOWN);
        }
        break;
    case CTRL_W:        /* Kill region (cut) */
        editorKillRegion();
        break;
    case CTRL_X:        /* C-x prefix */
        E.cx_prefix = 1;
        editorSetStatusMessage("C-x-");
        return;
    case CTRL_Y:        /* Yank (paste) */
        editorYank();
        break;
    case CTRL_UNDERSCORE: /* Undo (C-_ or C-/) */
        editorUndo();
        break;
    case BACKSPACE:     /* Backspace */
    case CTRL_H:        /* Ctrl-h */
    case DEL_KEY:
        editorDelChar();
        break;
    case PAGE_UP:
    case PAGE_DOWN:
        if (c == PAGE_UP && E.cy != 0)
            E.cy = 0;
        else if (c == PAGE_DOWN && E.cy != E.screenrows-1)
            E.cy = E.screenrows-1;
        {
        int times = E.screenrows;
        while(times--)
            editorMoveCursor(c == PAGE_UP ? ARROW_UP:
                                            ARROW_DOWN);
        }
        break;

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
    case HOME_KEY:
    case END_KEY:
        editorMoveCursor(c);
        break;
    case CTRL_HOME:
        editorMoveToBeginning();
        break;
    case CTRL_END:
        editorMoveToEnd();
        break;
    case CTRL_ARROW_LEFT:
    case ALT_B:
        editorMoveWordBackward();
        break;
    case CTRL_ARROW_RIGHT:
    case ALT_F:
        editorMoveWordForward();
        break;
    case CTRL_ARROW_UP:
        editorMoveParagraphBackward();
        break;
    case CTRL_ARROW_DOWN:
        editorMoveParagraphForward();
        break;
    case ALT_V:         /* Page up */
        if (E.cy != 0)
            E.cy = 0;
        {
        int times = E.screenrows;
        while(times--)
            editorMoveCursor(ARROW_UP);
        }
        break;
    case ALT_W:         /* Copy region */
        editorCopyRegion();
        break;
    case CTRL_L: /* ctrl+l, clear screen */
        /* Just refresh the line as side effect. */
        break;
    case ESC:
        /* Nothing to do for ESC in this mode. */
        break;
    default:
        /* Filter out control characters and non-printable characters.
         * Only allow printable ASCII (32-126) and TAB/ENTER.
         * This prevents weird characters from appearing when pressing
         * unhandled key combinations. In the future, we can add a compose
         * key for UTF-8 input. */
        if (c == TAB || c == ENTER) {
            /* Allow TAB and ENTER */
            editorInsertCharAutoComplete(c);
        } else if (c >= 32 && c < 127) {
            /* Printable ASCII characters */
            editorInsertCharAutoComplete(c);
        }
        /* Silently ignore all other control/non-printable characters */
        break;
    }
}
