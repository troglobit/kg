/* ============================= Terminal update ============================ */

#include "def.h"

#define ABUF_INIT {NULL,0}

void abAppend(struct abuf *ab, const char *s, int len)
{
	char *new = realloc(ab->b, ab->len+len);

	if (new == NULL) return;
	memcpy(new+ab->len, s, len);
	ab->b = new;
	ab->len += len;
}

void abFree(struct abuf *ab)
{
	free(ab->b);
}

/* Append a VT100 "move to absolute position" escape (1-based). */
static void abMoveTo(struct abuf *ab, int row, int col)
{
	char buf[32];
	int len = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", row, col);
	abAppend(ab, buf, len);
}

/* Append n copies of character c to ab in bulk. */
static void abFill(struct abuf *ab, char c, int n)
{
	char buf[256];
	while (n > 0) {
		int chunk = n < (int)sizeof(buf) ? n : (int)sizeof(buf);
		memset(buf, c, chunk);
		abAppend(ab, buf, chunk);
		n -= chunk;
	}
}

static void abAppendSpaces(struct abuf *ab, int n) { abFill(ab, ' ', n); }

/* Render the text rows of one window into ab.
 * win_y, win_x, win_h, win_w describe the window's position/size.
 * rowoff/coloff/numrows/rows describe the buffer viewport.
 * is_full_width: if true we can use \x1b[0K (erase to EOL) to clear the
 * rest of each row; if false (vertical split) we must space-pad to stay
 * within the window's column range. */
static void drawWindowRows(struct abuf *ab,
	int win_y, int win_x, int win_h, int win_w,
	int rowoff, int coloff, int numrows, erow *rows,
	int is_full_width)
{
	int y, j;

	for (y = 0; y < win_h; y++) {
		int fr = rowoff + y;
		int current_color = -1;
		int len;

		abMoveTo(ab, win_y + y, win_x);

		if (fr >= numrows) {
			int filled;
			if (numrows == 0 && y == win_h/3) {
				char welcome[80];
				int wlen = snprintf(welcome, sizeof(welcome),
					"kg editor -- version %s", KG_VERSION);
				int padding = (win_w - wlen) / 2;
				if (padding > 0) {
					abAppend(ab, "~", 1);
					padding--;
				}
				while (padding-- > 0) abAppend(ab, " ", 1);
				if (wlen > win_w) wlen = win_w;
				abAppend(ab, welcome, wlen);
				filled = win_w; /* welcome block fills the window */
			} else {
				abAppend(ab, "~", 1);
				filled = 1;
			}
			if (is_full_width)
				abAppend(ab, "\x1b[0K", 4);
			else
				abAppendSpaces(ab, win_w - filled);
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
					abAppend(ab, "\x1b[7m", 4);
					sym = (c[j] <= 26) ? ('@' + c[j]) : '?';
					abAppend(ab, &sym, 1);
					abAppend(ab, "\x1b[0m", 4);
				} else if (hl[j] == HL_NORMAL) {
					if (current_color != -1) {
						abAppend(ab, "\x1b[39m", 5);
						current_color = -1;
					}
					abAppend(ab, c+j, 1);
				} else {
					int color = editorSyntaxToColor(hl[j]);
					if (color != current_color) {
						char cbuf[16];
						int clen = snprintf(cbuf, sizeof(cbuf), "\x1b[%dm", color);
						current_color = color;
						abAppend(ab, cbuf, clen);
					}
					abAppend(ab, c+j, 1);
				}
			}
		}
		abAppend(ab, "\x1b[39m", 5);
		if (is_full_width) {
			abAppend(ab, "\x1b[0K", 4);
		} else {
			int used = rows[fr].rsize - coloff;
			if (used < 0) used = 0;
			if (used > win_w) used = win_w;
			abAppendSpaces(ab, win_w - used);
		}
	}
}

/* Render the mode line for one window at terminal row ml_row, starting at
 * terminal column win_x (1-based).  Needed for vertical splits where two mode
 * lines share the same terminal row. */
static void drawModeLine(struct abuf *ab, int ml_row, int win_x, int win_w,
	int bufidx, int is_active, int cur_row, int cur_col, int total_rows, int rowoff, int win_h)
{
	char status[120];
	int len;
	struct editorBuffer *b = &buflist[bufidx];
	const char *fname    = b->filename ? b->filename : "[new]";
	const char *modename = b->syntax   ? b->syntax->name : "Fundamental";
	int dirty = (is_active || bufidx == buf_current) ? E.dirty : b->dirty;
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

	abMoveTo(ab, ml_row, win_x);
	abAppend(ab, is_active ? "\x1b[7m" : "\x1b[2m", 4); /* active: reverse; inactive: dim */

	len = snprintf(status, sizeof(status), "%s  %.30s  %s (%d,%d)  (%s)",
		dirty ? "-**-" : "----", fname,
		pos, cur_row, cur_col, modename);

	if (len > win_w) len = win_w;
	abAppend(ab, status, len);
	abAppendSpaces(ab, win_w - len);
	abAppend(ab, "\x1b[0m", 4);
}

