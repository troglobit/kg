/* ============================= Terminal update ============================ */

#include "def.h"

#define ABUF_INIT {NULL,0}

void ab_append(struct abuf *ab, const char *s, int len)
{
	char *new = realloc(ab->b, ab->len+len);

	if (new == NULL) return;
	memcpy(new+ab->len, s, len);
	ab->b = new;
	ab->len += len;
}

void ab_free(struct abuf *ab)
{
	free(ab->b);
}

/* Append a VT100 "move to absolute position" escape (1-based). */
static void ab_move_to(struct abuf *ab, int row, int col)
{
	char buf[32];
	int len = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", row, col);
	ab_append(ab, buf, len);
}

/* Append n copies of character c to ab in bulk. */
static void ab_fill(struct abuf *ab, char c, int n)
{
	char buf[256];
	while (n > 0) {
		int chunk = n < (int)sizeof(buf) ? n : (int)sizeof(buf);
		memset(buf, c, chunk);
		ab_append(ab, buf, chunk);
		n -= chunk;
	}
}

static void ab_append_spaces(struct abuf *ab, int n) { ab_fill(ab, ' ', n); }

/* Render the text rows of one window into ab.
 * win_y, win_x, win_h, win_w describe the window's position/size.
 * rowoff/coloff/numrows/rows describe the buffer viewport.
 * is_full_width: if true we can use \x1b[0K (erase to EOL) to clear the
 * rest of each row; if false (vertical split) we must space-pad to stay
 * within the window's column range. */
static void draw_window_rows(struct abuf *ab,
	int win_y, int win_x, int win_h, int win_w,
	int rowoff, int coloff, int numrows, erow *rows,
	int is_full_width)
{
	int y, j;

	for (y = 0; y < win_h; y++) {
		int fr = rowoff + y;
		int current_color = -1;
		int len;

		ab_move_to(ab, win_y + y, win_x);

		if (fr >= numrows) {
			int filled;
			if (numrows == 0 && y == win_h/3) {
				char welcome[80];
				int wlen = snprintf(welcome, sizeof(welcome),
					"kg editor -- version %s", KG_VERSION);
				int padding = (win_w - wlen) / 2;
				if (padding > 0) {
					ab_append(ab, "~", 1);
					padding--;
				}
				while (padding-- > 0) ab_append(ab, " ", 1);
				if (wlen > win_w) wlen = win_w;
				ab_append(ab, welcome, wlen);
				filled = win_w; /* welcome block fills the window */
			} else {
				ab_append(ab, "~", 1);
				filled = 1;
			}
			if (is_full_width)
				ab_append(ab, "\x1b[0K", 4);
			else
				ab_append_spaces(ab, win_w - filled);
			continue;
		}

		{
			erow *r = &rows[fr];
			char *c;
			unsigned char *hl;

			len = r->rsize - coloff;
			if (len < 0) len = 0;
			if (len > win_w) len = win_w;

			c  = r->render + coloff;
			hl = r->hl    + coloff;

			for (j = 0; j < len; j++) {
				if (hl[j] == HL_NONPRINT) {
					char sym;
					ab_append(ab, "\x1b[7m", 4);
					sym = (c[j] <= 26) ? ('@' + c[j]) : '?';
					ab_append(ab, &sym, 1);
					ab_append(ab, "\x1b[0m", 4);
				} else if (hl[j] == HL_NORMAL) {
					if (current_color != -1) {
						ab_append(ab, "\x1b[39m", 5);
						current_color = -1;
					}
					ab_append(ab, c+j, 1);
				} else {
					int color = editor_syntax_to_color(hl[j]);
					if (color != current_color) {
						char cbuf[16];
						int clen = snprintf(cbuf, sizeof(cbuf), "\x1b[%dm", color);
						current_color = color;
						ab_append(ab, cbuf, clen);
					}
					ab_append(ab, c+j, 1);
				}
			}
		}
		ab_append(ab, "\x1b[39m", 5);
		if (is_full_width) {
			ab_append(ab, "\x1b[0K", 4);
		} else {
			int used = rows[fr].rsize - coloff;
			if (used < 0) used = 0;
			if (used > win_w) used = win_w;
			ab_append_spaces(ab, win_w - used);
		}
	}
}

/* Render the mode line for one window at terminal row ml_row, starting at
 * terminal column win_x (1-based).  Needed for vertical splits where two mode
 * lines share the same terminal row. */
static void draw_mode_line(struct abuf *ab, int ml_row, int win_x, int win_w,
	int bufidx, int is_active, int cur_row, int cur_col, int total_rows, int rowoff, int win_h)
{
	char status[120];
	int len;
	struct editor_buffer *b = &buflist[bufidx];
	const char *fname    = b->filename ? b->filename : "[new]";
	const char *modename = b->syntax   ? b->syntax->name : "Fundamental";
	int dirty = (is_active || bufidx == buf_current) ? editor.dirty : b->dirty;
	char pos[8];

	/* Emacs-style position indicator. */
	if (total_rows <= win_h)
		snprintf(pos, sizeof(pos), "All");
	else if (rowoff == 0)
		snprintf(pos, sizeof(pos), "Top");
	else if (rowoff + win_h >= total_rows)
		snprintf(pos, sizeof(pos), "Bot");
	else
		snprintf(pos, sizeof(pos), "%d%%", rowoff * 100 / total_rows);

	ab_move_to(ab, ml_row, win_x);
	ab_append(ab, is_active ? "\x1b[7m" : "\x1b[2m", 4); /* active: reverse; inactive: dim */

	len = snprintf(status, sizeof(status), "%s  %.30s  %s (%d,%d)  (%s)",
		dirty ? "-**-" : "----", fname,
		pos, cur_row, cur_col, modename);

	if (len > win_w) len = win_w;
	ab_append(ab, status, len);
	ab_append_spaces(ab, win_w - len);
	ab_append(ab, "\x1b[0m", 4);
}

