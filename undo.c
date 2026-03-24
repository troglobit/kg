/* undo.c - Simple undo/redo functionality */

#include "def.h"

#define MAX_UNDO_SIZE 1000

/* Global undo stack */
struct undoStack undostack = {NULL, 0, MAX_UNDO_SIZE, -1};

/* Initialize the undo stack */
void undoInit(void)
{
	undostack.head = NULL;
	undostack.size = 0;
	undostack.max_size = MAX_UNDO_SIZE;
	undostack.clean_size = -1;  /* -1 means never saved clean */
}

/* Free the entire undo stack */
void undoFree(void)
{
	struct undoOp *op = undostack.head;

	while (op) {
		struct undoOp *next = op->next;
		if (op->text) free(op->text);
		free(op);
		op = next;
	}
	undostack.head = NULL;
	undostack.size = 0;
}

/* Push an undo operation onto the stack */
void undoPush(enum undoType type, int row, int col, int c, char *text, int len)
{
	struct undoOp *op;

	/* Skip if undo recording is suppressed */
	if (suppress_undo) return;

	/* Create new undo operation */
	op = malloc(sizeof(struct undoOp));
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
		struct undoOp *curr = undostack.head;
		struct undoOp *prev = NULL;
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
				struct undoOp *next = curr->next;
				if (curr->text) free(curr->text);
				free(curr);
				curr = next;
				undostack.size--;
			}
		}
	}
}

/* Perform undo operation */
void editorUndo(void)
{
	struct undoOp *op;

	if (!undostack.head) {
		editorSetStatusMessage("Nothing to undo");
		return;
	}

	op = undostack.head;
	undostack.head = op->next;
	undostack.size--;

	/* Position cursor at operation location */
	if (op->row < E.rowoff) {
		E.rowoff = op->row;
		E.cy = 0;
	} else if (op->row >= E.rowoff + E.screenrows) {
		E.rowoff = op->row - E.screenrows + 1;
		E.cy = E.screenrows - 1;
	} else {
		E.cy = op->row - E.rowoff;
	}

	if (op->col < E.coloff) {
		E.coloff = op->col;
		E.cx = 0;
	} else if (op->col >= E.coloff + E.screencols) {
		E.coloff = op->col - E.screencols + 1;
		E.cx = E.screencols - 1;
	} else {
		E.cx = op->col - E.coloff;
	}

	/* Perform the reverse operation */
	switch (op->type) {
	case UNDO_INSERT_CHAR:
		/* Reverse: delete the character */
		if (op->row < E.numrows) {
			erow *row = &E.row[op->row];
			if (op->col < row->size) {
				editorRowDelChar(row, op->col);
				E.dirty++;
			}
		}
		break;

	case UNDO_DELETE_CHAR:
		/* Reverse: insert the character */
		if (op->row < E.numrows) {
			erow *row = &E.row[op->row];
			editorRowInsertChar(row, op->col, op->c);
			E.dirty++;
		}
		break;

	case UNDO_INSERT_LINE:
		/* Reverse: delete the line */
		if (op->row < E.numrows) {
			editorDelRow(op->row);
			E.dirty++;
		}
		break;

	case UNDO_DELETE_LINE:
		/* Reverse: insert the line */
		if (op->text) {
			editorInsertRow(op->row, op->text, op->len);
			E.dirty++;
		}
		break;

	case UNDO_SPLIT_LINE:
		/* Reverse: truncate row at split point, append saved rest, delete row+1.
		 * Using saved op->text rather than live row+1 content because row+1 may
		 * have an auto-indent prefix that was not part of the original text. */
		if (op->row < E.numrows) {
			erow *row = &E.row[op->row];
			row->size = op->col;
			row->chars[op->col] = '\0';
			if (op->text && op->len > 0)
				editorRowAppendString(row, op->text, op->len);
			else
				editorUpdateRow(row);
			if (op->row + 1 < E.numrows)
				editorDelRow(op->row + 1);
			E.dirty++;
		}
		break;

	case UNDO_JOIN_LINE:
		/* Reverse: split the line */
		if (op->row < E.numrows && op->text) {
			erow *row = &E.row[op->row];
			/* Insert new line after current */
			editorInsertRow(op->row + 1, op->text, op->len);
			/* Truncate current line at split point */
			row->size = op->col;
			row->chars[op->col] = '\0';
			editorUpdateRow(row);
			E.dirty++;
		}
		break;

	case UNDO_KILL_TEXT:
		/* Reverse: re-insert the killed text at the original position.
		 * Cursor is already set to (op->row, op->col) above. */
		if (op->text && op->len > 0)
			editorInsertTextRaw(op->text, op->len);
		break;

	case UNDO_YANK_TEXT:
		/* Reverse: delete the yanked text forward from (op->row, op->col).
		 * Cursor is already set to (op->row, op->col) above. */
		if (op->text && op->len > 0) {
			int i;
			suppress_undo = 1;
			for (i = 0; i < op->len; i++)
				editorDelForwardChar();
			suppress_undo = 0;
		}
		break;
	}

	/* Check if we've undone back to the saved state */
	if (undostack.size == undostack.clean_size)
		E.dirty = 0;

	/* Free the operation */
	if (op->text) free(op->text);
	free(op);

	editorSetStatusMessage("Undo");
}

/* Mark current state as clean (called after save) */
void undoMarkClean(void)
{
	undostack.clean_size = undostack.size;
}
