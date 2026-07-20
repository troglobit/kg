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

/* Smart case: an all-lowercase query folds case, a query with any uppercase
 * letter searches case-sensitively, like GNU Emacs. */
static int query_has_upper(const char *q, int qlen)
{
	int i;

	for (i = 0; i < qlen; i++)
		if (isupper((unsigned char)q[i]))
			return 1;
	return 0;
}

/* strstr, optionally folding case. */
static char *case_strstr(const char *hay, const char *needle, int fold)
{
	if (!fold)
		return strstr(hay, needle);
	if (!*needle)
		return (char *)hay;

	for (; *hay; hay++) {
		const char *h = hay, *n = needle;

		while (*h && *n &&
		       tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
			h++;
			n++;
		}
		if (!*n)
			return (char *)hay;
	}
	return NULL;
}

/* Rightmost occurrence of needle whose end falls at or before `limit`, so a
 * reverse search lands on the match before point (not one straddling it) and
 * repeats step backward, like GNU Emacs. */
static char *isearch_find_last_before(const char *s, const char *needle,
				      int limit, int qlen, int fold)
{
	char *best = NULL;
	char *match = (char *)s;

	while ((match = case_strstr(match, needle, fold)) != NULL) {
		if (match - s + qlen > limit)
			break;
		best = match;
		match++;
	}
	return best;
}

/* Scan the rows from (start_row, start_col) in `direction`, wrapping once
 * through the buffer, for `query`.  On a hit fills *match_row/_col/_len and
 * returns 1; returns 0 when nothing matches.  Columns index row->render. */
static int isearch_find_match(int start_row, int start_col, int direction,
			      const char *query, int qlen, int fold,
			      int *match_row, int *match_col, int *match_len)
{
	int current, i;

	if (editor.numrows == 0 || qlen == 0) return 0;
	if (start_row < 0) start_row = 0;
	else if (start_row >= editor.numrows) start_row = editor.numrows - 1;

	current = start_row;
	for (i = 0; i < editor.numrows; i++) {
		erow *row = &editor.row[current];
		int col = (i == 0) ? start_col : (direction > 0 ? 0 : row->rsize);
		char *match;

		if (col < 0) col = 0;
		else if (col > row->rsize) col = row->rsize;

		if (direction > 0)
			match = case_strstr(row->render + col, query, fold);
		else
			match = isearch_find_last_before(row->render, query, col, qlen, fold);

		if (match) {
			*match_row = current;
			*match_col = match - row->render;
			*match_len = qlen;
			return 1;
		}

		current += direction;
		if (current < 0) current = editor.numrows - 1;
		else if (current == editor.numrows) current = 0;
	}
	return 0;
}

/* A motion or set-mark command typed during incremental search ends the
 * search and runs from the match, the way Emacs hands off to the command
 * instead of beeping.  Returns 1 if c was such a key, 0 otherwise.  These
 * motions mirror editor_process_keypress(); keep the two in step. */
static int isearch_handoff_key(int c)
{
	switch (c) {
	case KEY_NULL:            /* C-SPC: set mark, leave point at the match */
		editor_set_mark();
		return 1;
	case CTRL_A:
	case HOME_KEY:
		editor_move_cursor(HOME_KEY);
		break;
	case CTRL_E:
	case END_KEY:
		editor_move_cursor(END_KEY);
		break;
	case CTRL_B:
		editor_move_cursor(ARROW_LEFT);
		break;
	case CTRL_F:
		editor_move_cursor(ARROW_RIGHT);
		break;
	case CTRL_N:
		editor_move_cursor(ARROW_DOWN);
		break;
	case CTRL_P:
		editor_move_cursor(ARROW_UP);
		break;
	case CTRL_D:
		editor_del_forward_char();
		break;
	case CTRL_HOME:
	case CTRL_PAGE_UP:
	case ALT_LT:
		editor_move_to_beginning();
		break;
	case CTRL_END:
	case CTRL_PAGE_DOWN:
	case ALT_GT:
		editor_move_to_end();
		break;
	case ALT_B:
	case CTRL_ARROW_LEFT:
		editor_move_word_backward();
		break;
	case ALT_F:
	case CTRL_ARROW_RIGHT:
		editor_move_word_forward();
		break;
	case ALT_M:
		editor_move_to_indentation();
		break;
	case ALT_A:
		editor_move_sentence_backward();
		break;
	case ALT_E:
		editor_move_sentence_forward();
		break;
	case ALT_LBRACE:
	case CTRL_ARROW_UP:
		editor_move_paragraph_backward();
		break;
	case ALT_RBRACE:
	case CTRL_ARROW_DOWN:
		editor_move_paragraph_forward();
		break;
	default:
		return 0;
	}
	editor_set_status_message("");
	return 1;
}

