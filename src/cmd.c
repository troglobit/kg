/* ======================= Named command dispatcher ========================== */

#include "def.h"

/* ---- Individual commands ---- */

/* Print the editor version string. */
static void cmd_version(int fd)
{
	(void)fd;
	editor_set_status_message("kg version %s", KG_VERSION);
}

/* Toggle read-only mode on the current buffer. */
static void cmd_toggle_read_only(int fd)
{
	(void)fd;
	editor.readonly = !editor.readonly;
	editor_set_status_message(editor.readonly ? "Read-only" : "Writable");
}

/* Clear the modified flag without saving. */
static void cmd_not_modified(int fd)
{
	(void)fd;
	editor.dirty = 0;
	editor_set_status_message("Modification-flag cleared");
}

/* Show current line, column, and position within the buffer. */
static void cmd_what_cursor_position(int fd)
{
	int line = editor.rowoff + editor.cy + 1;
	int col  = editor.coloff + editor.cx + 1;
	int pct  = editor.numrows ? (line * 100) / editor.numrows : 100;

	(void)fd;
	editor_set_status_message("Line %d of %d (%d%%), col %d",
	                          line, editor.numrows, pct, col);
}

/* Go to a specific line (prompts for line or line:col). */
static void cmd_goto_line(int fd)
{
	editor_goto_line(fd);
}

/* Save current buffer to its file. */
static void cmd_save_buffer(int fd)
{
	editor_save(fd);
}

/* Strip trailing whitespace from row, push one undo record, return bytes removed. */
static int strip_trailing_whitespace(erow *row, int filerow)
{
	int newsize = row->size;
	int removed;

	while (newsize > 0 && isspace((unsigned char)row->chars[newsize - 1]))
		newsize--;

	if (newsize == row->size)
		return 0;

	removed = row->size - newsize;
	/* Each removed span is a separate undo record so C-_ restores line by line. */
	undo_push(UNDO_KILL_TEXT, filerow, newsize, 0, row->chars + newsize, removed);
	row->chars[newsize] = '\0';
	row->size = newsize;
	editor_update_row(row);
	return removed;
}

/* Remove trailing whitespace from every line in the buffer. */
static void cmd_whitespace_cleanup(int fd)
{
	int r, changed = 0;

	(void)fd;

	for (r = 0; r < editor.numrows; r++) {
		if (strip_trailing_whitespace(&editor.row[r], r))
			changed++;
	}

	if (changed)
		editor.dirty = 1;
	editor_set_status_message(
		changed ? "Removed trailing whitespace from %d line%s."
		        : "No trailing whitespace found.",
		changed, changed == 1 ? "" : "s");
}

/* Remove trailing whitespace from the current line only. */
static void cmd_delete_trailing_space(int fd)
{
	int filerow = editor.rowoff + editor.cy;
	int removed;

	(void)fd;

	if (filerow >= editor.numrows)
		return;

	removed = strip_trailing_whitespace(&editor.row[filerow], filerow);
	if (!removed) {
		editor_set_status_message("No trailing whitespace on this line");
		return;
	}
	editor.dirty = 1;
	editor_set_status_message("Removed %d trailing space%s",
	                          removed, removed == 1 ? "" : "s");
}

/* ---- Command table ---- */

typedef void (*cmdfn)(int fd);

struct named_cmd {
	const char *name;
	cmdfn fn;
};

static const struct named_cmd cmdtable[] = {
	{ "delete-trailing-space",  cmd_delete_trailing_space  },
	{ "goto-line",              cmd_goto_line              },
	{ "not-modified",           cmd_not_modified           },
	{ "save-buffer",            cmd_save_buffer            },
	{ "toggle-read-only",       cmd_toggle_read_only       },
	{ "version",                cmd_version                },
	{ "what-cursor-position",   cmd_what_cursor_position   },
	{ "whitespace-cleanup",     cmd_whitespace_cleanup     },
	{ NULL, NULL }
};

