/* ============================ Word movement =============================== */

#include "def.h"

#define FILL_COLUMN 72

/* Non-zero when point sits at the very end of the buffer, i.e. there is
 * nothing further forward to move onto. */
static int at_buffer_end(void)
{
	int filerow = editor.rowoff + editor.cy;
	int filecol = editor.coloff + editor.cx;
	erow *row = (filerow >= editor.numrows) ? NULL : &editor.row[filerow];

	return (!row || filecol >= row->size) && filerow >= editor.numrows - 1;
}

/* Non-zero when the character at point is a word constituent.  The end of a
 * line (the implicit newline) and the end of the buffer count as whitespace,
 * so callers can treat the buffer as one continuous stream. */
static int point_on_word(void)
{
	int filerow = editor.rowoff + editor.cy;
	int filecol = editor.coloff + editor.cx;
	erow *row = (filerow >= editor.numrows) ? NULL : &editor.row[filerow];

	if (!row || filecol >= row->size)
		return 0;
	return !isspace((unsigned char)row->chars[filecol]);
}

/* Move cursor forward by one word (to start of next word).  Whitespace runs,
 * including line breaks, are crossed so that M-f / Ctrl-Right continue onto
 * the following line just as M-b does onto the previous one. */
void editor_move_word_forward(void)
{
	/* If standing inside a word, step past the rest of it first. */
	while (!at_buffer_end() && point_on_word())
		editor_move_cursor(ARROW_RIGHT);

	/* Then skip the whitespace gap — newlines included — to land at the
	 * start of the next word. */
	while (!at_buffer_end() && !point_on_word())
		editor_move_cursor(ARROW_RIGHT);
}

/* Move cursor backward by one word */
void editor_move_word_backward(void)
{
	erow *row = (editor.rowoff + editor.cy >= editor.numrows) ? NULL : &editor.row[editor.rowoff + editor.cy];
	int filerow = editor.rowoff + editor.cy;
	int filecol = editor.coloff + editor.cx;

	if (!row) return;
	if (filecol == 0) {
		/* Move to end of previous line */
		if (filerow > 0) {
			editor_move_cursor(ARROW_LEFT);
		}
		return;
	}

	/* Move back one position to check current position */
	editor_move_cursor(ARROW_LEFT);
	filerow = editor.rowoff + editor.cy;
	filecol = editor.coloff + editor.cx;
	row = (filerow >= editor.numrows) ? NULL : &editor.row[filerow];

	if (!row) return;

	/* Skip whitespace */
	while (filecol > 0 && isspace(row->chars[filecol])) {
		editor_move_cursor(ARROW_LEFT);
		filecol = editor.coloff + editor.cx;
	}

	/* Skip word characters */
	while (filecol > 0 && !isspace(row->chars[filecol - 1])) {
		editor_move_cursor(ARROW_LEFT);
		filecol = editor.coloff + editor.cx;
	}
}

/* Kill from cursor to start of next word, saving text to kill ring (M-d). */
void editor_kill_word_forward(void)
{
	int filerow = editor.rowoff + editor.cy;
	int filecol = editor.coloff + editor.cx;
	int start_col = filecol;
	int kill_len;
	char *text;
	erow *row = (filerow >= editor.numrows) ? NULL : &editor.row[filerow];

	if (!row) return;

	/* Skip whitespace OR word+whitespace, within the current line only
	 * (unlike editor_move_word_forward, M-d does not kill across lines). */
	if (filecol < row->size && isspace((unsigned char)row->chars[filecol])) {
		while (filecol < row->size && isspace((unsigned char)row->chars[filecol]))
			filecol++;
	} else {
		while (filecol < row->size && !isspace((unsigned char)row->chars[filecol]))
			filecol++;
		while (filecol < row->size && isspace((unsigned char)row->chars[filecol]))
			filecol++;
	}

	kill_len = filecol - start_col;
	if (kill_len <= 0) return;

	text = malloc(kill_len + 1);
	if (!text) return;
	memcpy(text, row->chars + start_col, kill_len);
	text[kill_len] = '\0';

	kill_ring_set(text, kill_len);
	undo_push(UNDO_KILL_TEXT, filerow, start_col, 0, text, kill_len);
	free(text);

	memmove(row->chars + start_col, row->chars + start_col + kill_len,
	        row->size - start_col - kill_len + 1);
	row->size -= kill_len;
	editor_update_row(row);
	editor.dirty++;
}

