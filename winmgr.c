/* ========================= Window management ============================== */

#include "def.h"

struct editorWindow winlist[MAX_WINDOWS];
int win_current    = 0;
int win_count      = 0;
int win_total_rows = 0;
int win_total_cols = 0;

/* Save the active E cursor/scroll state into the current window slot and
 * also into its buffer slot so switching buffers later is consistent. */
void winSaveActiveView(void)
{
	struct editorWindow *w = &winlist[win_current];

	w->cx     = E.cx;
	w->cy     = E.cy;
	w->rowoff = E.rowoff;
	w->coloff = E.coloff;

	/* Keep buflist in sync so a buffer switch restores correctly. */
	if (w->bufidx < MAX_BUFFERS && buflist[w->bufidx].active) {
		buflist[w->bufidx].cx     = E.cx;
		buflist[w->bufidx].cy     = E.cy;
		buflist[w->bufidx].rowoff = E.rowoff;
		buflist[w->bufidx].coloff = E.coloff;
	}
}

/* Load the buffer associated with win_current into live E state and restore
 * the window's cursor/scroll.  Called after win_current changes. */
static void winActivateWindow(void)
{
	struct editorBuffer *b;
	buf_current = winlist[win_current].bufidx;
	b = &buflist[buf_current];
	E.numrows  = b->numrows;
	E.row      = b->row;
	E.dirty    = b->dirty;
	E.filename = b->filename;
	E.syntax   = b->syntax;
	E.mark_set = b->mark_set;
	E.mark_row = b->mark_row;
	E.mark_col = b->mark_col;
	undostack  = b->undostack;
	winRestoreActiveView();
}

/* Restore the active window's cursor/scroll into E and update E.screenrows/cols
 * to match the window dimensions so all movement code stays correct. */
void winRestoreActiveView(void)
{
	struct editorWindow *w = &winlist[win_current];

	E.cx         = w->cx;
	E.cy         = w->cy;
	E.rowoff     = w->rowoff;
	E.coloff     = w->coloff;
	E.screenrows = w->h;
	E.screencols = w->w;
}

/* Recompute the layout of all active windows.
 *
 * Windows are organised into column groups (col_group field).  Windows sharing
 * the same col_group value are stacked vertically in the same column; different
 * col_group values are placed side-by-side.
 *
 * Layout algorithm:
 *   1. Collect distinct col_group values in order of first appearance.
 *   2. Divide win_total_cols equally among column groups.
 *   3. Within each column group, divide the usable rows equally among its windows.
 *      Each window band = (text rows) + 1 mode-line row.
 *   4. One global echo-area row is reserved at win_total_rows.
 */
void winReflow(void)
{
	int col_groups[MAX_WINDOWS];
	int num_col_groups = 0;
	int usable, col_w, col_rem, col_x;
	int i, j;

	if (win_count == 0) return;

	/* Collect distinct col_group values in appearance order. */
	for (i = 0; i < MAX_WINDOWS; i++) {
		int found = 0;
		if (!winlist[i].active) continue;
		for (j = 0; j < num_col_groups; j++) {
			if (col_groups[j] == winlist[i].col_group) { found = 1; break; }
		}
		if (!found) col_groups[num_col_groups++] = winlist[i].col_group;
	}

	if (num_col_groups == 0) return;

	usable  = win_total_rows - 1; /* reserve 1 row for the global echo area */
	/* Reserve one column per inter-group gap for the vertical separator. */
	col_w   = (win_total_cols - (num_col_groups - 1)) / num_col_groups;
	col_rem = (win_total_cols - (num_col_groups - 1)) % num_col_groups;
	col_x   = 1; /* VT100 columns are 1-based */

	for (j = 0; j < num_col_groups; j++) {
		int cg  = col_groups[j];
		int cw  = col_w + (j == num_col_groups - 1 ? col_rem : 0);
		int n   = 0; /* windows in this col_group */
		int row_h, row_rem, row_y, win_idx;

		for (i = 0; i < MAX_WINDOWS; i++)
			if (winlist[i].active && winlist[i].col_group == cg) n++;

		row_h   = usable / n;
		row_rem = usable % n;
		row_y   = 1;
		win_idx = 0;

		for (i = 0; i < MAX_WINDOWS; i++) {
			int band_h;
			if (!winlist[i].active || winlist[i].col_group != cg) continue;

			band_h = row_h + (win_idx < row_rem ? 1 : 0);
			winlist[i].y = row_y;
			winlist[i].x = col_x;
			winlist[i].h = band_h - 1; /* text rows; mode line uses the last row */
			winlist[i].w = cw;
			row_y  += band_h;
			win_idx++;
		}

		col_x += cw + 1; /* +1 skips the separator column between groups */
	}

	E.screenrows = winlist[win_current].h;
	E.screencols = winlist[win_current].w;

	/* Clamp cursor to new bounds. */
	if (E.cy >= E.screenrows) E.cy = E.screenrows - 1;
	if (E.cx >= E.screencols) E.cx = E.screencols - 1;
	if (E.cy < 0)             E.cy = 0;
	if (E.cx < 0)             E.cx = 0;
}

