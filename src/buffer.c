/* ======================= Editor rows implementation ======================= */

#include "def.h"

/* Update the rendered version and the syntax highlight of a row. */
void editor_update_row(erow *row)
{
	unsigned long long allocsize;
	unsigned int tabs = 0, nonprint = 0;
	int j, idx;

	/* Create a version of the row we can directly print on the screen,
	 * respecting tabs, substituting non printable characters with '?'. */
	free(row->render);
	for (j = 0; j < row->size; j++)
		if (row->chars[j] == TAB) tabs++;

	allocsize = (unsigned long long)row->size + tabs*8 + nonprint*9 + 1;
	if (allocsize > UINT32_MAX) {
		editor_set_status_message("Line too long for editor");
		running = 0;
		return;
	}

	row->render = malloc(row->size + tabs*8 + nonprint*9 + 1);
	idx = 0;
	for (j = 0; j < row->size; j++) {
		if (row->chars[j] == TAB) {
			row->render[idx++] = ' ';
			while ((idx+1) % 8 != 0) row->render[idx++] = ' ';
		} else {
			row->render[idx++] = row->chars[j];
		}
	}
	row->rsize = idx;
	row->render[idx] = '\0';

	/* Update the syntax highlighting attributes of the row. */
	editor_update_syntax(row);
}

/* Insert a row at the specified position, shifting the other rows on the bottom
 * if required. */
void editor_insert_row(int at, char *s, size_t len)
{
	if (at > editor.numrows) return;
	editor.row = realloc(editor.row, sizeof(erow) * (editor.numrows+1));
	if (at != editor.numrows) {
		memmove(editor.row+at+1, editor.row+at, sizeof(editor.row[0]) * (editor.numrows-at));
		for (int j = at+1; j <= editor.numrows; j++) editor.row[j].idx++;
	}
	editor.row[at].size = len;
	editor.row[at].chars = malloc(len+1);
	memcpy(editor.row[at].chars, s, len+1);
	editor.row[at].hl = NULL;
	editor.row[at].hl_oc = 0;
	editor.row[at].render = NULL;
	editor.row[at].rsize = 0;
	editor.row[at].idx = at;
	editor_update_row(editor.row+at);
	editor.numrows++;
	editor.dirty++;
}

/* Free row's heap allocated stuff. */
void editor_free_row(erow *row)
{
	free(row->render);
	free(row->chars);
	free(row->hl);
}

/* Remove the row at the specified position, shifting the remaining on the top. */
void editor_del_row(int at)
{
	erow *row;

	if (at >= editor.numrows) return;
	row = editor.row+at;
	editor_free_row(row);
	memmove(editor.row+at, editor.row+at+1, sizeof(editor.row[0]) * (editor.numrows-at-1));
	for (int j = at; j < editor.numrows-1; j++) editor.row[j].idx--;
	editor.numrows--;
	editor.dirty++;
}

/* Turn the editor rows into a single heap-allocated string.
 * Returns the pointer to the heap-allocated string and populate the
 * integer pointed by 'buflen' with the size of the string, excluding
 * the final nulterm. */
char *editor_rows_to_string(erow *rows, int numrows, int *buflen)
{
	char *buf = NULL, *p;
	int totlen = 0;
	int j;

	/* Compute count of bytes */
	for (j = 0; j < numrows; j++)
		totlen += rows[j].size+1; /* +1 is for "\n" at end of every row */
	*buflen = totlen;
	totlen++; /* Also make space for nulterm */

	p = buf = malloc(totlen);
	for (j = 0; j < numrows; j++) {
		memcpy(p, rows[j].chars, rows[j].size);
		p += rows[j].size;
		*p = '\n';
		p++;
	}
	*p = '\0';
	return buf;
}

/* Insert a character at the specified position in a row, moving the remaining
 * chars on the right if needed. */
void editor_row_insert_char(erow *row, int at, int c)
{
	if (at > row->size) {
		/* Pad the string with spaces if the insert location is outside the
		 * current length by more than a single character. */
		int padlen = at - row->size;
		/* In the next line +2 means: new char and null term. */
		row->chars = realloc(row->chars, row->size+padlen+2);
		memset(row->chars+row->size, ' ', padlen);
		row->chars[row->size+padlen+1] = '\0';
		row->size += padlen+1;
	} else {
		/* If we are in the middle of the string just make space for 1 new
		 * char plus the (already existing) null term. */
		row->chars = realloc(row->chars, row->size+2);
		memmove(row->chars+at+1, row->chars+at, row->size-at+1);
		row->size++;
	}
	row->chars[at] = c;
	editor_update_row(row);
	editor.dirty++;
}

