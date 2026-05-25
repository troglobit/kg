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

/* Convert a `chars`-column index to its rendered (post-tab-expansion)
 * column on the same row, matching editor_update_row's expansion rule
 * (each TAB widens to the next 8-column stop). */
static int chars_to_render_col(erow *row, int chars_col)
{
	int j, idx = 0;

	if (chars_col > row->size) chars_col = row->size;
	for (j = 0; j < chars_col; j++) {
		if (row->chars[j] == TAB) {
			idx++;
			while ((idx + 1) % 8 != 0) idx++;
		} else {
			idx++;
		}
	}
	return idx;
}

/* Render the text rows of one window into ab.
 * win_y, win_x, win_h, win_w describe the window's position/size.
 * rowoff/coloff/numrows/rows describe the buffer viewport.
 * is_active: the window currently has the user's focus; only this one
 * shows the visual-mark region overlay.
 * is_full_width: if true we can use \x1b[0K (erase to EOL) to clear the
 * rest of each row; if false (vertical split) we must space-pad to stay
 * within the window's column range. */
static void draw_window_rows(struct abuf *ab,
	int win_y, int win_x, int win_h, int win_w,
	int rowoff, int coloff, int numrows, erow *rows,
	int is_active, int is_full_width)
{
	int y, j;
	int region_active = 0;
	int region_s_row = 0, region_s_col = 0;
	int region_e_row = 0, region_e_col = 0;

	if (is_active && editor.mark_highlight && editor.mark_set) {
		int p_row = editor.rowoff + editor.cy;
		int p_col = editor.coloff + editor.cx;
		int m_row = editor.mark_row;
		int m_col = editor.mark_col;
		if (editor.rect_mode) {
			/* Rectangle bounds live in VISUAL-column space.  Each
			 * row in range maps that visual range back to its own
			 * byte / render range — matches what the kill/clear
			 * operations cut, and stays rectangular across rows
			 * with different tab and UTF-8 content. */
			int p_vcol = (p_row < numrows)
				? editor_visual_col(&rows[p_row], p_col) : p_col;
			int m_vcol = (m_row < numrows)
				? editor_visual_col(&rows[m_row], m_col) : m_col;
			region_s_row = (p_row < m_row) ? p_row : m_row;
			region_e_row = (p_row > m_row) ? p_row : m_row;
			region_s_col = (p_vcol < m_vcol) ? p_vcol : m_vcol;
			region_e_col = (p_vcol > m_vcol) ? p_vcol : m_vcol;
		} else if (m_row < p_row || (m_row == p_row && m_col < p_col)) {
			region_s_row = m_row; region_s_col = m_col;
			region_e_row = p_row; region_e_col = p_col;
		} else {
			region_s_row = p_row; region_s_col = p_col;
			region_e_row = m_row; region_e_col = m_col;
		}
		region_active = (region_s_row != region_e_row ||
		                 region_s_col != region_e_col);
	}

	for (y = 0; y < win_h; y++) {
		int fr = rowoff + y;
		int current_color = -1;
		int current_reverse = 0;
		int hi_lo = -1, hi_hi = -1;   /* highlight bounds in render-col, half-open */
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

			if (region_active && fr >= region_s_row && fr <= region_e_row) {
				if (editor.rect_mode) {
					int byte_lo = editor_chars_col_at_visual(r, region_s_col);
					int byte_hi = editor_chars_col_at_visual(r, region_e_col);
					if (byte_lo > r->size) byte_lo = r->size;
					if (byte_hi > r->size) byte_hi = r->size;
					hi_lo = chars_to_render_col(r, byte_lo);
					hi_hi = chars_to_render_col(r, byte_hi);
				} else {
					hi_lo = (fr == region_s_row) ?
						chars_to_render_col(r, region_s_col) : 0;
					hi_hi = (fr == region_e_row) ?
						chars_to_render_col(r, region_e_col) : r->rsize;
				}
			}

			for (j = 0; j < len; j++) {
				int render_col = coloff + j;
				int want_rev = (render_col >= hi_lo && render_col < hi_hi);
				/* Defer the reverse-video toggle on UTF-8 continuation
				 * bytes so an escape can never split a multi-byte
				 * glyph; the next start byte will catch up. */
				if (want_rev != current_reverse && !utf8_is_cont(c[j])) {
					if (want_rev) ab_append(ab, "\x1b[7m",  4);
					else          ab_append(ab, "\x1b[27m", 5);
					current_reverse = want_rev;
				}
				if (hl[j] == HL_NONPRINT) {
					char sym;
					ab_append(ab, "\x1b[7m", 4);
					sym = (c[j] <= 26) ? ('@' + c[j]) : '?';
					ab_append(ab, &sym, 1);
					ab_append(ab, "\x1b[0m", 4);
					/* The [0m reset just cleared every attribute, so the
					 * next iteration must re-establish whatever color and
					 * reverse state it actually wants. */
					current_color = -1;
					current_reverse = 0;
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
			/* When rect mode's right edge is past this row's
			 * visual content, extend the highlight into virtual
			 * space with reverse-video spaces — so a rectangle
			 * pulled out past short rows still looks rectangular. */
			if (editor.rect_mode && region_active &&
			    fr >= region_s_row && fr <= region_e_row) {
				int row_vwidth = editor_visual_col(r, r->size);

				if (region_e_col > row_vwidth) {
					int virt_e = region_e_col - row_vwidth;
					int virt_s = region_s_col > row_vwidth
					                ? region_s_col - row_vwidth : 0;
					int skip = virt_s;
					int rev  = virt_e - virt_s;

					if (skip > 0) {
						if (current_reverse) {
							ab_append(ab, "\x1b[27m", 5);
							current_reverse = 0;
						}
						ab_append_spaces(ab, skip);
					}
					if (rev > 0) {
						if (!current_reverse) {
							ab_append(ab, "\x1b[7m", 4);
							current_reverse = 1;
						}
						ab_append_spaces(ab, rev);
					}
				}
			}
			if (current_reverse)
				ab_append(ab, "\x1b[27m", 5);
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
	char status[512];
	char bname[128];
	int len;
	struct editor_buffer *b = &buflist[bufidx];
	const char *modename = b->syntax ? b->syntax->name : "Fundamental";
	const char *changed  = "";
	int is_current = (is_active || bufidx == buf_current);
	int dirty = is_current ? editor.dirty : b->dirty;
	char pos[8];

	/* Show only the basename in the mode line (Emacs-style); the directory
	 * part is still available via C-x C-b.  buf_display_name() also
	 * prepends the parent directory when another open buffer shares the
	 * basename, so foo and dir/foo can be told apart. */
	buf_display_name(bufidx, bname, sizeof(bname));
	if (is_current ? editor.disk_changed : b->disk_changed)
		changed = " (changed)";

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

	len = snprintf(status, sizeof(status), "%s  %s%s  %s (%d,%d)  (%s)",
		dirty ? "-**-" : "----", bname, changed,
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
			rowoff, coloff, numrows, rows, is_active, is_full_width);

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
	if (msglen && time(NULL) - editor.statusmsg_time < 5) {
		/* Cap by display width, not byte count, so embedded ANSI escapes
		 * (e.g. reverse video around a selected completion) pass through
		 * intact and aren't sliced mid-sequence. */
		int p = 0, visible = 0;

		while (p < msglen && visible < win_total_cols) {
			if (editor.statusmsg[p] == '\x1b') {
				p++;
				if (p < msglen && editor.statusmsg[p] == '[') {
					p++;
					while (p < msglen &&
					       (editor.statusmsg[p] < 0x40 ||
					        editor.statusmsg[p] > 0x7e))
						p++;
					if (p < msglen) p++;   /* the final letter */
				}
				continue;
			}
			p++;
			visible++;
		}
		ab_append(&ab, editor.statusmsg, p);
		ab_append(&ab, "\x1b[0m", 4);   /* close any open attribute */
	}

	/* ---- Place cursor ---- */
	if (editor.echo_cursor_col > 0) {
		/* Minibuffer prompt active: park the cursor on the echo area so the
		 * user can see what they're typing. */
		int col = editor.echo_cursor_col;
		if (col > win_total_cols) col = win_total_cols;
		ab_move_to(&ab, win_total_rows, col);
	} else {
		struct editor_window *w = &winlist[win_current];
		erow *row = (editor.rowoff + editor.cy < editor.numrows) ? &editor.row[editor.rowoff + editor.cy] : NULL;

		cx = 1;
		if (row) {
			/* editor.cx is a byte offset into row->chars, but the cursor
			 * must be placed at the visible column.  Tabs widen to the
			 * next 8-stop; UTF-8 continuation bytes carry no width. */
			int target = editor.cx + editor.coloff;

			for (j = editor.coloff; j < target && j < row->size; j++) {
				if (row->chars[j] == TAB) cx += 7 - ((cx) % 8);
				if (!utf8_is_cont((unsigned char)row->chars[j])) cx++;
			}
			/* Past EOL — rect mode allows the cursor to live in virtual
			 * space; each virtual byte renders as one extra column. */
			if (target > row->size) cx += target - row->size;
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
