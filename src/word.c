/* ============================ Word movement =============================== */

#include "def.h"

#define FILL_COLUMN 72

/* Move cursor forward by one word (to start of next word). */
void editorMoveWordForward(void)
{
	erow *row = (E.rowoff + E.cy >= E.numrows) ? NULL : &E.row[E.rowoff + E.cy];
	int filecol = E.coloff + E.cx;

	if (!row) return;

	/* If on whitespace, skip it first so we always land at the same
	 * relative position (start of next word) regardless of entry point. */
	if (filecol < row->size && isspace((unsigned char)row->chars[filecol])) {
		while (filecol < row->size && isspace((unsigned char)row->chars[filecol])) {
			editorMoveCursor(ARROW_RIGHT);
			filecol = E.coloff + E.cx;
		}
		return;
	}

	/* Skip current word */
	while (filecol < row->size && !isspace((unsigned char)row->chars[filecol])) {
		editorMoveCursor(ARROW_RIGHT);
		filecol = E.coloff + E.cx;
	}

	/* Skip whitespace to land at start of next word */
	while (filecol < row->size && isspace((unsigned char)row->chars[filecol])) {
		editorMoveCursor(ARROW_RIGHT);
		filecol = E.coloff + E.cx;
	}
}

/* Move cursor backward by one word */
void editorMoveWordBackward(void)
{
	erow *row = (E.rowoff + E.cy >= E.numrows) ? NULL : &E.row[E.rowoff + E.cy];
	int filerow = E.rowoff + E.cy;
	int filecol = E.coloff + E.cx;

	if (!row) return;
	if (filecol == 0) {
		/* Move to end of previous line */
		if (filerow > 0) {
			editorMoveCursor(ARROW_LEFT);
		}
		return;
	}

	/* Move back one position to check current position */
	editorMoveCursor(ARROW_LEFT);
	filerow = E.rowoff + E.cy;
	filecol = E.coloff + E.cx;
	row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

	if (!row) return;

	/* Skip whitespace */
	while (filecol > 0 && isspace(row->chars[filecol])) {
		editorMoveCursor(ARROW_LEFT);
		filecol = E.coloff + E.cx;
	}

	/* Skip word characters */
	while (filecol > 0 && !isspace(row->chars[filecol - 1])) {
		editorMoveCursor(ARROW_LEFT);
		filecol = E.coloff + E.cx;
	}
}

/* Kill from cursor to start of next word, saving text to kill ring (M-d). */
void editorKillWordForward(void)
{
	int filerow = E.rowoff + E.cy;
	int filecol = E.coloff + E.cx;
	int start_col = filecol;
	int kill_len;
	char *text;
	erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

	if (!row) return;

	/* Mirror editorMoveWordForward: skip whitespace OR word+whitespace */
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

	killRingSet(text, kill_len);
	undoPush(UNDO_KILL_TEXT, filerow, start_col, 0, text, kill_len);
	free(text);

	memmove(row->chars + start_col, row->chars + start_col + kill_len,
	        row->size - start_col - kill_len + 1);
	row->size -= kill_len;
	editorUpdateRow(row);
	E.dirty++;
}

/* Kill from start of current word back to cursor, saving text to kill ring (M-Backspace). */
void editorKillWordBackward(void)
{
	int filerow = E.rowoff + E.cy;
	int filecol = E.coloff + E.cx;
	int end_col = filecol;
	int kill_len;
	char *text;
	erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

	if (!row || filecol == 0) return;

	/* Mirror editorMoveWordBackward: skip whitespace then word chars */
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

	killRingSet(text, kill_len);
	undoPush(UNDO_KILL_TEXT, filerow, filecol, 0, text, kill_len);
	free(text);

	if (filecol < E.coloff) {
		E.coloff = filecol;
		E.cx = 0;
	} else {
		E.cx = filecol - E.coloff;
	}

	memmove(row->chars + filecol, row->chars + filecol + kill_len,
	        row->size - filecol - kill_len + 1);
	row->size -= kill_len;
	editorUpdateRow(row);
	E.dirty++;
}