/* Kill from start of current word back to cursor, saving text to kill ring (M-Backspace). */
void editor_kill_word_backward(void)
{
	int filerow = editor.rowoff + editor.cy;
	int filecol = editor.coloff + editor.cx;
	int end_col = filecol;
	int kill_len;
	char *text;
	erow *row = (filerow >= editor.numrows) ? NULL : &editor.row[filerow];

	if (!row || filecol == 0) return;

	/* Mirror editor_move_word_backward: skip whitespace then word chars */
	while (filecol > 0 && isspace((unsigned char)row->chars[filecol - 1]))
		filecol--;
	while (filecol > 0 && !isspace((unsigned char)row->chars[filecol - 1]))
		filecol--;

	kill_len = end_col - filecol;
	if (kill_len <= 0) return;

	text = malloc(kill_len + 1);
	if (!text) return;
	memcpy(text, row->chars + filecol, kill_len);
	text[kill_len] = '\0';

	kill_ring_set(text, kill_len);
	undo_push(UNDO_KILL_TEXT, filerow, filecol, 0, text, kill_len);
	free(text);

	if (filecol < editor.coloff) {
		editor.coloff = filecol;
		editor.cx = 0;
	} else {
		editor.cx = filecol - editor.coloff;
	}

	memmove(row->chars + filecol, row->chars + filecol + kill_len,
	        row->size - filecol - kill_len + 1);
	row->size -= kill_len;
	editor_update_row(row);
	editor.dirty++;
}

/* Move to the beginning of the previous paragraph (or beginning of buffer) */
void editor_move_paragraph_backward(void)
{
	int filerow = editor.rowoff + editor.cy;
	int found_blank = 0;
	erow *row;

	/* If we're at the first line, we can't go back */
	if (filerow == 0) {
		editor_move_cursor(HOME_KEY);
		return;
	}

	/* Move up one line to start search */
	filerow--;

	/* Skip any blank lines we're currently on */
	while (filerow >= 0) {
		row = &editor.row[filerow];
		if (row->size != 0)
			break;
		filerow--;
	}

	/* Now find the next blank line (paragraph separator) */
	while (filerow >= 0) {
		row = &editor.row[filerow];
		if (row->size == 0) {
			found_blank = 1;
			break;
		}
		filerow--;
	}

	/* Position cursor at the line after the blank line, or at beginning */
	if (found_blank && filerow < editor.numrows - 1) {
		filerow++;
	} else if (!found_blank) {
		filerow = 0;
	}

	/* Update cursor position */
	if (filerow < editor.rowoff) {
		editor.rowoff = filerow;
		editor.cy = 0;
	} else {
		editor.cy = filerow - editor.rowoff;
	}
	editor.cx = 0;
	editor.coloff = 0;
}

/* Move to the beginning of the next paragraph (or end of buffer) */
void editor_move_paragraph_forward(void)
{
	int filerow = editor.rowoff + editor.cy;
	int found_blank = 0;
	erow *row;

	/* If we're at the last line, we can't go forward */
	if (filerow >= editor.numrows - 1) {
		editor_move_cursor(END_KEY);
		return;
	}

	/* Move down one line to start search */
	filerow++;

	/* Skip any blank lines we're currently on */
	while (filerow < editor.numrows) {
		row = &editor.row[filerow];
		if (row->size != 0)
			break;
		filerow++;
	}

	/* Now find the next blank line (paragraph separator) */
	while (filerow < editor.numrows) {
		row = &editor.row[filerow];
		if (row->size == 0) {
			found_blank = 1;
			break;
		}
		filerow++;
	}

	/* Position cursor at the line after the blank line, or at end */
	if (found_blank && filerow < editor.numrows - 1) {
		filerow++;
	} else if (!found_blank && filerow >= editor.numrows) {
		filerow = editor.numrows - 1;
	}

	/* Update cursor position */
	if (filerow >= editor.rowoff + editor.screenrows) {
		editor.rowoff = filerow - editor.screenrows + 1;
		editor.cy = editor.screenrows - 1;
	} else {
		editor.cy = filerow - editor.rowoff;
	}
	editor.cx = 0;
	editor.coloff = 0;
}

/* Sentence boundary helpers.  A sentence ends at '.', '?', or '!' that is
 * followed in the source text by whitespace (including newline / EOF). */
