/* ========================= Window management ============================== */

#include "def.h"

struct editor_window winlist[MAX_WINDOWS];
int win_current    = 0;
int win_count      = 0;
int win_total_rows = 0;
int win_total_cols = 0;

/* win_moveseam() refuses a resize shrinking a window below these; the
 * terminal-shrink path squeezes down to one text row and two columns
 * before giving up and going single-window. */
#define MIN_WIN_ROWS 2
#define MIN_WIN_COLS 6

/* Every window owns its rectangle, in 1-based screen coordinates:
 * text rows y .. y+h-1 with the mode line at y+h, columns x .. x+w-1
 * with a divider column at x+w when another window sits to the right.
 * The echo area is the last terminal row, so the bottom-most mode
 * lines sit at SCREEN_BOT and the right-most window edge at
 * SCREEN_RIGHT. */
#define SCREEN_BOT   (win_total_rows - 1)
#define SCREEN_RIGHT (win_total_cols + 1)

/* Save the active editor cursor/scroll state into the current window slot and
 * also into its buffer slot so switching buffers later is consistent. */
void win_save_active_view(void)
{
	struct editor_window *w = &winlist[win_current];

	w->cx     = editor.cx;
	w->cy     = editor.cy;
	w->rowoff = editor.rowoff;
	w->coloff = editor.coloff;

	/* Keep buflist in sync so a buffer switch restores correctly. */
	if (w->bufidx < MAX_BUFFERS && buflist[w->bufidx].active) {
		buflist[w->bufidx].cx     = editor.cx;
		buflist[w->bufidx].cy     = editor.cy;
		buflist[w->bufidx].rowoff = editor.rowoff;
		buflist[w->bufidx].coloff = editor.coloff;
	}
}

/* Load the buffer associated with win_current into live editor state and restore
 * the window's cursor/scroll.  Called after win_current changes. */
static void win_activate_window(void)
{
	struct editor_buffer *b;
	buf_current = winlist[win_current].bufidx;
	b = &buflist[buf_current];
	editor.numrows  = b->numrows;
	editor.row      = b->row;
	editor.dirty    = b->dirty;
	editor.filename = b->filename;
	editor.syntax   = b->syntax;
	editor.mark_set = b->mark_set;
	editor.mark_row = b->mark_row;
	editor.mark_col = b->mark_col;
	undostack  = b->undostack;
	win_restore_active_view();
}

/* Restore the active window's cursor/scroll into editor and update editor.screenrows/cols
 * to match the window dimensions so all movement code stays correct. */
void win_restore_active_view(void)
{
	struct editor_window *w = &winlist[win_current];

	editor.cx         = w->cx;
	editor.cy         = w->cy;
	editor.rowoff     = w->rowoff;
	editor.coloff     = w->coloff;
	editor.screenrows = w->h;
	editor.screencols = w->w;
}

/* Push the current window's size into the live editor view and clamp
 * the cursor to the new bounds. */
static void win_sync_view(void)
{
	editor.screenrows = winlist[win_current].h;
	editor.screencols = winlist[win_current].w;

	if (editor.cy >= editor.screenrows) editor.cy = editor.screenrows - 1;
	if (editor.cx >= editor.screencols) editor.cx = editor.screencols - 1;
	if (editor.cy < 0) editor.cy = 0;
	if (editor.cx < 0) editor.cx = 0;
}

/* Give the window the whole screen above the echo area. */
static void win_fill_screen(struct editor_window *w)
{
	w->y = 1;
	w->x = 1;
	w->h = win_total_rows > 3 ? win_total_rows - 2 : 1;
	w->w = win_total_cols;
}

/* Initialise the window list with a single window covering the whole screen.
 * Called once from init_editor() after update_window_size(). */
void win_init(void)
{
	memset(winlist, 0, sizeof(winlist));
	win_current = 0;
	win_count   = 1;

	winlist[0].bufidx = 0;
	winlist[0].active = 1;
	win_fill_screen(&winlist[0]);

	win_sync_view();
}

/* First inactive winlist slot, or -1 when the list is full. */
static int win_free_slot(void)
{
	int i;

	for (i = 0; i < MAX_WINDOWS; i++)
		if (!winlist[i].active)
			return i;
	return -1;
}

/* Split the current window (C-x 2): it keeps the upper half, a new
 * window showing the same buffer takes the lower half.  Other windows
 * are not disturbed. */