/* This function writes the whole screen using VT100 escape characters. */
void editorRefreshScreen(void)
{
	struct abuf ab = ABUF_INIT;
	int i, cx, j;
	int msglen;

	abAppend(&ab, "\x1b[?25l", 6); /* Hide cursor. */

	if (E.show_help) {
		abAppend(&ab, "\x1b[H", 3);
		editorDrawHelp(&ab, E.screenrows);
		abAppend(&ab, "\x1b[0K", 4);
		abAppend(&ab, "\x1b[7m", 4);
		abAppendSpaces(&ab, E.screencols);
		abAppend(&ab, "\x1b[0m\r\n", 6);
		abAppend(&ab, "\x1b[0K", 4);
		abAppend(&ab, "\x1b[H", 3);
		abAppend(&ab, "\x1b[?25h", 6);
		write(STDOUT_FILENO, ab.b, ab.len);
		abFree(&ab);
		return;
	}

	/* ---- Render each window ---- */
	for (i = 0; i < MAX_WINDOWS; i++) {
		struct editorWindow *w = &winlist[i];
		int bidx, numrows, rowoff, coloff;
		erow *rows;
		int is_active = (i == win_current);
		int is_full_width = (w->w == win_total_cols);
		int ml_row;
		struct editorBuffer *b;

		if (!w->active) continue;

		bidx = w->bufidx;
		b    = &buflist[bidx];

		if (is_active) {
			numrows = E.numrows;
			rows    = E.row;
			rowoff  = E.rowoff;
			coloff  = E.coloff;
		} else {
			/* Row data: if this window shares the active buffer, use the
			 * live E arrays — b->row may be a stale pointer after realloc. */
			numrows = (bidx == buf_current) ? E.numrows : b->numrows;
			rows    = (bidx == buf_current) ? E.row     : b->row;
			/* Always use the window's own scroll offsets, not the buffer
			 * slot's (which tracks the last-active window's scroll). */
			rowoff  = w->rowoff;
			coloff  = w->coloff;
		}

		drawWindowRows(&ab, w->y, w->x, w->h, w->w,
			rowoff, coloff, numrows, rows, is_full_width);

		ml_row = w->y + w->h;
		{
			int wrowoff    = is_active ? E.rowoff : w->rowoff;
			int cur_row    = is_active ? (E.rowoff + E.cy + 1) : (w->rowoff + w->cy + 1);
			int cur_col    = is_active ? (E.cx + 1) : (w->cx + 1);
			int total_rows = (is_active || bidx == buf_current) ? E.numrows : b->numrows;
			drawModeLine(&ab, ml_row, w->x, w->w, bidx, is_active,
				cur_row, cur_col, total_rows, wrowoff, w->h);
		}

		/* Draw vertical separator to the right of non-rightmost windows. */
		if (w->x + w->w < win_total_cols) {
			int sep_col = w->x + w->w;
			int row;
			for (row = w->y; row < ml_row; row++) {
				abMoveTo(&ab, row, sep_col);
				abAppend(&ab, "\xe2\x94\x82", 3); /* │ */
			}
			/* Mode line row: invert to blend with the mode line. */
			abMoveTo(&ab, ml_row, sep_col);
			abAppend(&ab, "\x1b[7m\xe2\x94\x82\x1b[0m", 11);
		}
	}

	/* ---- Echo area (one row at the very bottom) ---- */
	abMoveTo(&ab, win_total_rows, 1);
	abAppend(&ab, "\x1b[0K", 4);
	msglen = strlen(E.statusmsg);
	if (msglen && time(NULL) - E.statusmsg_time < 5)
		abAppend(&ab, E.statusmsg,
			msglen <= win_total_cols ? msglen : win_total_cols);

	/* ---- Place cursor in active window ---- */
	{
		struct editorWindow *w = &winlist[win_current];
		erow *row = (E.rowoff + E.cy < E.numrows) ? &E.row[E.rowoff + E.cy] : NULL;

		cx = 1;
		if (row) {
			for (j = E.coloff; j < (E.cx + E.coloff) && j < row->size; j++) {
				if (row->chars[j] == TAB) cx += 7 - ((cx) % 8);
				cx++;
			}
		}
		abMoveTo(&ab, w->y + E.cy, w->x + cx - 1);
	}

	abAppend(&ab, "\x1b[?25h", 6); /* Show cursor. */
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

/* Set an editor status message for the echo area at the bottom. */
void editorSetStatusMessage(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
	va_end(ap);
	E.statusmsg_time = time(NULL);
}