static int is_sentence_end(char c)
{
	return c == '.' || c == '?' || c == '!';
}

/* Move past the next sentence-ending punctuation (M-e). */
void editor_move_sentence_forward(void)
{
	while (1) {
		int filerow = editor.rowoff + editor.cy;
		int filecol = editor.coloff + editor.cx;
		erow *row;
		char c;
		int next_is_ws;

		if (filerow >= editor.numrows) return;
		row = &editor.row[filerow];

		if (filecol >= row->size) {
			if (filerow + 1 >= editor.numrows) return;
			editor_move_cursor(ARROW_RIGHT);
			continue;
		}

		c = row->chars[filecol];
		if (is_sentence_end(c)) {
			if (filecol + 1 >= row->size)
				next_is_ws = 1;
			else
				next_is_ws = isspace((unsigned char)row->chars[filecol + 1]);
			if (next_is_ws) {
				editor_move_cursor(ARROW_RIGHT);
				return;
			}
		}
		editor_move_cursor(ARROW_RIGHT);
	}
}

/* Move to the start of the current sentence — i.e. just past the previous
 * sentence-end punctuation and any whitespace that followed it (M-a).
 * If the cursor is already at a sentence start, move to the start of the
 * previous sentence.  Lands at beginning-of-buffer if no earlier sentence
 * boundary exists.
 *
 * Walks backward from the cursor and stops at the first sentence boundary
 * found, so the cost is proportional to the distance moved rather than to
 * the size of the prefix of the buffer. */
void editor_move_sentence_backward(void)
{
	int orig_r = editor.rowoff + editor.cy;
	int orig_c = editor.coloff + editor.cx;
	int r = orig_r, c = orig_c;
	int target_r = 0, target_c = 0;

	if (orig_r == 0 && orig_c == 0) goto place;

	/* Step back once so we don't immediately re-discover the boundary we're
	 * already standing on (when cursor is at a sentence start, we want the
	 * previous sentence, not the current one). */
	if (c > 0) c--;
	else { r--; c = editor.row[r].size; }

	while (1) {
		if (c < editor.row[r].size) {
			char ch = editor.row[r].chars[c];
			if (is_sentence_end(ch)) {
				/* A sentence end is [.?!] immediately followed in source
				 * by whitespace.  An end-of-line right after the period
				 * counts (the implicit newline is whitespace). */
				int is_break = (c + 1 >= editor.row[r].size) ||
				               isspace((unsigned char)editor.row[r].chars[c + 1]);
				if (is_break) {
					/* Walk forward over the whitespace gap to land on the
					 * first non-whitespace char — that is the start of the
					 * sentence we want. */
					int tr = r, tc = c + 1;
					while (tr < editor.numrows) {
						if (tc >= editor.row[tr].size) {
							tr++;
							tc = 0;
							continue;
						}
						if (!isspace((unsigned char)editor.row[tr].chars[tc])) break;
						tc++;
					}
					/* Accept the target only if it sits strictly before the
					 * original cursor — otherwise it's the start of the
					 * sentence we were already in, and we want the one
					 * before that. */
					if (tr < orig_r || (tr == orig_r && tc < orig_c)) {
						target_r = tr;
						target_c = tc;
						goto place;
					}
				}
			}
		}
		if (r == 0 && c == 0) break;
		if (c > 0) c--;
		else { r--; c = editor.row[r].size; }
	}

place:
	if (target_r < editor.rowoff) editor.rowoff = target_r;
	if (target_r >= editor.rowoff + editor.screenrows)
		editor.rowoff = target_r - editor.screenrows + 1;
	if (editor.rowoff < 0) editor.rowoff = 0;
	editor.cy = target_r - editor.rowoff;

	if (target_c < editor.coloff) editor.coloff = target_c;
	if (target_c >= editor.coloff + editor.screencols)
		editor.coloff = target_c - editor.screencols + 1;
	if (editor.coloff < 0) editor.coloff = 0;
	editor.cx = target_c - editor.coloff;
}

/* Join the current line with the previous one, stripping leading whitespace
 * from the current line and inserting a single space at the join point when
 * both sides are non-empty.  Cursor lands at the join point (M-^). */