void win_split_horizontal(void)
{
	struct editor_window *cur = &winlist[win_current];
	int slot, h = cur->h;

	if (win_count >= MAX_WINDOWS) {
		editor_set_status_message("Too many windows (%d max).", MAX_WINDOWS);
		return;
	}
	if (h < 2 * MIN_WIN_ROWS + 1) {
		editor_set_status_message("Window too small to split.");
		return;
	}
	slot = win_free_slot();

	buf_save_current_state();

	/* New window inherits the same buffer and cursor state. */
	winlist[slot]   = *cur;
	cur->h          = (h - 1) / 2;
	winlist[slot].y = cur->y + cur->h + 1;
	winlist[slot].h = h - 1 - cur->h;

	win_count++;
	win_sync_view();
}

/* Split the current window side by side (C-x 3): it keeps the left
 * half, a new window showing the same buffer appears to the right of
 * a one column divider.  Other windows are not disturbed. */
void win_split_vertical(void)
{
	struct editor_window *cur = &winlist[win_current];
	int slot, w = cur->w;

	if (win_count >= MAX_WINDOWS) {
		editor_set_status_message("Too many windows (%d max).", MAX_WINDOWS);
		return;
	}
	if (w < 2 * MIN_WIN_COLS + 1) {
		editor_set_status_message("Window too small to split.");
		return;
	}
	slot = win_free_slot();

	buf_save_current_state();

	winlist[slot]   = *cur;
	cur->w          = (w - 1) / 2;
	winlist[slot].x = cur->x + cur->w + 1;
	winlist[slot].w = w - 1 - cur->w;

	win_count++;
	win_sync_view();
}

/* Switch focus to window `idx`: save the old view, load the new one,
 * and echo the buffer name. */
static void win_focus(int idx)
{
	buf_save_current_state();
	win_current = idx;
	win_activate_window();
	editor_set_status_message("%s", editor.filename ? editor.filename : "[new]");
}

/* Switch focus to the next window (C-x o). */
void win_cycle_next(void)
{
	int i;

	if (win_count <= 1) {
		editor_set_status_message("No other windows.");
		return;
	}

	for (i = 1; i <= MAX_WINDOWS; i++) {
		int idx = (win_current + i) % MAX_WINDOWS;
		if (winlist[idx].active) { win_focus(idx); break; }
	}
}

/* Find the window beside the current one: the neighbor sharing the
 * edge in the given direction, with overlapping rows or columns.
 * Returns its winlist index, or -1 when there is none. */
static int win_find_dir(int dx, int dy)
{
	struct editor_window *cur = &winlist[win_current];
	int i;

	for (i = 0; i < MAX_WINDOWS; i++) {
		struct editor_window *w = &winlist[i];

		if (!w->active || i == win_current)
			continue;
		if (dx > 0 && w->x != cur->x + cur->w + 1)
			continue;
		if (dx < 0 && cur->x != w->x + w->w + 1)
			continue;
		if (dy > 0 && w->y != cur->y + cur->h + 1)
			continue;
		if (dy < 0 && cur->y != w->y + w->h + 1)
			continue;
		/* text rows or columns must overlap, edges do not count */
		if (dx != 0 && (w->y >= cur->y + cur->h || cur->y >= w->y + w->h))
			continue;
		if (dy != 0 && (w->x >= cur->x + cur->w || cur->x >= w->x + w->w))
			continue;

		return i;
	}

	return -1;
}

/* Switch focus to the window beside the current one (M-arrow), like
 * windmove in GNU Emacs. */
void win_move_dir(int dx, int dy)
{
	int i = win_find_dir(dx, dy);

	if (i < 0) {
		editor_set_status_message("No window there.");
		return;
	}
	win_focus(i);
}

/* Move the window divider at the given column, or with `cols` false
 * the mode line at the given row, by delta: windows ending on it grow
 * or shrink, windows starting just past it follow.  Refuses, without
 * changing anything, when a window would come out below the minimum
 * size. */
static int win_moveseam(int seam, int delta, int cols)
{
	int min = cols ? MIN_WIN_COLS : MIN_WIN_ROWS;
	int i;

	if (seam <= 1 || seam >= (cols ? SCREEN_RIGHT : SCREEN_BOT))
		return 0;	/* the screen edge, not a divider */

	for (i = 0; i < MAX_WINDOWS; i++) {
		struct editor_window *p = &winlist[i];
		int size, end, start;

		if (!p->active)
			continue;
		size  = cols ? p->w : p->h;
		end   = cols ? p->x + p->w : p->y + p->h;
		start = cols ? p->x : p->y;
		if (end == seam && size + delta < min)
			return 0;
		if (start == seam + 1 && size - delta < min)
			return 0;
	}

	for (i = 0; i < MAX_WINDOWS; i++) {
		struct editor_window *p = &winlist[i];

		if (!p->active)
			continue;
		if ((cols ? p->x + p->w : p->y + p->h) == seam) {
			if (cols)
				p->w += delta;
			else
				p->h += delta;
		} else if ((cols ? p->x : p->y) == seam + 1) {
			if (cols) {
				p->x += delta;
				p->w -= delta;
			} else {
				p->y += delta;
				p->h -= delta;
			}
		}
	}

	win_sync_view();
	return 1;
}

