/* ============================ Find / Replace =============================== */

#include "def.h"

#define KILO_QUERY_LEN 256

#define RESTORE_HL do { \
	if (saved_hl) { \
		memcpy(editor.row[saved_hl_line].hl, saved_hl, editor.row[saved_hl_line].rsize); \
		free(saved_hl); \
		saved_hl = NULL; \
	} \
} while (0)

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
			RESTORE_HL;
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
			RESTORE_HL;

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

void editor_query_replace(int fd)
{
	char search[KILO_QUERY_LEN+1] = {0};
	char replace[KILO_QUERY_LEN+1] = {0};
	char *saved_hl = NULL;
	int saved_hl_line = -1;
	int slen, rlen;
	int filerow, match_col;
	int count = 0, replace_all = 0;

	if (editor_read_line(fd, "Query replace: ", search, sizeof(search)) < 0 || !search[0])
		return;
	if (editor_read_line(fd, "Replace with: ", replace, sizeof(replace)) < 0)
		return;

	slen = strlen(search);
	rlen = strlen(replace);
	filerow   = editor.rowoff + editor.cy;
	match_col = editor.coloff + editor.cx;

	while (filerow < editor.numrows) {
		char *match = strstr(editor.row[filerow].chars + match_col, search);
		int c;

		if (!match) {
			filerow++;
			match_col = 0;
			continue;
		}
		match_col = match - editor.row[filerow].chars;

		editor_goto_line_direct(filerow + 1, match_col + 1);

		/* Highlight the match.  Convert the chars offset to a render
		 * offset so the highlight lands correctly even when tabs precede
		 * the match on the line. */
		RESTORE_HL;
		{
			erow *row = &editor.row[filerow];
			if (row->hl) {
				int i, rcol = 0;
				for (i = 0; i < match_col; i++)
					rcol += (row->chars[i] == '\t') ? (8 - rcol % 8) : 1;
				saved_hl_line = filerow;
				saved_hl = malloc(row->rsize);
				memcpy(saved_hl, row->hl, row->rsize);
				if (rcol + slen <= row->rsize)
					memset(row->hl + rcol, HL_MATCH, slen);
			}
		}

		if (!replace_all) {
			editor_set_status_message(
				"Replace \"%s\" with \"%s\"? (y/n/!/q)", search, replace);
			editor_refresh_screen();
			c = editor_read_key(fd);
		} else {
			c = 'y';
		}

		if (c == ESC || c == CTRL_G || c == 'q')
			break;
		if (c == '!') {
			replace_all = 1;
			c = 'y';
		}

		if (c == 'y' || c == ENTER) {
			erow *row = &editor.row[filerow];
			int i;

			/* Push two undo entries so C-_ fully reverses the replacement:
			 * YANK_TEXT is popped first and deletes the inserted replacement;
			 * KILL_TEXT is popped second and reinserts the original search text. */
			undo_push(UNDO_KILL_TEXT, filerow, match_col, 0, search, slen);
			undo_push(UNDO_YANK_TEXT, filerow, match_col, 0, replace, rlen);

			suppress_undo = 1;
			for (i = 0; i < slen; i++)
				editor_row_del_char(row, match_col);
			for (i = 0; i < rlen; i++)
				editor_row_insert_char(row, match_col + i, (unsigned char)replace[i]);
			suppress_undo = 0;

			match_col += rlen;
			count++;
		} else {
			match_col++;
		}
	}

	RESTORE_HL;
	editor_set_status_message(count ? "Replaced %d occurrence%s." : "No replacements made.",
				  count, count == 1 ? "" : "s");
}