/* Prompt "M-x", filter by typing, Tab-complete, Left/Right cycle, Enter execute. */
void editor_named_command(int fd)
{
	char name[64];
	char msg[256];
	int  len = 0, c, i, off;
	int  sel = 0;   /* index in cmdtable of highlighted entry */

	name[0] = '\0';

	while (1) {
		/* Find first match and count all matches for current prefix. */
		int first = -1, nmatches = 0;

		for (i = 0; cmdtable[i].name; i++) {
			if (strncmp(cmdtable[i].name, name, len) == 0) {
				if (first < 0) first = i;
				nmatches++;
			}
		}

		/* Keep sel pointing at a matching entry. */
		if (sel < 0 || !cmdtable[sel].name ||
		    strncmp(cmdtable[sel].name, name, len) != 0)
			sel = first;

		/* Build status line: "M-x <typed>  [sel] other other ..." */
		off = snprintf(msg, sizeof(msg), "M-x %s ", name);
		if (nmatches == 0) {
			off += snprintf(msg + off, sizeof(msg) - off, "[no match]");
		} else {
			for (i = 0; cmdtable[i].name && off < (int)sizeof(msg) - 2; i++) {
				if (strncmp(cmdtable[i].name, name, len) != 0) continue;
				if (i == sel)
					off += snprintf(msg + off, sizeof(msg) - off, "[%s] ", cmdtable[i].name);
				else
					off += snprintf(msg + off, sizeof(msg) - off, "%s ", cmdtable[i].name);
			}
		}
		editor_set_status_message("%s", msg);
		editor_refresh_screen();

		c = editor_read_key(fd);

		if (c == DEL_KEY || c == CTRL_H || c == BACKSPACE) {
			if (len > 0) name[--len] = '\0';
		} else if (c == ESC || c == CTRL_G) {
			editor_set_status_message("");
			return;
		} else if (c == ENTER) {
			editor_set_status_message("");
			if (sel >= 0 && cmdtable[sel].name &&
			    strncmp(cmdtable[sel].name, name, len) == 0)
				cmdtable[sel].fn(fd);
			else
				editor_set_status_message("No command: %s", name);
			return;
		} else if (c == TAB) {
			/* Complete to the longest common prefix of all current matches. */
			if (first >= 0) {
				const char *ref = cmdtable[first].name;
				int clen;

				for (clen = len; ref[clen]; clen++) {
					int ok = 1;

					for (i = 0; cmdtable[i].name && ok; i++) {
						if (strncmp(cmdtable[i].name, name, len) != 0) continue;
						if (cmdtable[i].name[clen] != ref[clen]) ok = 0;
					}
					if (!ok) break;
				}
				if (clen > len && clen < (int)sizeof(name) - 1) {
					strncpy(name, ref, clen);
					name[clen] = '\0';
					len = clen;
				}
			}
		} else if (c == ARROW_RIGHT || c == CTRL_F) {
			/* Cycle forward through matches. */
			int found = 0;

			for (i = sel + 1; cmdtable[i].name; i++) {
				if (strncmp(cmdtable[i].name, name, len) == 0) {
					sel = i; found = 1; break;
				}
			}
			if (!found && first >= 0) sel = first; /* wrap */
		} else if (c == ARROW_LEFT || c == CTRL_B) {
			/* Cycle backward through matches. */
			int prev = -1;

			for (i = 0; cmdtable[i].name && i < sel; i++) {
				if (strncmp(cmdtable[i].name, name, len) == 0) prev = i;
			}
			if (prev < 0) {
				/* wrap to last match */
				for (i = 0; cmdtable[i].name; i++) {
					if (strncmp(cmdtable[i].name, name, len) == 0) prev = i;
				}
			}
			if (prev >= 0) sel = prev;
		} else if (isprint(c) && len < (int)sizeof(name) - 1) {
			name[len++] = c;
			name[len]   = '\0';
		}
	}
}
