/* rect.c - Rectangle operations (C-x SPC, C-x r {k,y,d,c}) */

#include "def.h"

/* Rectangle kill ring.  Holds the last killed/copied rectangle as a
 * '\n'-joined string of per-row content, plus the row count so yank
 * can rebuild the rectangle exactly even when some rows are empty. */
static char *rect_killed       = NULL;
static int   rect_killed_len   = 0;
static int   rect_killed_nrows = 0;

void rect_kill_ring_free(void)
{
	free(rect_killed);
	rect_killed       = NULL;
	rect_killed_len   = 0;
	rect_killed_nrows = 0;
}

static void rect_kill_ring_set(char *text, int len, int nrows)
{
	rect_kill_ring_free();
	rect_killed = malloc(len + 1);
	if (!rect_killed) return;
	if (len > 0) memcpy(rect_killed, text, len);
	rect_killed[len]   = '\0';
	rect_killed_len    = len;
	rect_killed_nrows  = nrows;
}

/* Resolve point + mark into normalized rectangle bounds.  The column
 * range is in VISUAL columns so tab-laden rows under the rect get the
 * same visible left/right edges; each operation maps the visual range
 * back to its own byte range per row via editor_chars_col_at_visual().
 * Returns 0 (with a status message) if there is no mark. */
static int rect_bounds(int *s_row, int *s_vcol, int *e_row, int *e_vcol)
{
	int p_row, p_col, m_row, m_col;
	int p_vcol, m_vcol;

	if (!editor.mark_set) {
		editor_set_status_message("No mark set");
		return 0;
	}
	p_row = editor.rowoff + editor.cy;
	p_col = editor.coloff + editor.cx;
	m_row = editor.mark_row;
	m_col = editor.mark_col;
	p_vcol = (p_row < editor.numrows)
		? editor_visual_col(&editor.row[p_row], p_col) : p_col;
	m_vcol = (m_row < editor.numrows)
		? editor_visual_col(&editor.row[m_row], m_col) : m_col;
	*s_row  = (p_row  < m_row)  ? p_row  : m_row;
	*e_row  = (p_row  > m_row)  ? p_row  : m_row;
	*s_vcol = (p_vcol < m_vcol) ? p_vcol : m_vcol;
	*e_vcol = (p_vcol > m_vcol) ? p_vcol : m_vcol;
	return 1;
}

/* Map a row's [s_vcol, e_vcol) visual range to its byte range.  Clamps
 * to row content — past-EOL portions don't contribute bytes, only the
 * visual highlight (rendered as virtual spaces). */
static void rect_row_byte_range(erow *row, int s_vcol, int e_vcol,
				int *byte_lo, int *byte_hi)
{
	int lo = editor_chars_col_at_visual(row, s_vcol);
	int hi = editor_chars_col_at_visual(row, e_vcol);
	if (lo > row->size) lo = row->size;
	if (hi > row->size) hi = row->size;
	*byte_lo = lo;
	*byte_hi = hi;
}

/* Snapshot rows [start_row, end_row) joined with '\n' for undo storage.
 * Caller frees the returned buffer.  Returns NULL when there is nothing
 * to snapshot, with *out_len = 0. */
static char *rect_snapshot_rows(int start_row, int end_row, int *out_len)
{
	int total = 0;
	int r;
	char *buf, *p;

	if (start_row < 0)            start_row = 0;
	if (end_row   > editor.numrows) end_row = editor.numrows;
	if (end_row  <= start_row) {
		*out_len = 0;
		return NULL;
	}

	for (r = start_row; r < end_row; r++) {
		total += editor.row[r].size;
		if (r < end_row - 1) total++;   /* '\n' separator */
	}

	buf = malloc(total + 1);
	if (!buf) {
		*out_len = 0;
		return NULL;
	}
	p = buf;
	for (r = start_row; r < end_row; r++) {
		memcpy(p, editor.row[r].chars, editor.row[r].size);
		p += editor.row[r].size;
		if (r < end_row - 1) *p++ = '\n';
	}
	*p = '\0';
	*out_len = total;
	return buf;
}