/* Resize the current window with Meta, Shift and an arrow key: the
 * edge on that side of the window travels with the arrow when it is
 * a divider, otherwise the opposite one, so the arrow and the divider
 * always move together.  `n` is the repeat count. */
void win_resize_dir(int dx, int dy, int n)
{
	struct editor_window *cur = &winlist[win_current];
	int cols  = dx != 0;
	int d     = cols ? dx : dy;
	int lo    = cols ? cur->x - 1 : cur->y - 1;
	int hi    = cols ? cur->x + cur->w : cur->y + cur->h;
	int limit = cols ? SCREEN_RIGHT : SCREEN_BOT;
	int seam  = d > 0 ? hi : lo;

	if (seam <= 1 || seam >= limit)
		seam = d > 0 ? lo : hi;
	if (seam <= 1 || seam >= limit) {
		editor_set_status_message(cols ? "No window beside."
		                               : "No window above or below.");
		return;
	}
	if (!win_moveseam(seam, d * n, cols))
		editor_set_status_message("Impossible change.");
}

/* Grow the current window by `n` text rows (C-x ^), like
 * enlarge-window in GNU Emacs: the mode line below travels, or the
 * one above when the window is bottom-most.  Negative n shrinks. */
void win_enlarge_v(int n)
{
	struct editor_window *cur = &winlist[win_current];
	int seam = cur->y + cur->h;

	if (seam >= SCREEN_BOT) {
		seam = cur->y - 1;
		n = -n;
	}
	if (seam <= 1) {
		editor_set_status_message("No window above or below.");
		return;
	}
	if (!win_moveseam(seam, n, 0))
		editor_set_status_message("Impossible change.");
}

/* Widen the current window by `n` columns (C-x }), like
 * enlarge-window-horizontally in GNU Emacs.  The right-most window
 * widens leftward.  Negative n narrows (C-x {). */
void win_enlarge_h(int n)
{
	struct editor_window *cur = &winlist[win_current];
	int seam = cur->x + cur->w;

	if (seam >= SCREEN_RIGHT) {
		seam = cur->x - 1;
		n = -n;
	}
	if (seam <= 1) {
		editor_set_status_message("No window beside.");
		return;
	}
	if (!win_moveseam(seam, n, 1))
		editor_set_status_message("Impossible change.");
}

/* Is the window's band, mode line and divider included, fully inside
 * the rectangle? */
static int balin(const struct editor_window *p, int top, int bot,
                 int left, int right)
{
	return p->y >= top && p->y + p->h <= bot &&
	       p->x >= left && p->x + p->w <= right;
}

/* Find the lowest cut through the rectangle: a mode line row (or,
 * with `rows` false, a divider column) that no window straddles.
 * Returns 0 when there is none. */
static int balcut(int top, int bot, int left, int right, int rows)
{
	int i, j, e, best = 0;

	for (i = 0; i < MAX_WINDOWS; i++) {
		const struct editor_window *p = &winlist[i];

		if (!p->active || !balin(p, top, bot, left, right))
			continue;
		e = rows ? p->y + p->h : p->x + p->w;
		if (e >= (rows ? bot : right))
			continue;
		for (j = 0; j < MAX_WINDOWS; j++) {
			const struct editor_window *q = &winlist[j];

			if (!q->active || !balin(q, top, bot, left, right))
				continue;
			if (rows ? q->y <= e && q->y + q->h > e
			         : q->x <= e && q->x + q->w > e)
				break;
		}
		if (j == MAX_WINDOWS && (best == 0 || e < best))
			best = e;
	}
	return best;
}

/* The number of windows in the tallest stack in the rectangle, or,
 * with `rows` false, in the widest row.  Returns zero for a layout
 * the cuts cannot take apart. */
static int balweight(int top, int bot, int left, int right, int rows)
{
	int i, e, a, b;

	if ((e = balcut(top, bot, left, right, 1)) != 0) {
		a = balweight(top, e, left, right, rows);
		b = balweight(e + 1, bot, left, right, rows);
		return a == 0 || b == 0 ? 0 : rows ? a + b : a > b ? a : b;
	}
	if ((e = balcut(top, bot, left, right, 0)) != 0) {
		a = balweight(top, bot, left, e, rows);
		b = balweight(top, bot, e + 1, right, rows);
		return a == 0 || b == 0 ? 0 : rows ? (a > b ? a : b) : a + b;
	}
	a = 0;
	for (i = 0; i < MAX_WINDOWS; i++)
		if (winlist[i].active &&
		    balin(&winlist[i], top, bot, left, right))
			a++;
	return a == 1;
}