/* Move to the beginning of the previous paragraph (or beginning of buffer) */
void editorMoveParagraphBackward(void)
{
	int filerow = E.rowoff + E.cy;
	int found_blank = 0;
	erow *row;

	/* If we're at the first line, we can't go back */
	if (filerow == 0) {
		editorMoveCursor(HOME_KEY);
		return;
	}

	/* Move up one line to start search */
	filerow--;

	/* Skip any blank lines we're currently on */
	while (filerow >= 0) {
		row = &E.row[filerow];
		if (row->size != 0)
			break;
		filerow--;
	}

	/* Now find the next blank line (paragraph separator) */
	while (filerow >= 0) {
		row = &E.row[filerow];
		if (row->size == 0) {
			found_blank = 1;
			break;
		}
		filerow--;
	}

	/* Position cursor at the line after the blank line, or at beginning */
	if (found_blank && filerow < E.numrows - 1) {
		filerow++;
	} else if (!found_blank) {
		filerow = 0;
	}

	/* Update cursor position */
	if (filerow < E.rowoff) {
		E.rowoff = filerow;
		E.cy = 0;
	} else {
		E.cy = filerow - E.rowoff;
	}
	E.cx = 0;
	E.coloff = 0;
}

/* Move to the beginning of the next paragraph (or end of buffer) */
void editorMoveParagraphForward(void)
{
	int filerow = E.rowoff + E.cy;
	int found_blank = 0;
	erow *row;

	/* If we're at the last line, we can't go forward */
	if (filerow >= E.numrows - 1) {
		editorMoveCursor(END_KEY);
		return;
	}

	/* Move down one line to start search */
	filerow++;

	/* Skip any blank lines we're currently on */
	while (filerow < E.numrows) {
		row = &E.row[filerow];
		if (row->size != 0)
			break;
		filerow++;
	}

	/* Now find the next blank line (paragraph separator) */
	while (filerow < E.numrows) {
		row = &E.row[filerow];
		if (row->size == 0) {
			found_blank = 1;
			break;
		}
		filerow++;
	}

	/* Position cursor at the line after the blank line, or at end */
	if (found_blank && filerow < E.numrows - 1) {
		filerow++;
	} else if (!found_blank && filerow >= E.numrows) {
		filerow = E.numrows - 1;
	}

	/* Update cursor position */
	if (filerow >= E.rowoff + E.screenrows) {
		E.rowoff = filerow - E.screenrows + 1;
		E.cy = E.screenrows - 1;
	} else {
		E.cy = filerow - E.rowoff;
	}
	E.cx = 0;
	E.coloff = 0;
}

/* Reflow the current paragraph to FILL_COLUMN (M-q).
 * Paragraph boundaries are blank lines.  Indentation from the first
 * line is detected and re-applied to every reflowed line.
 * The entire operation is recorded as a single undo record. */
void editorReflowParagraph(void)
{
	int filerow = E.rowoff + E.cy;
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

	if (filerow >= E.numrows || E.row[filerow].size == 0)
		return;

	/* Locate paragraph boundaries and sum text length for pre-allocation */
	para_start = filerow;
	while (para_start > 0 && E.row[para_start - 1].size > 0)
		para_start--;
	para_end = filerow;
	while (para_end < E.numrows - 1 && E.row[para_end + 1].size > 0)
		para_end++;
	nrows = para_end - para_start + 1;
	total_chars = 0;
	for (i = para_start; i <= para_end; i++)
		total_chars += E.row[i].size;

	fill_col = (FILL_COLUMN < E.screencols - 1) ? FILL_COLUMN : E.screencols - 1;

	/* Save original text (lines joined with '\n') for undo */
	orig_text = malloc(total_chars + nrows + 1);
	orig_len  = 0;
	for (i = para_start; i <= para_end; i++) {
		row = &E.row[i];
		if (i > para_start)
			orig_text[orig_len++] = '\n';
		memcpy(orig_text + orig_len, row->chars, row->size);
		orig_len += row->size;
	}
	orig_text[orig_len] = '\0';

	/* Detect leading whitespace indent from first paragraph line */
	row = &E.row[para_start];
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

		row  = &E.row[i];
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
		editorDelRow(i);
	for (i = 0; i < new_count; i++)
		editorInsertRow(para_start + i, new_lines[i], new_lens[i]);
	suppress_undo = 0;

	/* col = new_count: the undo handler uses it to know how many rows to delete */
	undoPush(UNDO_REFLOW_PARA, para_start, new_count, 0, orig_text, orig_len);

	free(orig_text);
	for (i = 0; i < new_count; i++) free(new_lines[i]);
	free(new_lines);
	free(new_lens);

	if (para_start < E.rowoff) {
		E.rowoff = para_start;
		E.cy = 0;
	} else if (para_start >= E.rowoff + E.screenrows) {
		E.rowoff = para_start - E.screenrows + 1;
		E.cy = E.screenrows - 1;
	} else {
		E.cy = para_start - E.rowoff;
	}
	E.cx     = indent_len;
	E.coloff = 0;
	editorSetStatusMessage("Paragraph reflowed");
}