void editor_join_line(void)
{
	int filerow = editor.rowoff + editor.cy;
	int prev_row_idx, join_col, add_space;
	erow *prev, *cur;
	const char *rest;
	int rest_len;

	if (filerow == 0) return;   /* nothing above */

	prev_row_idx = filerow - 1;
	prev = &editor.row[prev_row_idx];
	cur  = &editor.row[filerow];

	rest     = cur->chars;
	rest_len = cur->size;
	while (rest_len > 0 && isspace((unsigned char)*rest)) {
		rest++;
		rest_len--;
	}

	join_col  = prev->size;
	add_space = (join_col > 0 && rest_len > 0) ? 1 : 0;

	undo_push(UNDO_JOIN_LINE, prev_row_idx, join_col, 0, cur->chars, cur->size);

	{
		char *newchars = realloc(prev->chars, join_col + add_space + rest_len + 1);
		if (!newchars) {
			editor_set_status_message("Out of memory joining lines");
			return;
		}
		prev->chars = newchars;
	}
	if (add_space)
		prev->chars[join_col] = ' ';
	memcpy(prev->chars + join_col + add_space, rest, rest_len);
	prev->size = join_col + add_space + rest_len;
	prev->chars[prev->size] = '\0';
	editor_update_row(prev);

	suppress_undo = 1;
	editor_del_row(filerow);
	suppress_undo = 0;

	/* Move cursor to join point */
	if (prev_row_idx < editor.rowoff) {
		editor.rowoff = prev_row_idx;
		editor.cy = 0;
	} else {
		editor.cy = prev_row_idx - editor.rowoff;
	}
	if (join_col < editor.coloff) {
		editor.coloff = join_col;
		editor.cx = 0;
	} else {
		editor.cx = join_col - editor.coloff;
	}

	editor.dirty++;
}

/* Apply a case transformation to the word forward from point.
 * mode: 'u' = upcase, 'l' = downcase, 'c' = capitalize. */
static void do_word_case(int mode)
{
	int filerow = editor.rowoff + editor.cy;
	int filecol = editor.coloff + editor.cx;
	int word_start, word_end, word_len, i;
	char *orig;
	erow *row;

	row = (filerow >= editor.numrows) ? NULL : &editor.row[filerow];
	if (!row) return;

	word_start = filecol;
	while (word_start < row->size && isspace((unsigned char)row->chars[word_start]))
		word_start++;
	word_end = word_start;
	while (word_end < row->size && !isspace((unsigned char)row->chars[word_end]))
		word_end++;

	word_len = word_end - word_start;
	if (word_len <= 0) return;

	/* Save original for undo, then transform in-place */
	orig = malloc(word_len + 1);
	if (!orig) return;
	memcpy(orig, row->chars + word_start, word_len);
	orig[word_len] = '\0';

	for (i = 0; i < word_len; i++) {
		unsigned char ch = (unsigned char)orig[i];

		if (mode == 'u')
			row->chars[word_start + i] = toupper(ch);
		else if (mode == 'l')
			row->chars[word_start + i] = tolower(ch);
		else /* 'c' */
			row->chars[word_start + i] = (i == 0) ? toupper(ch) : tolower(ch);
	}
	editor_update_row(row);
	editor.dirty++;

	/* Two undo records (LIFO): first pop deletes transformed text, second re-inserts original */
	undo_push(UNDO_KILL_TEXT, filerow, word_start, 0, orig, word_len);
	undo_push(UNDO_YANK_TEXT, filerow, word_start, 0, row->chars + word_start, word_len);

	free(orig);

	if (word_end < editor.coloff) {
		editor.coloff = word_end;
		editor.cx = 0;
	} else if (word_end >= editor.coloff + editor.screencols) {
		editor.coloff = word_end - editor.screencols + 1;
		editor.cx = editor.screencols - 1;
	} else {
		editor.cx = word_end - editor.coloff;
	}
}

void editor_upcase_word(void)     { do_word_case('u'); }
void editor_downcase_word(void)   { do_word_case('l'); }
void editor_capitalize_word(void) { do_word_case('c'); }

/* Toggle line comment on the current line, or on every line covered by the
 * mark region when mark is set (M-;).
 *
 * Comment prefix (scs) is taken from the current syntax table.  When adding
 * a comment the prefix is inserted at column 0 followed by a space.  When
 * removing one, the prefix (plus the optional trailing space) is deleted from
 * wherever it sits after leading whitespace. */