/* Common tear-down after a rectangle command. */
static void rect_deactivate(void)
{
	editor.mark_set       = 0;
	editor.mark_highlight = 0;
	editor.rect_mode      = 0;
	editor.shift_select   = 0;
	editor_snap_cx_to_row();
}

/* Kill or delete rectangle.  When save_to_ring is non-zero, the cut
 * rectangle is stored so editor_yank_rect can paste it.  Each row's
 * byte range is resolved from the visual column bounds independently,
 * so tabs and UTF-8 don't bend the cut into a non-rectangular shape. */
static void rect_kill_or_delete(int save_to_ring)
{
	int s_row, s_vcol, e_row, e_vcol;
	int orig_numrows;
	int s_row_byte_lo;
	char *snap;
	int snap_len;
	int r;

	if (!rect_bounds(&s_row, &s_vcol, &e_row, &e_vcol))
		return;
	if (s_row == e_row && s_vcol == e_vcol) {
		editor_set_status_message("Empty rectangle");
		return;
	}

	orig_numrows = editor.numrows;
	snap = rect_snapshot_rows(s_row, e_row + 1, &snap_len);

	/* Build the rectangle text for the kill ring (each row's chars
	 * intersected with the per-row byte range, joined with '\n'). */
	if (save_to_ring) {
		int killed_total = 0;
		int killed_nrows = e_row - s_row + 1;
		char *killed_text;

		for (r = s_row; r <= e_row && r < editor.numrows; r++) {
			int lo, hi;
			rect_row_byte_range(&editor.row[r], s_vcol, e_vcol, &lo, &hi);
			killed_total += hi - lo;
			if (r < e_row) killed_total++;
		}
		killed_text = malloc(killed_total + 1);
		if (killed_text) {
			char *p = killed_text;
			for (r = s_row; r <= e_row && r < editor.numrows; r++) {
				erow *row = &editor.row[r];
				int lo, hi;
				rect_row_byte_range(row, s_vcol, e_vcol, &lo, &hi);
				if (hi > lo) {
					memcpy(p, row->chars + lo, hi - lo);
					p += hi - lo;
				}
				if (r < e_row) *p++ = '\n';
			}
			*p = '\0';
			rect_kill_ring_set(killed_text, killed_total, killed_nrows);
			free(killed_text);
		}
	}

	/* Cursor lands at the rect's left edge on the start row — convert
	 * s_vcol to that row's byte for the goto, and for the undo anchor. */
	if (s_row < editor.numrows) {
		int hi_unused;
		rect_row_byte_range(&editor.row[s_row], s_vcol, s_vcol,
				    &s_row_byte_lo, &hi_unused);
	} else {
		s_row_byte_lo = 0;
	}

	undo_push(UNDO_RECT_OVERWRITE, s_row, s_row_byte_lo, orig_numrows,
		  snap ? snap : (char *)"", snap_len);
	free(snap);

	suppress_undo = 1;
	for (r = s_row; r <= e_row && r < editor.numrows; r++) {
		erow *row = &editor.row[r];
		int lo, hi, i;

		rect_row_byte_range(row, s_vcol, e_vcol, &lo, &hi);
		for (i = hi - 1; i >= lo; i--)
			editor_row_del_char(row, i);
	}
	suppress_undo = 0;

	editor_cursor_goto(s_row, s_row_byte_lo);
	rect_deactivate();
	editor.dirty++;
	editor_set_status_message(save_to_ring ? "Rectangle killed" : "Rectangle deleted");
}

void editor_kill_rect(void)
{
	rect_kill_or_delete(1);
}

void editor_delete_rect(void)
{
	rect_kill_or_delete(0);
}

/* Replace each row's chars in the visual [s_vcol, e_vcol) span with
 * spaces, padding short rows out so the rectangle "exists" everywhere
 * it should.  Each row resolves the visual range to its own byte range. */
