/* yank.c - Kill ring (yank buffer) for copy/paste operations */

#include "def.h"

/* Global kill ring */
struct killRing killring = {NULL, 0};

/* Initialize the kill ring */
void killRingInit(void) {
	killring.text = NULL;
	killring.len = 0;
}

/* Free the kill ring */
void killRingFree(void) {
	if (killring.text) {
		free(killring.text);
		killring.text = NULL;
		killring.len = 0;
	}
}

/* Set the kill ring to new text (replaces existing content) */
void killRingSet(char *text, int len) {
	if (len <= 0) return;

	killRingFree();
	killring.text = malloc(len + 1);
	if (!killring.text) return;

	memcpy(killring.text, text, len);
	killring.text[len] = '\0';
	killring.len = len;
}

/* Append text to the kill ring (for consecutive kills) */
void killRingAppend(char *text, int len) {
	if (len <= 0) return;

	if (!killring.text) {
		killRingSet(text, len);
		return;
	}

	char *new_text = realloc(killring.text, killring.len + len + 1);
	if (!new_text) return;

	memcpy(new_text + killring.len, text, len);
	new_text[killring.len + len] = '\0';
	killring.text = new_text;
	killring.len += len;
}

/* Get the kill ring text (returns NULL if empty) */
char *killRingGet(void) {
	return killring.text;
}

/* Set mark at current cursor position */
void editorSetMark(void) {
	E.mark_set = 1;
	E.mark_row = E.rowoff + E.cy;
	E.mark_col = E.coloff + E.cx;
	editorSetStatusMessage("Mark set");
}

/* Get text from region (between mark and point) */
static char *getRegionText(int *out_len) {
	if (!E.mark_set) return NULL;

	int start_row, start_col, end_row, end_col;

	/* Determine which position comes first */
	int cur_row = E.rowoff + E.cy;
	int cur_col = E.coloff + E.cx;

	if (E.mark_row < cur_row || (E.mark_row == cur_row && E.mark_col < cur_col)) {
		start_row = E.mark_row;
		start_col = E.mark_col;
		end_row = cur_row;
		end_col = cur_col;
	} else {
		start_row = cur_row;
		start_col = cur_col;
		end_row = E.mark_row;
		end_col = E.mark_col;
	}

	/* Calculate total length needed */
	int total_len = 0;
	for (int row = start_row; row <= end_row && row < E.numrows; row++) {
		if (row == start_row && row == end_row) {
			/* Single line region */
			total_len += end_col - start_col;
		} else if (row == start_row) {
			/* First line */
			total_len += E.row[row].size - start_col + 1; /* +1 for newline */
		} else if (row == end_row) {
			/* Last line */
			total_len += end_col;
		} else {
			/* Middle lines */
			total_len += E.row[row].size + 1; /* +1 for newline */
		}
	}

	if (total_len == 0) return NULL;

	/* Allocate and copy text */
	char *text = malloc(total_len + 1);
	if (!text) return NULL;

	int pos = 0;
	for (int row = start_row; row <= end_row && row < E.numrows; row++) {
		int copy_start = (row == start_row) ? start_col : 0;
		int copy_end = (row == end_row) ? end_col : E.row[row].size;

		if (copy_end > E.row[row].size) copy_end = E.row[row].size;
		if (copy_start > E.row[row].size) copy_start = E.row[row].size;

		int copy_len = copy_end - copy_start;
		if (copy_len > 0) {
			memcpy(text + pos, E.row[row].chars + copy_start, copy_len);
			pos += copy_len;
		}

		/* Add newline except for last line */
		if (row < end_row) {
			text[pos++] = '\n';
		}
	}

	text[pos] = '\0';
	*out_len = pos;
	return text;
}

/* Kill (cut) region - removes text and saves to kill ring */
void editorKillRegion(void) {
	if (!E.mark_set) {
		editorSetStatusMessage("No mark set");
		return;
	}

	int len;
	char *text = getRegionText(&len);
	if (!text) {
		editorSetStatusMessage("Empty region");
		return;
	}

	killRingSet(text, len);

	/* Delete the region */
	int start_row, start_col;
	int cur_row = E.rowoff + E.cy;
	int cur_col = E.coloff + E.cx;

	if (E.mark_row < cur_row || (E.mark_row == cur_row && E.mark_col < cur_col)) {
		start_row = E.mark_row;
		start_col = E.mark_col;
	} else {
		start_row = cur_row;
		start_col = cur_col;
	}

	/* Position cursor at start of region */
	if (start_row < E.rowoff) {
		E.rowoff = start_row;
		E.cy = 0;
	} else {
		E.cy = start_row - E.rowoff;
	}
	E.cx = start_col;
	E.coloff = 0;

	/* Record single undo operation for entire region kill */
	undoPush(UNDO_KILL_TEXT, start_row, start_col, 0, text, len);

	/* Delete character by character, but suppress individual undo records */
	suppress_undo = 1;
	for (int i = 0; i < len; i++) {
		editorDelChar();
	}
	suppress_undo = 0;

	E.mark_set = 0;
	free(text);
	editorSetStatusMessage("Region killed");
}

/* Copy region - saves to kill ring without removing */
void editorCopyRegion(void) {
	if (!E.mark_set) {
		editorSetStatusMessage("No mark set");
		return;
	}

	int len;
	char *text = getRegionText(&len);
	if (!text) {
		editorSetStatusMessage("Empty region");
		return;
	}

	killRingSet(text, len);
	E.mark_set = 0;
	free(text);
	editorSetStatusMessage("Region copied");
}

/* Yank (paste) from kill ring */
void editorYank(void) {
	char *text = killRingGet();
	if (!text) {
		editorSetStatusMessage("Kill ring is empty");
		return;
	}

	int filerow = E.rowoff+E.cy;
	int filecol = E.coloff+E.cx;

	/* Record single undo operation for entire yank */
	undoPush(UNDO_YANK_TEXT, filerow, filecol, 0, text, killring.len);

	/* Insert each character from the kill ring, suppress individual undo records */
	suppress_undo = 1;
	for (int i = 0; i < killring.len; i++) {
		if (text[i] == '\n') {
			editorInsertNewline();
		} else {
			editorInsertChar(text[i]);
		}
	}
	suppress_undo = 0;

	editorSetStatusMessage("Yanked");
}