/* Append the string 's' at the end of a row */
void editor_row_append_string(erow *row, char *s, size_t len)
{
	row->chars = realloc(row->chars, row->size+len+1);
	memcpy(row->chars+row->size, s, len);
	row->size += len;
	row->chars[row->size] = '\0';
	editor_update_row(row);
	editor.dirty++;
}

/* Delete the character at offset 'at' from the specified row. */
void editor_row_del_char(erow *row, int at)
{
	if (row->size <= at) return;
	memmove(row->chars+at, row->chars+at+1, row->size-at);
	editor_update_row(row);
	row->size--;
	editor.dirty++;
}

/* Insert the specified char at the current prompt position. */
void editor_insert_char(int c)
{
	erow *row = (editor.rowoff + editor.cy >= editor.numrows) ? NULL : &editor.row[editor.rowoff + editor.cy];
	int filerow = editor.rowoff + editor.cy;
	int filecol = editor.coloff + editor.cx;

	/* If the row where the cursor is currently located does not exist in our
	 * logical representation of the file, add enough empty rows as needed. */
	if (!row) {
		while (editor.numrows <= filerow)
			editor_insert_row(editor.numrows, "", 0);
	}
	row = &editor.row[filerow];

	/* Record undo operation */
	undo_push(UNDO_INSERT_CHAR, filerow, filecol, c, NULL, 0);

	editor_row_insert_char(row, filecol, c);
	if (editor.cx == editor.screencols - 1)
		editor.coloff++;
	else
		editor.cx++;
	editor.dirty++;
}

/* Split the current line at the cursor without auto-indent.
 * Used by yank and kill-undo to re-insert newlines exactly as copied. */
void editor_insert_newline_raw(void)
{
	int filerow = editor.rowoff + editor.cy;
	int filecol = editor.coloff + editor.cx;
	erow *row;
	int rest_len;

	if (filerow >= editor.numrows) {
		editor_insert_row(filerow, "", 0);
	} else {
		row = &editor.row[filerow];
		if (filecol > row->size) filecol = row->size;
		rest_len = row->size - filecol;
		editor_insert_row(filerow + 1, row->chars + filecol, rest_len);
		row = &editor.row[filerow];
		row->chars[filecol] = '\0';
		row->size = filecol;
		editor_update_row(row);
	}
	if (editor.cy == editor.screenrows - 1) editor.rowoff++;
	else editor.cy++;
	editor.cx = 0;
	editor.coloff = 0;
}

/* Insert text character by character without recording undo, using raw
 * newlines (no auto-indent).  Used by editor_yank and UNDO_KILL_TEXT. */
void editor_insert_text_raw(const char *text, int len)
{
	int i;
	suppress_undo = 1;
	for (i = 0; i < len; i++) {
		if (text[i] == '\n')
			editor_insert_newline_raw();
		else
			editor_insert_char(text[i]);
	}
	suppress_undo = 0;
}

/* Inserting a newline is slightly complex as we have to handle inserting a
 * newline in the middle of a line, splitting the line as needed. */
void editor_insert_newline(void)
{
	erow *row = (editor.rowoff + editor.cy >= editor.numrows) ? NULL : &editor.row[editor.rowoff + editor.cy];
	char *new_content;
	int filerow = editor.rowoff + editor.cy;
	int filecol = editor.coloff + editor.cx;
	int rest_len, indent = 0;

	if (!row) {
		if (filerow == editor.numrows) {
			editor_insert_row(filerow, "", 0);
			goto fixcursor;
		}
		return;
	}
	/* If the cursor is over the current line size, we want to conceptually
	 * think it's just over the last character. */
	if (filecol >= row->size) filecol = row->size;
	if (filecol == 0) {
		undo_push(UNDO_INSERT_LINE, filerow, 0, 0, NULL, 0);
		editor_insert_row(filerow, "", 0);
	} else {
		/* Compute leading whitespace of the current line for auto-indent. */
		while (indent < row->size &&
		       (row->chars[indent] == ' ' || row->chars[indent] == TAB))
			indent++;
		/* Don't indent past the split point. */
		if (indent > filecol) indent = filecol;

		/* Build new line: indent prefix + rest of split. */
		rest_len = row->size - filecol;
		new_content = malloc(indent + rest_len + 1);
		memcpy(new_content, row->chars, indent);
		memcpy(new_content + indent, row->chars + filecol, rest_len);
		new_content[indent + rest_len] = '\0';

		/* Record undo: save the original rest without the indent prefix. */
		undo_push(UNDO_SPLIT_LINE, filerow, filecol, 0, row->chars + filecol, rest_len);
		editor_insert_row(filerow + 1, new_content, indent + rest_len);
		free(new_content);
		row = &editor.row[filerow];
		row->chars[filecol] = '\0';
		row->size = filecol;
		editor_update_row(row);
	}
fixcursor:
	if (editor.cy == editor.screenrows - 1) {
		editor.rowoff++;
	} else {
		editor.cy++;
	}
	editor.cx = indent;
	editor.coloff = 0;
	if (editor.cx >= editor.screencols) {
		editor.coloff = indent - editor.screencols + 1;
		editor.cx = editor.screencols - 1;
	}
}

