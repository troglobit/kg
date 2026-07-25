/* tty.c - Low level terminal handling */

#include "def.h"

static struct termios orig_termios; /* In order to restore at exit.*/

void disable_raw_mode(int fd)
{
#ifdef KG_FUZZ
	(void)fd;
	editor.rawmode = 0;
	return;
#endif
	/* Don't even check the return value as it's too late. */
	if (editor.rawmode) {
		/* Back to the normal screen; restores the shell's scrollback. */
		tty_write("\x1b[?1049l", 8);
		tcsetattr(fd, TCSAFLUSH, &orig_termios);
		editor.rawmode = 0;
	}
}

/* Called at exit to avoid remaining in raw mode. */
void editor_at_exit(void)
{
#ifdef KG_FUZZ
	return;
#endif
	/* Clear screen and home the cursor: a fallback for terminals
	 * without alternate-screen support; on the rest disable_raw_mode()
	 * restores the shell's content anyway. */
	tty_write("\x1b[2J", 4);
	tty_write("\x1b[H", 3);

	disable_raw_mode(STDIN_FILENO);
}

/* Raw mode: 1960 magic shit. */
int enable_raw_mode(int fd)
{
#ifdef KG_FUZZ
	(void)fd;
	editor.rawmode = 1;
	return 0;
#endif
	struct termios raw;

	if (editor.rawmode) return 0; /* Already enabled. */
	if (!isatty(STDIN_FILENO)) goto fatal;
	atexit(editor_at_exit);
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
	editor.rawmode = 1;

	/* Switch to the alternate screen, like other full-screen editors.
	 * Besides preserving the shell's scrollback, this makes VTE-based
	 * terminals (gnome-terminal, ...) forward shift-modified arrow
	 * keys to the editor instead of scrolling the scrollback. */
	tty_write("\x1b[?1049h", 8);
	return 0;

fatal:
	errno = ENOTTY;
	return -1;
}

