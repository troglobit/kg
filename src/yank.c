/* yank.c - Kill ring (yank buffer) for copy/paste operations */

#include "def.h"

/* Global kill ring */
struct kill_ring killring = {NULL, 0};

/* Initialize the kill ring */
void kill_ring_init(void)
{
	killring.text = NULL;
	killring.len = 0;
}

/* Free the kill ring */
void kill_ring_free(void)
{
	if (killring.text) {
		free(killring.text);
		killring.text = NULL;
		killring.len = 0;
	}
}

/* Set the kill ring to new text (replaces existing content) */
void kill_ring_set(char *text, int len)
{
	if (len <= 0) return;

	kill_ring_free();
	killring.text = malloc(len + 1);
	if (!killring.text) return;

	memcpy(killring.text, text, len);
	killring.text[len] = '\0';
	killring.len = len;
}

/* Append text to the kill ring (for consecutive kills) */
void kill_ring_append(char *text, int len)
{
	char *new_text;

	if (len <= 0) return;

	if (!killring.text) {
		kill_ring_set(text, len);
		return;
	}

	new_text = realloc(killring.text, killring.len + len + 1);
	if (!new_text) return;

	memcpy(new_text + killring.len, text, len);
	new_text[killring.len + len] = '\0';
	killring.text = new_text;
	killring.len += len;
}

/* Get the kill ring text (returns NULL if empty) */
char *kill_ring_get(void)
{
	return killring.text;
}

/* Set mark at current cursor position */
void editor_set_mark(void)
{
	editor.mark_set = 1;
	editor.mark_row = editor.rowoff + editor.cy;
	editor.mark_col = editor.coloff + editor.cx;
	editor_set_status_message("Mark set");
}

/* Get text from region (between mark and point) */
static char *get_region_text(int *out_len)
{
	int start_row, start_col, end_row, end_col;
	int cur_row = editor.rowoff + editor.cy;
	int cur_col = editor.coloff + editor.cx;
	int total_len = 0;
	char *text;
	int pos = 0;
	int row;

	if (!editor.mark_set) return NULL;

	/* Determine which position comes first */
	if (editor.mark_row < cur_row || (editor.mark_row == cur_row && editor.mark_col < cur_col)) {
		start_row = editor.mark_row;
		start_col = editor.mark_col;
		end_row = cur_row;
		end_col = cur_col;
	} else {
		start_row = cur_row;
		start_col = cur_col;
		end_row = editor.mark_row;
		end_col = editor.mark_col;
	}

	/* Calculate total length needed */
	for (row = start_row; row <= end_row && row < editor.numrows; row++) {
		if (row == start_row && row == end_row) {
			/* Single line region */
			total_len += end_col - start_col;
		} else if (row == start_row) {
			/* First line */
			total_len += editor.row[row].size - start_col + 1; /* +1 for newline */
		} else if (row == end_row) {
			/* Last line */
			total_len += end_col;
		} else {
			/* Middle lines */
			total_len += editor.row[row].size + 1; /* +1 for newline */
		}
	}

	if (total_len == 0) return NULL;

	/* Allocate and copy text */
	text = malloc(total_len + 1);
	if (!text) return NULL;

	for (row = start_row; row <= end_row && row < editor.numrows; row++) {
		int copy_start = (row == start_row) ? start_col : 0;
		int copy_end = (row == end_row) ? end_col : editor.row[row].size;
		int copy_len;

		if (copy_end > editor.row[row].size) copy_end = editor.row[row].size;
		if (copy_start > editor.row[row].size) copy_start = editor.row[row].size;

		copy_len = copy_end - copy_start;
		if (copy_len > 0) {
			memcpy(text + pos, editor.row[row].chars + copy_start, copy_len);
			pos += copy_len;
		}

		/* Add newline except for last line */
		if (row < end_row)
			text[pos++] = '\n';
	}

	text[pos] = '\0';
	*out_len = pos;
	return text;
}

/* Kill (cut) region - removes text and saves to kill ring */
void editor_kill_region(void)
{
	int start_row, start_col;
	int cur_row = editor.rowoff + editor.cy;
	int cur_col = editor.coloff + editor.cx;
	char *text;
	int len;

	if (!editor.mark_set) {
		editor_set_status_message("No mark set");
		return;
	}

	text = get_region_text(&len);
	if (!text) {
		editor_set_status_message("Empty region");
		return;
	}

	kill_ring_set(text, len);

	/* Delete the region */
	if (editor.mark_row < cur_row || (editor.mark_row == cur_row && editor.mark_col < cur_col)) {
		start_row = editor.mark_row;
		start_col = editor.mark_col;
	} else {
		start_row = cur_row;
		start_col = cur_col;
	}

	/* Position cursor at start of region */
	if (start_row < editor.rowoff) {
		editor.rowoff = start_row;
		editor.cy = 0;
	} else {
		editor.cy = start_row - editor.rowoff;
	}
	editor.cx = start_col;
	editor.coloff = 0;

	/* Record single undo operation for entire region kill */
	undo_push(UNDO_KILL_TEXT, start_row, start_col, 0, text, len);

	/* Delete character by character, but suppress individual undo records */
	suppress_undo = 1;
	for (int i = 0; i < len; i++) {
		editor_del_forward_char();
	}
	suppress_undo = 0;

	editor.mark_set = 0;
	free(text);
	editor_set_status_message("Region killed");
}

/* Copy region - saves to kill ring without removing */
void editor_copy_region(void)
{
	char *text;
	int len;

	if (!editor.mark_set) {
		editor_set_status_message("No mark set");
		return;
	}

	text = get_region_text(&len);
	if (!text) {
		editor_set_status_message("Empty region");
		return;
	}

	kill_ring_set(text, len);
	editor.mark_set = 0;
	free(text);
	editor_set_status_message("Region copied");
}

/* Yank (paste) from kill ring */
void editor_yank(void)
{
	int filerow = editor.rowoff + editor.cy;
	int filecol = editor.coloff + editor.cx;
	char *text = kill_ring_get();

	if (!text) {
		editor_set_status_message("Kill ring is empty");
		return;
	}

	/* Record single undo operation for entire yank */
	undo_push(UNDO_YANK_TEXT, filerow, filecol, 0, text, killring.len);

	editor_insert_text_raw(text, killring.len);

	editor_set_status_message("Yanked");
}
