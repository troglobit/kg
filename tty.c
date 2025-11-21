/* tty.c - Low level terminal handling */

#include "def.h"

static struct termios orig_termios; /* In order to restore at exit.*/

void disableRawMode(int fd) {
	/* Don't even check the return value as it's too late. */
	if (E.rawmode) {
		tcsetattr(fd, TCSAFLUSH, &orig_termios);
		E.rawmode = 0;
	}
}

/* Called at exit to avoid remaining in raw mode. */
void editorAtExit(void) {
	/* Clear screen and reset cursor position before exiting. */
	write(STDOUT_FILENO, "\x1b[2J", 4);  /* Clear entire screen */
	write(STDOUT_FILENO, "\x1b[H", 3);   /* Move cursor to top-left */

	disableRawMode(STDIN_FILENO);
}

/* Raw mode: 1960 magic shit. */
int enableRawMode(int fd) {
	struct termios raw;

	if (E.rawmode) return 0; /* Already enabled. */
	if (!isatty(STDIN_FILENO)) goto fatal;
	atexit(editorAtExit);
	if (tcgetattr(fd, &orig_termios) == -1) goto fatal;

	raw = orig_termios;  /* modify the original mode */
	/* input modes: no break, no CR to NL, no parity check, no strip char,
	 * no start/stop output control. */
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	/* output modes - disable post processing */
	raw.c_oflag &= ~(OPOST);
	/* control modes - set 8 bit chars */
	raw.c_cflag |= (CS8);
	/* local modes - choing off, canonical off, no extended functions,
	 * no signal chars (^Z,^C) */
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	/* control chars - set return condition: min number of bytes and timer. */
	raw.c_cc[VMIN] = 0; /* Return each byte, or zero for timeout. */
	raw.c_cc[VTIME] = 1; /* 100 ms timeout (unit is tens of second). */

	/* put terminal in raw mode after flushing */
	if (tcsetattr(fd, TCSAFLUSH, &raw) < 0) goto fatal;
	E.rawmode = 1;
	return 0;

fatal:
	errno = ENOTTY;
	return -1;
}

/* Read a key from the terminal put in raw mode, trying to handle
 * escape sequences. */
int editorReadKey(int fd) {
	int nread;
	char c, seq[6];

	while ((nread = read(fd, &c, 1)) == 0);
	if (nread == -1) {
		running = 0;
		return 0;
	}

	while (1) {
		switch (c) {
		case ESC:    /* escape sequence */
			/* If this is just an ESC, we'll timeout here. */
			if (read(fd, seq, 1) == 0) return ESC;

			/* Alt+key sequences (ESC followed by a character). */
			if (seq[0] == 'f') return ALT_F;
			if (seq[0] == 'b') return ALT_B;
			if (seq[0] == 'v') return ALT_V;

			if (read(fd, seq+1, 1) == 0) return ESC;

			/* ESC [ sequences. */
			if (seq[0] == '[') {
				if (seq[1] >= '0' && seq[1] <= '9') {
					/* Extended escape, read additional byte. */
					if (read(fd, seq+2, 1) == 0) return ESC;
					if (seq[2] == '~') {
						switch (seq[1]) {
						case '3': return DEL_KEY;
						case '5': return PAGE_UP;
						case '6': return PAGE_DOWN;
						}
					} else if (seq[2] == ';') {
						/* ESC [ 1 ; modifier key sequences (Ctrl+Arrow/Home/End) */
						if (read(fd, seq+3, 1) == 0) return ESC;
						if (read(fd, seq+4, 1) == 0) return ESC;
						if (seq[1] == '1' && seq[3] == '5') {
							/* Ctrl modifier */
							switch (seq[4]) {
							case 'A': return CTRL_ARROW_UP;
							case 'B': return CTRL_ARROW_DOWN;
							case 'C': return CTRL_ARROW_RIGHT;
							case 'D': return CTRL_ARROW_LEFT;
							case 'H': return CTRL_HOME;
							case 'F': return CTRL_END;
							}
						}
					}
				} else {
					switch (seq[1]) {
					case 'A': return ARROW_UP;
					case 'B': return ARROW_DOWN;
					case 'C': return ARROW_RIGHT;
					case 'D': return ARROW_LEFT;
					case 'H': return HOME_KEY;
					case 'F': return END_KEY;
					}
				}
			}

			/* ESC O sequences. */
			else if (seq[0] == 'O') {
				switch (seq[1]) {
				case 'H': return HOME_KEY;
				case 'F': return END_KEY;
				}
			}
			break;
		default:
			return c;
		}
	}
}

/* Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor is stored at *rows and *cols and 0 is returned. */
int getCursorPosition(int ifd, int ofd, int *rows, int *cols) {
	char buf[32];
	unsigned int i = 0;

	/* Report cursor location */
	if (write(ofd, "\x1b[6n", 4) != 4) return -1;

	/* Read the response: ESC [ rows ; cols R */
	while (i < sizeof(buf)-1) {
		if (read(ifd, buf+i, 1) != 1) break;
		if (buf[i] == 'R') break;
		i++;
	}
	buf[i] = '\0';

	/* Parse it. */
	if (buf[0] != ESC || buf[1] != '[') return -1;
	if (sscanf(buf+2, "%d;%d", rows, cols) != 2) return -1;

	return 0;
}

/* Try to get the number of columns in the current terminal. If the ioctl()
 * call fails the function will try to query the terminal itself.
 * Returns 0 on success, -1 on error. */
int getWindowSize(int ifd, int ofd, int *rows, int *cols) {
	struct winsize ws;

	if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		/* ioctl() failed. Try to query the terminal itself. */
		int orig_row, orig_col, retval;

		/* Get the initial position so we can restore it later. */
		retval = getCursorPosition(ifd, ofd, &orig_row, &orig_col);
		if (retval == -1) goto failed;

		/* Go to right/bottom margin and get position. */
		if (write(ofd, "\x1b[999C\x1b[999B", 12) != 12) goto failed;
		retval = getCursorPosition(ifd, ofd, rows, cols);
		if (retval == -1) goto failed;

		/* Restore position. */
		char seq[32];
		snprintf(seq, 32, "\x1b[%d;%dH", orig_row, orig_col);
		if (write(ofd, seq, strlen(seq)) == -1) {
			/* Can't recover... */
		}
		return 0;
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}

failed:
	return -1;
}

void updateWindowSize(void) {
	int new_rows, new_cols;
	int attempts = 0;
	const int max_attempts = 3;

	/* Try to get window size with retry logic */
	while (attempts < max_attempts) {
		if (getWindowSize(STDIN_FILENO, STDOUT_FILENO, &new_rows, &new_cols) == 0) {
			/* Success - update the screen dimensions */
			E.screenrows = new_rows - 2; /* Get room for status bar. */
			E.screencols = new_cols;
			return;
		}

		attempts++;
		if (attempts < max_attempts) {
			/* Brief delay before retry (10ms) */
			usleep(10000);
		}
	}

	/* If all attempts failed, keep current dimensions and warn user */
	editorSetStatusMessage("Warning: failed updating window size");
}

void handleSigWinCh(int unused __attribute__((unused))) {
	updateWindowSize();

	/* Ensure cursor position is within new screen bounds */
	if (E.cy >= E.screenrows)
		E.cy = E.screenrows - 1;
	if (E.cx >= E.screencols)
		E.cx = E.screencols - 1;

	/* Adjust offsets if necessary */
	if (E.rowoff + E.cy >= E.numrows) {
		E.rowoff = E.numrows - E.cy - 1;
		if (E.rowoff < 0)
			E.rowoff = 0;
	}

	if (E.coloff + E.cx >= E.screencols) {
		E.coloff = 0;
		E.cx = E.screencols - 1;
	}

	editorRefreshScreen();
}