/* Decode an escape sequence (ESC byte already consumed) into a key code. */
static int parse_escape(int fd)
{
	char seq[6];

	if (read(fd, seq, 1) == 0) return ESC;   /* bare ESC */

	/* Alt+key: ESC followed by a single character */
	if (seq[0] == 'f') return ALT_F;
	if (seq[0] == 'b') return ALT_B;
	if (seq[0] == 'd') return ALT_D;
	if (seq[0] == 'g') return ALT_G;
	if (seq[0] == 'v') return ALT_V;
	if (seq[0] == 'w') return ALT_W;
	if (seq[0] == 'q') return ALT_Q;
	if (seq[0] == '\x7f' || seq[0] == '\b') return ALT_BACKSPACE;
	if (seq[0] == '%') return ALT_PCT;
	if (seq[0] == ';') return ALT_SEMICOLON;
	if (seq[0] == 'x') return ALT_X;
	if (seq[0] == '^') return ALT_CARET;
	if (seq[0] == 'u') return ALT_U;
	if (seq[0] == 'l') return ALT_L;
	if (seq[0] == 'c') return ALT_C;
	if (seq[0] == '!') return ALT_BANG;
	if (seq[0] == '|') return ALT_PIPE;
	if (seq[0] == '<') return ALT_LT;
	if (seq[0] == '>') return ALT_GT;
	if (seq[0] == '{') return ALT_LBRACE;
	if (seq[0] == '}') return ALT_RBRACE;
	if (seq[0] == 'm') return ALT_M;
	if (seq[0] == 'a') return ALT_A;
	if (seq[0] == 'e') return ALT_E;
	if (seq[0] == 'r') return ALT_R;
	if (seq[0] == 'h') return ALT_H;
	if (seq[0] == 'z') return ALT_Z;
	if (seq[0] == '\\') return ALT_BACKSLASH;
	if (seq[0] == ' ') return ALT_SPACE;
	if (seq[0] >= '0' && seq[0] <= '9') return ALT_0 + (seq[0] - '0');

	if (read(fd, seq+1, 1) == 0) return ESC;

	/* ESC [ sequences */
	if (seq[0] == '[') {
		if (seq[1] >= '0' && seq[1] <= '9') {
			if (read(fd, seq+2, 1) == 0) return ESC;
			if (seq[2] == '~') {
				switch (seq[1]) {
				case '1': return HOME_KEY;       /* VT220 Home */
				case '3': return DEL_KEY;
				case '4': return END_KEY;        /* VT220 End */
				case '5': return PAGE_UP;
				case '6': return PAGE_DOWN;
				case '7': return HOME_KEY;       /* rxvt Home */
				case '8': return END_KEY;        /* rxvt End */
				}
			} else if (seq[2] >= '0' && seq[2] <= '9') {
				/* Two-digit: ESC[<d1><d2>~ (F1=ESC[11~ .. F10=ESC[21~) */
				if (read(fd, seq+3, 1) == 0) return ESC;
				if (seq[3] == '~' && seq[1] == '1') {
					switch (seq[2]) {
					case '1': return KEY_F1;
					case '2': return KEY_F2;
					case '3': return KEY_F3;
					case '4': return KEY_F4;
					}
				}
				if (seq[3] == '~' && seq[1] == '2' && seq[2] == '1')
					return KEY_F10;
			} else if (seq[2] == ';') {
				/* ESC [ 1 ; N x  modified-key, N=2 Shift, N=5 Ctrl */
				if (read(fd, seq+3, 1) == 0) return ESC;
				if (read(fd, seq+4, 1) == 0) return ESC;
				if (seq[1] == '1' && seq[3] == '5') {
					switch (seq[4]) {
					case 'A': return CTRL_ARROW_UP;
					case 'B': return CTRL_ARROW_DOWN;
					case 'C': return CTRL_ARROW_RIGHT;
					case 'D': return CTRL_ARROW_LEFT;
					case 'H': return CTRL_HOME;
					case 'F': return CTRL_END;
					}
				} else if (seq[1] == '1' && seq[3] == '2') {
					switch (seq[4]) {
					case 'A': return SHIFT_ARROW_UP;
					case 'B': return SHIFT_ARROW_DOWN;
					case 'C': return SHIFT_ARROW_RIGHT;
					case 'D': return SHIFT_ARROW_LEFT;
					case 'H': return SHIFT_HOME;
					case 'F': return SHIFT_END;
					}
				} else if (seq[1] == '1' && seq[3] == '6') {
					switch (seq[4]) {
					case 'A': return CTRL_SHIFT_ARROW_UP;
					case 'B': return CTRL_SHIFT_ARROW_DOWN;
					case 'C': return CTRL_SHIFT_ARROW_RIGHT;
					case 'D': return CTRL_SHIFT_ARROW_LEFT;
					case 'H': return CTRL_SHIFT_HOME;
					case 'F': return CTRL_SHIFT_END;
					}
				} else if (seq[1] == '1' && seq[3] == '3') {
					switch (seq[4]) {
					case 'A': return ALT_ARROW_UP;
					case 'B': return ALT_ARROW_DOWN;
					case 'C': return ALT_ARROW_RIGHT;
					case 'D': return ALT_ARROW_LEFT;
					}
				} else if (seq[1] == '1' && seq[3] == '4') {
					switch (seq[4]) {
					case 'A': return ALT_SHIFT_ARROW_UP;
					case 'B': return ALT_SHIFT_ARROW_DOWN;
					case 'C': return ALT_SHIFT_ARROW_RIGHT;
					case 'D': return ALT_SHIFT_ARROW_LEFT;
					}
				} else if (seq[4] == '~') {
					/* ESC [ N ; M ~  modified Insert/Delete (CUA clipboard)
					 * and Shift/Ctrl+PgUp/PgDn */
					if (seq[1] == '2' && seq[3] == '2') return SHIFT_INSERT;
					if (seq[1] == '2' && seq[3] == '5') return CTRL_INSERT;
					if (seq[1] == '3' && seq[3] == '2') return SHIFT_DELETE;
					if (seq[1] == '5' && seq[3] == '2') return SHIFT_PAGE_UP;
					if (seq[1] == '6' && seq[3] == '2') return SHIFT_PAGE_DOWN;
					if (seq[1] == '5' && seq[3] == '5') return CTRL_PAGE_UP;
					if (seq[1] == '6' && seq[3] == '5') return CTRL_PAGE_DOWN;
				}
			} else if (seq[2] == '$' || seq[2] == '^' || seq[2] == '@') {
				/* rxvt keypad modifiers: $ Shift, ^ Ctrl, @ Ctrl+Shift.
				 * Combos kg has no key for (Ctrl+Shift on the Page keys,
				 * Ctrl or Ctrl+Shift on Delete/Insert) fall through to
				 * ESC. */
				switch (seq[1]) {
				case '2': /* Insert */
					if (seq[2] == '$') return SHIFT_INSERT;
					if (seq[2] == '^') return CTRL_INSERT;
					break;
				case '3': /* Delete */
					if (seq[2] == '$') return SHIFT_DELETE;
					break;
				case '5': /* Page Up */
					if (seq[2] == '$') return SHIFT_PAGE_UP;
					if (seq[2] == '^') return CTRL_PAGE_UP;
					break;
				case '6': /* Page Down */
					if (seq[2] == '$') return SHIFT_PAGE_DOWN;
					if (seq[2] == '^') return CTRL_PAGE_DOWN;
					break;
				case '7': /* Home */
					if (seq[2] == '$') return SHIFT_HOME;
					if (seq[2] == '^') return CTRL_HOME;
					return CTRL_SHIFT_HOME;
				case '8': /* End */
					if (seq[2] == '$') return SHIFT_END;
					if (seq[2] == '^') return CTRL_END;
					return CTRL_SHIFT_END;
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
			/* rxvt sends Shift+arrows as CSI lowercase letters; Ctrl
			 * uses the SS3 form (ESC O a-d) below.  It has no distinct
			 * Ctrl+Shift+arrow -- that sends the same lowercase bytes as
			 * plain Shift -- so only Shift is decoded here. */
			case 'a': return SHIFT_ARROW_UP;
			case 'b': return SHIFT_ARROW_DOWN;
			case 'c': return SHIFT_ARROW_RIGHT;
			case 'd': return SHIFT_ARROW_LEFT;
			}
		}
	/* ESC O sequences */
	} else if (seq[0] == 'O') {
		switch (seq[1]) {
		case 'H': return HOME_KEY;
		case 'F': return END_KEY;
		case 'P': return KEY_F1;
		case 'Q': return KEY_F2;
		case 'R': return KEY_F3;
		case 'S': return KEY_F4;
		/* rxvt sends Ctrl+arrows as SS3 lowercase letters. */
		case 'a': return CTRL_ARROW_UP;
		case 'b': return CTRL_ARROW_DOWN;
		case 'c': return CTRL_ARROW_RIGHT;
		case 'd': return CTRL_ARROW_LEFT;
		}
	}
	return ESC;
}

/* Read a key from the terminal in raw mode, decoding escape sequences.
 * During macro replay returns pre-recorded keys; during recording saves
 * every key so the full sequence (including sub-prompt characters) is
 * captured in one place. */
int editor_read_key(int fd)
{
	char c;
	int nread;
	int key;

	key = macro_next_key();
	if (key >= 0)
		return key;

	while ((nread = read(fd, &c, 1)) == 0 ||
	       (nread == -1 && errno == EINTR));
	if (nread == -1) {
		running = 0;
		return 0;
	}

	key = (c == ESC) ? parse_escape(fd) : (unsigned char)c;
	macro_on_key(key);
	return key;
}

/* Read a single byte from the terminal without decoding escape sequences.
 * Used by quoted-insert so that an ESC, an arrow-key prefix, or any other
 * byte the user is trying to embed literally ends up in the buffer as
 * itself rather than being interpreted as the start of a meta key. */
int editor_read_raw_byte(int fd)
{
	char c;
	int nread;
	int key;

	key = macro_next_key();
	if (key >= 0)
		return key;

	while ((nread = read(fd, &c, 1)) == 0 ||
	       (nread == -1 && errno == EINTR));
	if (nread == -1) {
		running = 0;
		return 0;
	}

	key = (unsigned char)c;
	macro_on_key(key);
	return key;
}

/* Top-level main-loop variant of editor_read_key: while waiting for the
 * next key, run the auto-revert poll on every 100 ms read timeout so
 * external file changes are noticed without requiring a keystroke.
 * Minibuffer prompts and y/n confirmations call the plain editor_read_key
 * instead so they aren't redrawn (or silently reverted) under the user. */
int editor_read_key_idle(int fd)
{
	char c;
	int nread;
	int key;

	key = macro_next_key();
	if (key >= 0)
		return key;

	while ((nread = read(fd, &c, 1)) == 0 ||
	       (nread == -1 && errno == EINTR)) {
		editor_process_pending_resize();
		if (autorevert_poll())
			editor_refresh_screen();
	}
	if (nread == -1) {
		running = 0;
		return 0;
	}

	key = (c == ESC) ? parse_escape(fd) : (unsigned char)c;
	macro_on_key(key);
	return key;
}

/* Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor is stored at *rows and *cols and 0 is returned. */
int get_cursor_position(int ifd, int ofd, int *rows, int *cols)
{
	unsigned int i = 0;
	char buf[32];

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
int get_window_size(int ifd, int ofd, int *rows, int *cols)
{
	struct winsize ws;

	if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		/* ioctl() failed. Try to query the terminal itself. */
		int orig_row, orig_col, retval;

		/* Get the initial position so we can restore it later. */
		retval = get_cursor_position(ifd, ofd, &orig_row, &orig_col);
		if (retval == -1) goto failed;

		/* Go to right/bottom margin and get position. */
		if (write(ofd, "\x1b[999C\x1b[999B", 12) != 12) goto failed;
		retval = get_cursor_position(ifd, ofd, rows, cols);
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

/* Probe terminal dimensions using ANSI escape sequences, bypassing ioctl.
 * Necessary on serial consoles where SIGWINCH is never delivered and
 * TIOCGWINSZ may return stale host-side values.
 * Saves and restores cursor position around the probe. */
void probe_window_size(void)
{
#ifdef KG_FUZZ
	return;
#endif
	int new_rows, new_cols, orig_row, orig_col;
	char seq[32];

	if (get_cursor_position(STDIN_FILENO, STDOUT_FILENO, &orig_row, &orig_col) == -1)
		return;

	/* Drive cursor to the bottom-right corner, then read back position. */
	if (write(STDOUT_FILENO, "\x1b[999B\x1b[999C", 12) != 12)
		goto restore;
	if (get_cursor_position(STDIN_FILENO, STDOUT_FILENO, &new_rows, &new_cols) == -1)
		goto restore;

	if (new_rows != win_total_rows || new_cols != win_total_cols) {
		win_total_rows = new_rows;
		win_total_cols = new_cols;
		if (win_count > 0)
			win_term_resize();
		else {
			editor.screenrows = new_rows - 2;
			editor.screencols = new_cols;
		}
	}

restore:
	snprintf(seq, sizeof(seq), "\x1b[%d;%dH", orig_row, orig_col);
	tty_write(seq, strlen(seq));
}

void update_window_size(void)
{
#ifdef KG_FUZZ
	return;
#endif
	const int max_attempts = 3;
	int new_rows, new_cols;
	int attempts = 0;

	/* Try to get window size with retry logic */
	while (attempts < max_attempts) {
		if (get_window_size(STDIN_FILENO, STDOUT_FILENO, &new_rows, &new_cols) == 0) {
			win_total_rows = new_rows;
			win_total_cols = new_cols;
			if (win_count > 0)
				win_term_resize();
			else {
				/* win_init() not yet called; set a sensible default. */
				editor.screenrows = new_rows - 2;
				editor.screencols = new_cols;
			}
			return;
		}

		attempts++;
		if (attempts < max_attempts) {
			/* Brief delay before retry (10ms) */
			usleep(10000);
		}
	}

	/* If all attempts failed, keep current dimensions and warn user */
	editor_set_status_message("Warning: failed updating window size");
}

/* Raised by handle_sig_winch, drained by editor_process_pending_resize
 * from ordinary code; a signal handler may only touch a flag like this. */
static volatile sig_atomic_t resize_pending = 0;

void handle_sig_winch(int unused __attribute__((unused)))
{
	resize_pending = 1;
}

/* Act on a pending terminal resize outside the signal handler: adapt the
 * window layout (which clamps cursors) and redraw.  Called from the main
 * loop and while idling for input, so a resize shows within a read timeout
 * without doing async-signal-unsafe work in the handler. */
void editor_process_pending_resize(void)
{
	if (!resize_pending)
		return;
	resize_pending = 0;
	update_window_size();
	editor_refresh_screen();
}

/* Suspend the editor (C-z): restore terminal, stop the process, then
 * re-enable raw mode and redraw when resumed with fg. */
void editor_suspend(void)
{
#ifdef KG_FUZZ
	return;
#endif
	tty_write("\x1b[2J\x1b[H", 7); /* fallback for non-alt-screen terminals */
	disable_raw_mode(STDIN_FILENO); /* also leaves the alternate screen */
	raise(SIGTSTP);
	/* Execution resumes here when the shell sends SIGCONT (fg). */
	enable_raw_mode(STDIN_FILENO);
	update_window_size();
	editor_refresh_screen();
}