/* Initialise the window list with a single window covering the whole screen.
 * Called once from initEditor() after updateWindowSize(). */
void winInit(void)
{
	memset(winlist, 0, sizeof(winlist));
	win_current = 0;
	win_count   = 1;

	winlist[0].bufidx    = 0;
	winlist[0].active    = 1;
	winlist[0].col_group = 0;

	/* win_total_rows/cols are set by updateWindowSize() before winInit(). */
	winReflow();
}

/* Split the current window horizontally (C-x 2): the current window shrinks
 * to the top half; a new window showing the same buffer appears below.
 * The new window stays in the same column group. */
void winSplitHorizontal(void)
{
	int i, slot = -1;

	if (win_count >= MAX_WINDOWS) {
		editorSetStatusMessage("Too many windows (%d max).", MAX_WINDOWS);
		return;
	}
	if (winlist[win_current].h < 3) {
		editorSetStatusMessage("Window too small to split.");
		return;
	}

	for (i = 0; i < MAX_WINDOWS; i++) {
		if (!winlist[i].active) { slot = i; break; }
	}
	if (slot < 0) return;

	bufSaveCurrentState();

	/* New window inherits the same buffer, cursor state, and col_group. */
	winlist[slot]        = winlist[win_current];
	winlist[slot].active = 1;

	win_count++;
	winReflow();
}

/* Split the current window vertically (C-x 3): the current window becomes the
 * left half; a new window showing the same buffer appears to the right.
 * The new window is placed in a new column group. */
void winSplitVertical(void)
{
	int i, slot = -1, max_cg = 0;

	if (win_count >= MAX_WINDOWS) {
		editorSetStatusMessage("Too many windows (%d max).", MAX_WINDOWS);
		return;
	}
	if (winlist[win_current].w < 6) {
		editorSetStatusMessage("Window too small to split.");
		return;
	}

	for (i = 0; i < MAX_WINDOWS; i++) {
		if (!winlist[i].active) { if (slot < 0) slot = i; continue; }
		if (winlist[i].col_group > max_cg) max_cg = winlist[i].col_group;
	}
	if (slot < 0) return;

	bufSaveCurrentState();

	/* New window: same buffer/cursor, but a new column group (rightmost). */
	winlist[slot]           = winlist[win_current];
	winlist[slot].active    = 1;
	winlist[slot].col_group = max_cg + 1;

	win_count++;
	winReflow();
}

/* Switch focus to the next window (C-x o). */
void winCycleNext(void)
{
	int i;

	if (win_count <= 1) {
		editorSetStatusMessage("No other windows.");
		return;
	}

	bufSaveCurrentState();

	for (i = 1; i <= MAX_WINDOWS; i++) {
		int idx = (win_current + i) % MAX_WINDOWS;
		if (winlist[idx].active) { win_current = idx; break; }
	}

	winActivateWindow();
	editorSetStatusMessage("%s", E.filename ? E.filename : "[new]");
}

/* Delete the current window (C-x 0). */
void winDeleteCurrent(void)
{
	int i;

	if (win_count <= 1) {
		editorSetStatusMessage("Only one window.");
		return;
	}

	bufSaveCurrentState();
	winlist[win_current].active = 0;
	win_count--;

	for (i = 0; i < MAX_WINDOWS; i++) {
		if (winlist[i].active) { win_current = i; break; }
	}

	winReflow();
	winActivateWindow();
}

/* Delete all other windows, leaving only the current one (C-x 1). */
void winDeleteOthers(void)
{
	int i;

	for (i = 0; i < MAX_WINDOWS; i++) {
		if (i != win_current) winlist[i].active = 0;
	}
	win_count = 1;
	winReflow();
}