void editor_clear_rect(void)
{
	int s_row, s_vcol, e_row, e_vcol;
	int orig_numrows;
	int s_row_byte_lo;
	char *snap;
	int snap_len;
	int r;

	if (!rect_bounds(&s_row, &s_vcol, &e_row, &e_vcol))
		return;
	if (s_row == e_row && s_vcol == e_vcol) {
		editor_set_status_message("Empty rectangle");
		return;
	}

	orig_numrows = editor.numrows;
	snap = rect_snapshot_rows(s_row, e_row + 1, &snap_len);

	if (s_row < editor.numrows) {
		int hi_unused;
		rect_row_byte_range(&editor.row[s_row], s_vcol, s_vcol,
				    &s_row_byte_lo, &hi_unused);
	} else {
		s_row_byte_lo = 0;
	}

	undo_push(UNDO_RECT_OVERWRITE, s_row, s_row_byte_lo, orig_numrows,
		  snap ? snap : (char *)"", snap_len);
	free(snap);

	suppress_undo = 1;
	for (r = s_row; r <= e_row && r < editor.numrows; r++) {
		erow *row = &editor.row[r];
		int lo, hi, i;

		/* Pad with spaces until row's visual width reaches s_vcol. */
		while (editor_visual_col(row, row->size) < s_vcol)
			editor_row_insert_char(row, row->size, ' ');

		rect_row_byte_range(row, s_vcol, e_vcol, &lo, &hi);
		for (i = lo; i < hi; i++) {
			editor_row_del_char(row, i);
			editor_row_insert_char(row, i, ' ');
		}

		/* Extend with spaces if the rect runs past row's visual end. */
		while (editor_visual_col(row, row->size) < e_vcol)
			editor_row_insert_char(row, row->size, ' ');
	}
	suppress_undo = 0;

	editor_cursor_goto(s_row, s_row_byte_lo);
	rect_deactivate();
	editor.dirty++;
	editor_set_status_message("Rectangle cleared");
}

/* Insert the last killed rectangle at point, padding short target rows
 * with spaces and appending new rows when the buffer is too short. */
void editor_yank_rect(void)
{
	int cur_row, cur_col;
	int orig_numrows;
	int rows_to_snap;
	char *snap;
	int snap_len;
	char *p, *end;
	int i;

	if (!rect_killed || rect_killed_nrows == 0) {
		editor_set_status_message("No rectangle to yank");
		return;
	}

	cur_row = editor.rowoff + editor.cy;
	cur_col = editor.coloff + editor.cx;
	orig_numrows = editor.numrows;

	rows_to_snap = rect_killed_nrows;
	if (cur_row + rows_to_snap > editor.numrows)
		rows_to_snap = editor.numrows - cur_row;
	if (rows_to_snap < 0) rows_to_snap = 0;
	snap = rect_snapshot_rows(cur_row, cur_row + rows_to_snap, &snap_len);

	undo_push(UNDO_RECT_OVERWRITE, cur_row, cur_col, orig_numrows,
		  snap ? snap : (char *)"", snap_len);
	free(snap);

	suppress_undo = 1;
	p   = rect_killed;
	end = rect_killed + rect_killed_len;
	i   = 0;
	while (i < rect_killed_nrows) {
		char *nl = (p < end) ? memchr(p, '\n', end - p) : NULL;
		int line_len = nl ? (nl - p) : (end - p);
		int target = cur_row + i;
		erow *r;
		int j;

		while (target >= editor.numrows)
			editor_insert_row(editor.numrows, "", 0);
		r = &editor.row[target];
		while (r->size < cur_col)
			editor_row_insert_char(r, r->size, ' ');
		for (j = 0; j < line_len; j++)
			editor_row_insert_char(r, cur_col + j, p[j]);

		if (!nl) break;
		p = nl + 1;
		i++;
	}
	suppress_undo = 0;

	rect_deactivate();
	editor.dirty++;
	editor_set_status_message("Rectangle yanked");
}

/* C-x SPC: start a rectangular region at point, or cancel an active one. */
void editor_rect_mode_toggle(void)
{
	if (editor.rect_mode && editor.mark_highlight) {
		editor.rect_mode      = 0;
		editor.mark_highlight = 0;
		editor_snap_cx_to_row();
		editor_set_status_message("");
		return;
	}
	editor_set_mark_silent();
	editor.rect_mode = 1;
	editor_set_status_message("Rectangle mark set");
}
