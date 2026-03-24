/* undo.c - Simple undo/redo functionality */

#include "def.h"

#define MAX_UNDO_SIZE 1000

/* Global undo stack */
struct undo_stack undostack = {NULL, 0, MAX_UNDO_SIZE, -1};

/* Initialize the undo stack */
void undo_init(void)
{
	undostack.head = NULL;
	undostack.size = 0;
	undostack.max_size = MAX_UNDO_SIZE;
	undostack.clean_size = -1;  /* -1 means never saved clean */
}

/* Free the entire undo stack */
void undo_free(void)
{
	struct undo_op *op = undostack.head;

	while (op) {
		struct undo_op *next = op->next;
		if (op->text) free(op->text);
		free(op);
		op = next;
	}
	undostack.head = NULL;
	undostack.size = 0;
}

/* Push an undo operation onto the stack */
void undo_push(enum undo_type type, int row, int col, int c, char *text, int len)
{
	struct undo_op *op;

	/* Skip if undo recording is suppressed */
	if (suppress_undo) return;

	/* Create new undo operation */
	op = malloc(sizeof(struct undo_op));
	if (!op) return;

	op->type = type;
	op->row = row;
	op->col = col;
	op->c = c;
	op->text = NULL;
	op->len = 0;

	/* Copy text if provided */
	if (text && len > 0) {
		op->text = malloc(len + 1);
		if (op->text) {
			memcpy(op->text, text, len);
			op->text[len] = '\0';
			op->len = len;
		}
	}

	/* Add to front of stack */
	op->next = undostack.head;
	undostack.head = op;
	undostack.size++;

	/* Trim stack if too large */
	if (undostack.size > undostack.max_size) {
		struct undo_op *curr = undostack.head;
		struct undo_op *prev = NULL;
		int count = 0;

		/* Find the last operation to keep */
		while (curr && count < undostack.max_size - 1) {
			prev = curr;
			curr = curr->next;
			count++;
		}

		/* Free the rest */
		if (prev) {
			prev->next = NULL;
			while (curr) {
				struct undo_op *next = curr->next;
				if (curr->text) free(curr->text);
				free(curr);
				curr = next;
				undostack.size--;
			}
		}
	}
}

/* Perform undo operation */
void editor_undo(void)
{
	struct undo_op *op;

	if (!undostack.head) {
		editor_set_status_message("Nothing to undo");
		return;
	}

	op = undostack.head;
	undostack.head = op->next;
	undostack.size--;

	/* Position cursor at operation location */
	if (op->row < editor.rowoff) {
		editor.rowoff = op->row;
		editor.cy = 0;
	} else if (op->row >= editor.rowoff + editor.screenrows) {
		editor.rowoff = op->row - editor.screenrows + 1;
		editor.cy = editor.screenrows - 1;
	} else {
		editor.cy = op->row - editor.rowoff;
	}

	if (op->col < editor.coloff) {
		editor.coloff = op->col;
		editor.cx = 0;
	} else if (op->col >= editor.coloff + editor.screencols) {
		editor.coloff = op->col - editor.screencols + 1;
		editor.cx = editor.screencols - 1;
	} else {
		editor.cx = op->col - editor.coloff;
	}

	/* Perform the reverse operation */
	switch (op->type) {
	case UNDO_INSERT_CHAR:
		/* Reverse: delete the character */
		if (op->row < editor.numrows) {
			erow *row = &editor.row[op->row];
			if (op->col < row->size) {
				editor_row_del_char(row, op->col);
				editor.dirty++;
			}
		}
		break;

	case UNDO_DELETE_CHAR:
		/* Reverse: insert the character */
		if (op->row < editor.numrows) {
			erow *row = &editor.row[op->row];
			editor_row_insert_char(row, op->col, op->c);
			editor.dirty++;
		}
		break;

	case UNDO_INSERT_LINE:
		/* Reverse: delete the line */
		if (op->row < editor.numrows) {
			editor_del_row(op->row);
			editor.dirty++;
		}
		break;

	case UNDO_DELETE_LINE:
		/* Reverse: insert the line */
		if (op->text) {
			editor_insert_row(op->row, op->text, op->len);
			editor.dirty++;
		}
		break;

	case UNDO_SPLIT_LINE:
		/* Reverse: truncate row at split point, append saved rest, delete row+1.
		 * Using saved op->text rather than live row+1 content because row+1 may
		 * have an auto-indent prefix that was not part of the original text. */
		if (op->row < editor.numrows) {
			erow *row = &editor.row[op->row];
			row->size = op->col;
			row->chars[op->col] = '\0';
			if (op->text && op->len > 0)
				editor_row_append_string(row, op->text, op->len);
			else
				editor_update_row(row);
			if (op->row + 1 < editor.numrows)
				editor_del_row(op->row + 1);
			editor.dirty++;
		}
		break;

	case UNDO_JOIN_LINE:
		/* Reverse: split the line */
		if (op->row < editor.numrows && op->text) {
			erow *row = &editor.row[op->row];
			/* Insert new line after current */
			editor_insert_row(op->row + 1, op->text, op->len);
			/* Truncate current line at split point */
			row->size = op->col;
			row->chars[op->col] = '\0';
			editor_update_row(row);
			editor.dirty++;
		}
		break;

	case UNDO_KILL_TEXT:
		/* Reverse: re-insert the killed text at the original position.
		 * Cursor is already set to (op->row, op->col) above. */
		if (op->text && op->len > 0)
			editor_insert_text_raw(op->text, op->len);
		break;

	case UNDO_YANK_TEXT:
		/* Reverse: delete the yanked text forward from (op->row, op->col).
		 * Cursor is already set to (op->row, op->col) above. */
		if (op->text && op->len > 0) {
			int i;
			suppress_undo = 1;
			for (i = 0; i < op->len; i++)
				editor_del_forward_char();
			suppress_undo = 0;
		}
		break;

	case UNDO_REFLOW_PARA: {
		/* op->row = paragraph start row
		 * op->col = number of reflowed rows to delete
		 * op->text = original lines joined with '\n' */
		char *line_start, *nl, *end;
		int r;

		suppress_undo = 1;
		for (r = 0; r < op->col; r++) {
			if (op->row < editor.numrows)
				editor_del_row(op->row);
		}
		if (op->text) {
			r = op->row;
			line_start = op->text;
			end = op->text + op->len;
			while (line_start < end) {
				nl = memchr(line_start, '\n', end - line_start);
				if (nl) {
					editor_insert_row(r++, line_start, nl - line_start);
					line_start = nl + 1;
				} else {
					editor_insert_row(r++, line_start, end - line_start);
					break;
				}
			}
		}
		suppress_undo = 0;
		editor.dirty++;
		break;
	}
	}

	/* Check if we've undone back to the saved state */
	if (undostack.size == undostack.clean_size)
		editor.dirty = 0;

	/* Free the operation */
	if (op->text) free(op->text);
	free(op);

	editor_set_status_message("Undo");
}

/* Mark current state as clean (called after save) */
void undo_mark_clean(void)
{
	undostack.clean_size = undostack.size;
}