void editor_comment_dwim(void)
{
	char *scs;
	int scslen;
	int row_start, row_end, r;

	if (!editor.syntax || !editor.syntax->singleline_comment_start[0]) {
		editor_set_status_message("No comment syntax for this buffer");
		return;
	}

	scs    = editor.syntax->singleline_comment_start;
	scslen = strlen(scs);

	row_start = editor.rowoff + editor.cy;
	row_end   = row_start;
	if (editor.mark_set) {
		int mark = editor.mark_row;
		int cur  = row_start;

		row_start = (mark < cur) ? mark : cur;
		row_end   = (mark > cur) ? mark : cur;
	}

	if (row_start >= editor.numrows) return;
	if (row_end   >= editor.numrows) row_end = editor.numrows - 1;

	for (r = row_start; r <= row_end; r++) {
		erow *row = &editor.row[r];
		int   i   = 0;

		/* Find first non-whitespace character. */
		while (i < row->size && isspace((unsigned char)row->chars[i]))
			i++;

		if (row->size - i >= scslen &&
		    strncmp(row->chars + i, scs, scslen) == 0) {
			/* Already commented: remove scs and an optional following space. */
			char removed[8];
			int  rlen = scslen;

			if (i + scslen < row->size && row->chars[i + scslen] == ' ')
				rlen++;

			memcpy(removed, row->chars + i, rlen);
			memmove(row->chars + i, row->chars + i + rlen,
			        row->size - i - rlen + 1);
			row->size -= rlen;
			editor_update_row(row);

			undo_push(UNDO_KILL_TEXT, r, i, 0, removed, rlen);
		} else {
			/* Not commented: insert scs + space at column 0. */
			char prefix[8];

			memcpy(prefix, scs, scslen);
			prefix[scslen] = ' ';

			row->chars = realloc(row->chars, row->size + scslen + 2);
			memmove(row->chars + scslen + 1, row->chars, row->size + 1);
			memcpy(row->chars, scs, scslen);
			row->chars[scslen] = ' ';
			row->size += scslen + 1;
			editor_update_row(row);

			undo_push(UNDO_YANK_TEXT, r, 0, 0, prefix, scslen + 1);
		}
	}
	editor.dirty = 1;
}

/* Reflow the current paragraph to FILL_COLUMN (M-q).
 * Paragraph boundaries are blank lines.  Indentation from the first
 * line is detected and re-applied to every reflowed line.
 * The entire operation is recorded as a single undo record. */
