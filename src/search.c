/* =============================== Find mode ================================ */

#include "def.h"

#define KILO_QUERY_LEN 256

void editor_find(int fd)
{
	char query[KILO_QUERY_LEN+1] = {0};
	int saved_cx = editor.cx, saved_cy = editor.cy;
	int saved_coloff = editor.coloff, saved_rowoff = editor.rowoff;
	int last_match = -1; /* Last line where a match was found. -1 for none. */
	int saved_hl_line = -1;  /* No saved HL */
	int find_next = 0; /* if 1 search next, if -1 search prev. */
	char *saved_hl = NULL;
	int qlen = 0;

#define FIND_RESTORE_HL do { \
	if (saved_hl) { \
		memcpy(editor.row[saved_hl_line].hl, saved_hl, editor.row[saved_hl_line].rsize); \
		free(saved_hl); \
		saved_hl = NULL; \
	} \
} while (0)

	while (1) {
		int c;

		editor_set_status_message("I-search: %s", query);
		editor_refresh_screen();

		c = editor_read_key(fd);
		if (c == DEL_KEY || c == CTRL_H || c == BACKSPACE) {
			if (qlen != 0) query[--qlen] = '\0';
			last_match = -1;
		} else if (c == ESC || c == ENTER || c == CTRL_G) {
			if (c == ESC) {
				editor.cx = saved_cx; editor.cy = saved_cy;
				editor.coloff = saved_coloff; editor.rowoff = saved_rowoff;
			}
			FIND_RESTORE_HL;
			editor_set_status_message("");
			return;
		} else if (c == ARROW_RIGHT || c == ARROW_DOWN || c == CTRL_S) {
			find_next = 1;
		} else if (c == ARROW_LEFT || c == ARROW_UP || c == CTRL_R) {
			find_next = -1;
		} else if (isprint(c)) {
			if (qlen < KILO_QUERY_LEN) {
				query[qlen++] = c;
				query[qlen] = '\0';
				last_match = -1;
			}
		}

		/* Search occurrence. */
		if (last_match == -1) find_next = 1;
		if (find_next) {
			char *match = NULL;
			int current = last_match;
			int match_offset = 0;
			int i;

			for (i = 0; i < editor.numrows; i++) {
				current += find_next;
				if (current == -1) current = editor.numrows - 1;
				else if (current == editor.numrows) current = 0;
				match = strstr(editor.row[current].render, query);
				if (match) {
					match_offset = match - editor.row[current].render;
					break;
				}
			}
			find_next = 0;

			/* Highlight */
			FIND_RESTORE_HL;

			if (match) {
				erow *row = &editor.row[current];
				last_match = current;
				if (row->hl) {
					saved_hl_line = current;
					saved_hl = malloc(row->rsize);
					memcpy(saved_hl, row->hl, row->rsize);
					memset(row->hl+match_offset, HL_MATCH, qlen);
				}
				editor.cy = 0;
				editor.cx = match_offset;
				editor.rowoff = current;
				editor.coloff = 0;
				/* Scroll horizontally as needed. */
				if (editor.cx > editor.screencols) {
					int diff = editor.cx - editor.screencols;
					editor.cx -= diff;
					editor.coloff += diff;
				}
			}
		}
	}
}