/* Retile the windows of the old rectangle into the new one.  The
 * sections at a cut get room in proportion to their tallest stack,
 * or widest row, so the window sizes come out even on both axes.
 * New geometry lands in balgeo[], not winlist[]: the cut and weight
 * scans judge the windows still to be placed by their old rows, so
 * winlist[] must stay untouched until every window has a place. */
static struct editor_window balgeo[MAX_WINDOWS];

static void balassign(int ot, int ob, int ol, int orr,
                      int nt, int nb, int nl, int nr)
{
	int i, e, w1, w2, c;

	if ((e = balcut(ot, ob, ol, orr, 1)) != 0) {
		w1 = balweight(ot, e, ol, orr, 1);
		w2 = balweight(e + 1, ob, ol, orr, 1);
		if (w1 <= 0 || w2 <= 0)
			return;		/* win_balance() refused these */
		c = nt - 1 + (nb - nt + 1) * w1 / (w1 + w2);
		balassign(ot, e, ol, orr, nt, c, nl, nr);
		balassign(e + 1, ob, ol, orr, c + 1, nb, nl, nr);
		return;
	}
	if ((e = balcut(ot, ob, ol, orr, 0)) != 0) {
		w1 = balweight(ot, ob, ol, e, 0);
		w2 = balweight(ot, ob, e + 1, orr, 0);
		if (w1 <= 0 || w2 <= 0)
			return;		/* win_balance() refused these */
		c = nl - 1 + (nr - nl + 1) * w1 / (w1 + w2);
		balassign(ot, ob, ol, e, nt, nb, nl, c);
		balassign(ot, ob, e + 1, orr, nt, nb, c + 1, nr);
		return;
	}
	for (i = 0; i < MAX_WINDOWS; i++) {
		struct editor_window *p = &winlist[i];

		if (!p->active || !balin(p, ot, ob, ol, orr))
			continue;
		balgeo[i].y = nt;
		balgeo[i].h = nb - nt;
		balgeo[i].x = nl;
		balgeo[i].w = nr - nl;
	}
}

/* Even out the window sizes (C-x +), like balance-windows in GNU
 * Emacs: heights and widths alike, in any layout the mode line and
 * divider cuts can take apart. */
void win_balance(void)
{
	int w;

	if (win_count <= 1)
		return;
	if ((w = balweight(1, SCREEN_BOT, 1, SCREEN_RIGHT, 1)) == 0) {
		editor_set_status_message("Cannot balance this window layout.");
		return;
	}
	if (SCREEN_BOT < 2 * w ||
	    SCREEN_RIGHT < 3 * balweight(1, SCREEN_BOT, 1, SCREEN_RIGHT, 0)) {
		editor_set_status_message("Not enough room to balance.");
		return;
	}
	memcpy(balgeo, winlist, sizeof(balgeo));
	balassign(1, SCREEN_BOT, 1, SCREEN_RIGHT, 1, SCREEN_BOT, 1, SCREEN_RIGHT);
	memcpy(winlist, balgeo, sizeof(winlist));
	win_sync_view();
}

/* Give the current window's rows or columns, divider included, to the
 * adjacent windows on the side given by dx/dy.  Only safe when those
 * windows tile the window's edge exactly: one sticking out would end
 * up overlapping a third window, so that is checked first.  Returns 1
 * on success. */
static int win_merge_side(const struct editor_window *w, int dx, int dy)
{
	int i, pass, adjacent, span = 0;

	for (pass = 0; pass < 2; pass++) {
		for (i = 0; i < MAX_WINDOWS; i++) {
			struct editor_window *p = &winlist[i];

			if (!p->active || p == w)
				continue;
			if (dy < 0)
				adjacent = p->y + p->h + 1 == w->y;
			else if (dy > 0)
				adjacent = w->y + w->h + 1 == p->y;
			else if (dx < 0)
				adjacent = p->x + p->w + 1 == w->x;
			else
				adjacent = w->x + w->w + 1 == p->x;
			if (!adjacent)
				continue;

			if (dy != 0) {
				/* columns not shared with w are not affected */
				if (p->x >= w->x + w->w + 1 ||
				    w->x >= p->x + p->w + 1)
					continue;
				if (p->x < w->x || p->x + p->w > w->x + w->w)
					return 0;
				if (pass == 0) {
					span += p->w + 1;
				} else {
					if (dy > 0)
						p->y = w->y;
					p->h += w->h + 1;
				}
			} else {
				/* rows not shared with w are not affected */
				if (p->y >= w->y + w->h + 1 ||
				    w->y >= p->y + p->h + 1)
					continue;
				if (p->y < w->y || p->y + p->h > w->y + w->h)
					return 0;
				if (pass == 0) {
					span += p->h + 1;
				} else {
					if (dx > 0)
						p->x = w->x;
					p->w += w->w + 1;
				}
			}
		}
		if (pass == 0 && span != (dy != 0 ? w->w + 1 : w->h + 1))
			return 0;
	}
	return 1;
}