void editor_reflow_paragraph(void)
{
	int filerow = editor.rowoff + editor.cy;
	int para_start, para_end, nrows, total_chars, i;
	int fill_col, indent_len;
	erow *row;
	char *words, *indent, *orig_text;
	int words_len, orig_len;
	char **new_lines;
	int *new_lens;
	int new_count, new_cap;
	char *cur;
	int cur_len, cur_cap;
	const char *p, *word_start;
	int word_len, need;

	if (filerow >= editor.numrows || editor.row[filerow].size == 0)
		return;

	/* Locate paragraph boundaries and sum text length for pre-allocation */
	para_start = filerow;
	while (para_start > 0 && editor.row[para_start - 1].size > 0)
		para_start--;
	para_end = filerow;
	while (para_end < editor.numrows - 1 && editor.row[para_end + 1].size > 0)
		para_end++;
	nrows = para_end - para_start + 1;
	total_chars = 0;
	for (i = para_start; i <= para_end; i++)
		total_chars += editor.row[i].size;

	fill_col = (FILL_COLUMN < editor.screencols - 1) ? FILL_COLUMN : editor.screencols - 1;

	/* Save original text (lines joined with '\n') for undo */
	orig_text = malloc(total_chars + nrows + 1);
	orig_len  = 0;
	for (i = para_start; i <= para_end; i++) {
		row = &editor.row[i];
		if (i > para_start)
			orig_text[orig_len++] = '\n';
		memcpy(orig_text + orig_len, row->chars, row->size);
		orig_len += row->size;
	}
	orig_text[orig_len] = '\0';

	/* Detect leading whitespace indent from first paragraph line */
	row = &editor.row[para_start];
	indent_len = 0;
	while (indent_len < row->size && isspace((unsigned char)row->chars[indent_len]))
		indent_len++;
	indent = malloc(indent_len + 1);
	if (indent_len > 0)
		memcpy(indent, row->chars, indent_len);
	indent[indent_len] = '\0';

	/* Build word stream: strip leading/trailing whitespace per line, join with spaces */
	words     = malloc(total_chars + nrows + 1);
	words_len = 0;
	for (i = para_start; i <= para_end; i++) {
		const char *line;
		int len;

		row  = &editor.row[i];
		line = row->chars;
		len  = row->size;

		while (len > 0 && isspace((unsigned char)*line))       { line++; len--; }
		while (len > 0 && isspace((unsigned char)line[len-1])) len--;
		if (len == 0) continue;

		if (words_len > 0)
			words[words_len++] = ' ';
		memcpy(words + words_len, line, len);
		words_len += len;
	}
	words[words_len] = '\0';

	/* Word-wrap into new_lines, tracking lengths to avoid strlen on insert */
	new_cap   = 8;
	new_lines = malloc(new_cap * sizeof(char *));
	new_lens  = malloc(new_cap * sizeof(int));
	new_count = 0;

	cur_cap = fill_col + indent_len + 2;
	cur     = malloc(cur_cap);
	memcpy(cur, indent, indent_len);
	cur_len = indent_len;

	p = words;
	while (*p) {
		while (*p == ' ') p++;
		if (!*p) break;

		word_start = p;
		while (*p && *p != ' ') p++;
		word_len = p - word_start;

		need = cur_len + (cur_len > indent_len ? 1 : 0) + word_len;
		if (need <= fill_col || cur_len == indent_len) {
			if (cur_len > indent_len) {
				if (cur_len + 1 >= cur_cap) {
					cur_cap *= 2;
					cur = realloc(cur, cur_cap);
				}
				cur[cur_len++] = ' ';
			}
			while (cur_len + word_len >= cur_cap) {
				cur_cap *= 2;
				cur = realloc(cur, cur_cap);
			}
			memcpy(cur + cur_len, word_start, word_len);
			cur_len += word_len;
		} else {
			char *saved = malloc(cur_len + 1);
			memcpy(saved, cur, cur_len);
			saved[cur_len] = '\0';
			if (new_count >= new_cap) {
				new_cap  *= 2;
				new_lines = realloc(new_lines, new_cap * sizeof(char *));
				new_lens  = realloc(new_lens,  new_cap * sizeof(int));
			}
			new_lines[new_count] = saved;
			new_lens[new_count]  = cur_len;
			new_count++;

			memcpy(cur, indent, indent_len);
			cur_len = indent_len;
			while (cur_len + word_len >= cur_cap) {
				cur_cap *= 2;
				cur = realloc(cur, cur_cap);
			}
			memcpy(cur + cur_len, word_start, word_len);
			cur_len += word_len;
		}
	}
	if (cur_len > indent_len) {
		char *saved = malloc(cur_len + 1);
		memcpy(saved, cur, cur_len);
		saved[cur_len] = '\0';
		if (new_count >= new_cap) {
			new_cap  *= 2;
			new_lines = realloc(new_lines, new_cap * sizeof(char *));
			new_lens  = realloc(new_lens,  new_cap * sizeof(int));
		}
		new_lines[new_count] = saved;
		new_lens[new_count]  = cur_len;
		new_count++;
	}
	free(cur);
	free(words);
	free(indent);

	/* Replace paragraph rows with reflowed lines */
	suppress_undo = 1;
	for (i = para_end; i >= para_start; i--)
		editor_del_row(i);
	for (i = 0; i < new_count; i++)
		editor_insert_row(para_start + i, new_lines[i], new_lens[i]);
	suppress_undo = 0;

	/* col = new_count: the undo handler uses it to know how many rows to delete */
	undo_push(UNDO_REFLOW_PARA, para_start, new_count, 0, orig_text, orig_len);

	free(orig_text);
	for (i = 0; i < new_count; i++) free(new_lines[i]);
	free(new_lines);
	free(new_lens);

	if (para_start < editor.rowoff) {
		editor.rowoff = para_start;
		editor.cy = 0;
	} else if (para_start >= editor.rowoff + editor.screenrows) {
		editor.rowoff = para_start - editor.screenrows + 1;
		editor.cy = editor.screenrows - 1;
	} else {
		editor.cy = para_start - editor.rowoff;
	}
	editor.cx     = indent_len;
	editor.coloff = 0;
	editor_set_status_message("Paragraph reflowed");
}