void editor_find(int fd, int direction)
{
	char query[KILO_QUERY_LEN+1] = {0};
	int saved_cx = editor.cx, saved_cy = editor.cy;
	int saved_coloff = editor.coloff, saved_rowoff = editor.rowoff;
	int start_row = editor.rowoff + editor.cy;
	int start_col = 0;
	int last_match_row = -1, last_match_col = -1;
	int saved_hl_line = -1;  /* No saved HL */
	int find_next = 0; /* if 1 search next, if -1 search prev. */
	char *saved_hl = NULL;
	int qlen = 0;

	/* Anchor the search at point so a fresh query, and reverse search in
	 * particular, starts where the cursor is rather than at the top.  The
	 * scan indexes row->render, so anchor in render columns too. */
	if (start_row >= 0 && start_row < editor.numrows)
		start_col = chars_to_render_col(&editor.row[start_row],
						editor.coloff + editor.cx);

	while (1) {
		int c;

		editor_set_status_message("I-search: %s", query);
		editor_refresh_screen();

		c = editor_read_key(fd);
		if (c == DEL_KEY || c == CTRL_H || c == BACKSPACE) {
			if (qlen != 0) query[--qlen] = '\0';
			last_match_row = last_match_col = -1;
			find_next = direction;
		} else if (c == ESC || c == ENTER || c == CTRL_G) {
			if (c == ESC) {
				editor.cx = saved_cx; editor.cy = saved_cy;
				editor.coloff = saved_coloff; editor.rowoff = saved_rowoff;
			}
			RESTORE_HL;
			editor_set_status_message("");
			return;
		} else if (c == ARROW_RIGHT || c == ARROW_DOWN || c == CTRL_S) {
			direction = find_next = 1;
		} else if (c == ARROW_LEFT || c == ARROW_UP || c == CTRL_R) {
			direction = find_next = -1;
		} else if (isprint(c)) {
			if (qlen < KILO_QUERY_LEN) {
				query[qlen++] = c;
				query[qlen] = '\0';
				last_match_row = last_match_col = -1;
				find_next = direction;
			}
		} else if (isearch_handoff_key(c)) {
			RESTORE_HL;
			return;
		}

		/* Search occurrence. */
		if (find_next) {
			int current = start_row, col = start_col;
			int match_row, match_col, match_len;
			int fold = !query_has_upper(query, qlen);

			/* Repeat from just past the last hit; a fresh query
			 * (last_match_row == -1) restarts from point. */
			if (last_match_row != -1) {
				current = last_match_row;
				col = last_match_col + (direction > 0 ? 1 : 0);
			}
			find_next = 0;

			/* Highlight */
			RESTORE_HL;

			if (isearch_find_match(current, col, direction, query, qlen, fold,
					       &match_row, &match_col, &match_len)) {
				erow *row = &editor.row[match_row];

				last_match_row = match_row;
				last_match_col = match_col;
				if (row->hl) {
					saved_hl_line = match_row;
					saved_hl = malloc(row->rsize);
					memcpy(saved_hl, row->hl, row->rsize);
					memset(row->hl + match_col, HL_MATCH, match_len);
				}
				/* Land point at the far end of the match in the
				 * search direction: end when going forward, start
				 * when going back, like Emacs isearch. */
				editor_reveal_position_centered(match_row,
				    match_col + (direction > 0 ? match_len : 0));
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
	int slen, rlen, fold;
	int filerow, match_col;
	int count = 0, replace_all = 0;

	if (editor_read_line(fd, "Query replace: ", search, sizeof(search)) < 0 || !search[0])
		return;
	if (editor_read_line(fd, "Replace with: ", replace, sizeof(replace)) < 0)
		return;

	slen = strlen(search);
	rlen = strlen(replace);
	fold = !query_has_upper(search, slen);
	filerow   = editor.rowoff + editor.cy;
	match_col = editor.coloff + editor.cx;

	while (filerow < editor.numrows) {
		char *match = case_strstr(editor.row[filerow].chars + match_col, search, fold);
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
			char matched[KILO_QUERY_LEN + 1];
			int i;

			/* Record the text actually matched, not the query: under
			 * case folding they differ, and undo must restore what was
			 * really there. */
			memcpy(matched, row->chars + match_col, slen);
			matched[slen] = '\0';

			/* Undo in two steps: YANK_TEXT (popped first) deletes the
			 * inserted replacement, then KILL_TEXT restores the original. */
			undo_push(UNDO_KILL_TEXT, filerow, match_col, 0, matched, slen);
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