/* Delete the char at the current prompt position. */
void editor_del_char(void)
{
	erow *row = (editor.rowoff + editor.cy >= editor.numrows) ? NULL : &editor.row[editor.rowoff + editor.cy];
	int filerow = editor.rowoff + editor.cy;
	int filecol = editor.coloff + editor.cx;

	if (!row || (filecol == 0 && filerow == 0)) return;
	if (filecol == 0) {
		/* Handle the case of column 0, we need to move the current line
		 * on the right of the previous one. */
		/* Record undo: save the line that will be joined */
		undo_push(UNDO_JOIN_LINE, filerow-1, editor.row[filerow-1].size, 0, row->chars, row->size);
		filecol = editor.row[filerow-1].size;
		editor_row_append_string(&editor.row[filerow-1], row->chars, row->size);
		editor_del_row(filerow);
		row = NULL;
		if (editor.cy == 0)
			editor.rowoff--;
		else
			editor.cy--;
		editor.cx = filecol;
		if (editor.cx >= editor.screencols) {
			int shift = (editor.screencols - editor.cx) + 1;
			editor.cx -= shift;
			editor.coloff += shift;
		}
	} else {
		/* Record undo: save the character being deleted */
		undo_push(UNDO_DELETE_CHAR, filerow, filecol-1, row->chars[filecol-1], NULL, 0);
		editor_row_del_char(row, filecol-1);
		if (editor.cx == 0 && editor.coloff)
			editor.coloff--;
		else
			editor.cx--;
	}
	if (row) editor_update_row(row);
	editor.dirty++;
}

/* Forward-delete the char at the current cursor position (DEL key).
 * At end of line, joins with the next line. */
void editor_del_forward_char(void)
{
	erow *row = (editor.rowoff + editor.cy >= editor.numrows) ? NULL : &editor.row[editor.rowoff + editor.cy];
	int filerow = editor.rowoff + editor.cy;
	int filecol = editor.coloff + editor.cx;

	if (!row) return;

	if (filecol == row->size) {
		if (filerow + 1 >= editor.numrows) return;
		undo_push(UNDO_JOIN_LINE, filerow, filecol, 0,
			 editor.row[filerow+1].chars, editor.row[filerow+1].size);
		editor_row_append_string(row, editor.row[filerow+1].chars, editor.row[filerow+1].size);
		editor_del_row(filerow + 1);
	} else {
		undo_push(UNDO_DELETE_CHAR, filerow, filecol, row->chars[filecol], NULL, 0);
		editor_row_del_char(row, filecol);
	}
	editor.dirty++;
}

/* Kill (delete) from cursor to end of line (C-k). */
void editor_kill_line(void)
{
	erow *row = (editor.rowoff + editor.cy >= editor.numrows) ? NULL : &editor.row[editor.rowoff + editor.cy];
	int filerow = editor.rowoff + editor.cy;
	int filecol = editor.coloff + editor.cx;

	if (!row) return;

	if (filecol >= row->size) {
		/* At end of line, join with next line like C-k in Emacs. */
		if (filerow+1 < editor.numrows) {
			/* Save newline to kill ring */
			kill_ring_append("\n", 1);
			/* Record undo: save the line that will be joined */
			undo_push(UNDO_KILL_TEXT, filerow, filecol, 0,
				 editor.row[filerow+1].chars, editor.row[filerow+1].size);
			editor_row_append_string(row, editor.row[filerow+1].chars, editor.row[filerow+1].size);
			editor_del_row(filerow+1);
		}
	} else {
		/* Delete from cursor to end of line and save to kill ring. */
		int kill_len = row->size - filecol;
		if (kill_len > 0) {
			kill_ring_append(row->chars + filecol, kill_len);
			/* Record undo operation */
			undo_push(UNDO_KILL_TEXT, filerow, filecol, 0, row->chars + filecol, kill_len);
		}
		row->chars[filecol] = '\0';
		row->size = filecol;
		editor_update_row(row);
		editor.dirty++;
	}
}