/* Delete the current window (C-x 0): its space goes to the neighbors
 * above, below, left or right -- the first side whose windows tile
 * the shared edge exactly. */
void win_delete_current(void)
{
	struct editor_window *cur = &winlist[win_current];
	int oy = cur->y, ox = cur->x;
	int i;

	if (win_count <= 1) {
		editor_set_status_message("Only one window.");
		return;
	}

	if (!win_merge_side(cur, 0, -1) && !win_merge_side(cur, 0, 1) &&
	    !win_merge_side(cur, -1, 0) && !win_merge_side(cur, 1, 0)) {
		editor_set_status_message("No window to absorb this one, try C-x 1.");
		return;
	}

	buf_save_current_state();
	cur->active = 0;
	win_count--;

	/* Focus the window that took over the deleted corner. */
	for (i = 0; i < MAX_WINDOWS; i++) {
		struct editor_window *p = &winlist[i];

		if (p->active &&
		    p->y <= oy && oy <= p->y + p->h &&
		    p->x <= ox && ox <= p->x + p->w)
			break;
	}
	if (i == MAX_WINDOWS)
		for (i = 0; !winlist[i].active; i++)
			;
	win_focus(i);
}

/* Delete all other windows, leaving only the current one (C-x 1). */
void win_delete_others(void)
{
	struct editor_window *cur = &winlist[win_current];
	int i;

	for (i = 0; i < MAX_WINDOWS; i++) {
		if (i != win_current) winlist[i].active = 0;
	}
	win_count = 1;

	win_fill_screen(cur);
	win_sync_view();
}

/* Shift every window edge at or past the seam by delta: windows on
 * the seam grow or shrink, windows past it follow.  Refuses when a
 * window would come out below one text row or two columns. */
static int win_shiftedges(int seam, int delta, int cols)
{
	int i, pos, size;

	for (i = 0; i < MAX_WINDOWS; i++) {
		struct editor_window *p = &winlist[i];

		if (!p->active)
			continue;
		pos  = cols ? p->x : p->y;
		size = cols ? p->w : p->h;
		if (pos <= seam && pos + size >= seam &&
		    size + delta < (cols ? 2 : 1))
			return 0;
	}

	for (i = 0; i < MAX_WINDOWS; i++) {
		struct editor_window *p = &winlist[i];

		if (!p->active)
			continue;
		if (cols) {
			if (p->x > seam)
				p->x += delta;
			else if (p->x + p->w >= seam)
				p->w += delta;
		} else {
			if (p->y > seam)
				p->y += delta;
			else if (p->y + p->h >= seam)
				p->h += delta;
		}
	}
	return 1;
}

/* One axis of a terminal resize: move the outermost window edge to
 * the new screen edge, preferring the seam at the focused window so
 * it takes the delta.  Falls back to single-window when the layout
 * cannot be squeezed any further. */
static void win_fit_axis(int cols)
{
	struct editor_window *cur = &winlist[win_current];
	int i, e, edge = 0, delta;

	for (i = 0; i < MAX_WINDOWS; i++) {
		if (!winlist[i].active)
			continue;
		e = cols ? winlist[i].x + winlist[i].w
		         : winlist[i].y + winlist[i].h;
		if (e > edge)
			edge = e;
	}
	if ((delta = (cols ? SCREEN_RIGHT : SCREEN_BOT) - edge) != 0) {
		if (!win_shiftedges(cols ? cur->x + cur->w : cur->y + cur->h,
		                    delta, cols) &&
		    !win_shiftedges(edge, delta, cols))
			win_delete_others();
	}
}

/* Adapt the layout to a new terminal size: the focused window grows
 * or shrinks, the other windows keep their size and move. */
void win_term_resize(void)
{
	if (win_count == 0)
		return;

	win_fit_axis(1);
	win_fit_axis(0);
	win_sync_view();
}