/* This function writes the whole screen using VT100 escape characters. */
void editor_refresh_screen(void)
{
	struct abuf ab = ABUF_INIT;
	int i, cx, j;
	int msglen;

	ab_append(&ab, "\x1b[?25l", 6); /* Hide cursor. */

	if (editor.show_help) {
		ab_append(&ab, "\x1b[H", 3);
		editor_draw_help(&ab, editor.screenrows);
		ab_append(&ab, "\x1b[0K", 4);
		ab_append(&ab, "\x1b[7m", 4);
		ab_append_spaces(&ab, editor.screencols);
		ab_append(&ab, "\x1b[0m\r\n", 6);
		ab_append(&ab, "\x1b[0K", 4);
		ab_append(&ab, "\x1b[H", 3);
		ab_append(&ab, "\x1b[?25h", 6);
		tty_write(ab.b, ab.len);
		ab_free(&ab);
		return;
	}

	/* ---- Render each window ---- */
	for (i = 0; i < MAX_WINDOWS; i++) {
		struct editor_window *w = &winlist[i];
		int bidx, numrows, rowoff, coloff;
		erow *rows;
		int is_active = (i == win_current);
		int is_full_width = (w->w == win_total_cols);
		int ml_row;
		struct editor_buffer *b;

		if (!w->active) continue;

		bidx = w->bufidx;
		b    = &buflist[bidx];

		if (is_active) {
			numrows = editor.numrows;
			rows    = editor.row;
			rowoff  = editor.rowoff;
			coloff  = editor.coloff;
		} else {
			/* Row data: if this window shares the active buffer, use the
			 * live editor arrays — b->row may be a stale pointer after realloc. */
			numrows = (bidx == buf_current) ? editor.numrows : b->numrows;
			rows    = (bidx == buf_current) ? editor.row     : b->row;
			/* Always use the window's own scroll offsets, not the buffer
			 * slot's (which tracks the last-active window's scroll). */
			rowoff  = w->rowoff;
			coloff  = w->coloff;
		}

		draw_window_rows(&ab, w->y, w->x, w->h, w->w,
			rowoff, coloff, numrows, rows, is_full_width);

		ml_row = w->y + w->h;
		{
			int wrowoff    = is_active ? editor.rowoff : w->rowoff;
			int cur_row    = is_active ? (editor.rowoff + editor.cy + 1) : (w->rowoff + w->cy + 1);
			int cur_col    = is_active ? (editor.cx + 1) : (w->cx + 1);
			int total_rows = (is_active || bidx == buf_current) ? editor.numrows : b->numrows;
			draw_mode_line(&ab, ml_row, w->x, w->w, bidx, is_active,
				cur_row, cur_col, total_rows, wrowoff, w->h);
		}

		/* Draw vertical separator to the right of non-rightmost windows. */
		if (w->x + w->w < win_total_cols) {
			int sep_col = w->x + w->w;
			int row;
			for (row = w->y; row < ml_row; row++) {
				ab_move_to(&ab, row, sep_col);
				ab_append(&ab, "\xe2\x94\x82", 3); /* │ */
			}
			/* Mode line row: invert to blend with the mode line. */
			ab_move_to(&ab, ml_row, sep_col);
			ab_append(&ab, "\x1b[7m\xe2\x94\x82\x1b[0m", 11);
		}
	}

	/* ---- Echo area (one row at the very bottom) ---- */
	ab_move_to(&ab, win_total_rows, 1);
	ab_append(&ab, "\x1b[0K", 4);
	msglen = strlen(editor.statusmsg);
	if (msglen && time(NULL) - editor.statusmsg_time < 5)
		ab_append(&ab, editor.statusmsg,
			msglen <= win_total_cols ? msglen : win_total_cols);

	/* ---- Place cursor in active window ---- */
	{
		struct editor_window *w = &winlist[win_current];
		erow *row = (editor.rowoff + editor.cy < editor.numrows) ? &editor.row[editor.rowoff + editor.cy] : NULL;

		cx = 1;
		if (row) {
			for (j = editor.coloff; j < (editor.cx + editor.coloff) && j < row->size; j++) {
				if (row->chars[j] == TAB) cx += 7 - ((cx) % 8);
				cx++;
			}
		}
		ab_move_to(&ab, w->y + editor.cy, w->x + cx - 1);
	}

	ab_append(&ab, "\x1b[?25h", 6); /* Show cursor. */
	tty_write(ab.b, ab.len);
	ab_free(&ab);
}

/* Set an editor status message for the echo area at the bottom. */
void editor_set_status_message(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(editor.statusmsg, sizeof(editor.statusmsg), fmt, ap);
	va_end(ap);
	editor.statusmsg_time = time(NULL);
}
