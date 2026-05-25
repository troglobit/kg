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

/* Re-read the current file from disk, discarding all unsaved changes. */
static void cmd_revert_buffer(int fd)
{
	int answer;

	if (is_special_buffer(editor.filename)) {
		editor_set_status_message("Cannot revert a special buffer");
		return;
	}
	if (editor.dirty) {
		editor_set_status_message("Buffer modified.  Revert from disk? (y/n) ");
		editor_refresh_screen();
		answer = editor_read_key(fd);
		if (answer != 'y' && answer != 'Y') {
			editor_set_status_message("");
			return;
		}
	}

	editor.cx = editor.cy = editor.rowoff = editor.coloff = 0;
	buf_reload_from_disk();
	editor_set_status_message("Reverted %s", editor.filename);
}

/* Join the current line with the previous one (M-^). */
static void cmd_join_line(int fd)
{
	(void)fd;
	editor_join_line();
}

/* Upcase, downcase, capitalize word forward from point. */
static void cmd_upcase_word(int fd)    { (void)fd; editor_upcase_word();     }
static void cmd_downcase_word(int fd)  { (void)fd; editor_downcase_word();   }
static void cmd_capitalize_word(int fd){ (void)fd; editor_capitalize_word(); }

/* Shell command (M-!) and shell-command-on-region (M-|). */
static void cmd_shell_command(int fd)           { editor_shell_command(fd);            }
static void cmd_shell_command_on_region(int fd) { editor_shell_command_on_region(fd);  }

/* Toggle auto-revert on the current buffer.  When on (or when the global
 * setting below is on), a clean buffer whose underlying file has changed on
 * disk is silently reloaded by the next poll. */
static void cmd_auto_revert_mode(int fd)
{
	(void)fd;
	editor.auto_revert = !editor.auto_revert;
	editor_set_status_message("Auto-revert for %s is %s",
	                          buf_basename(editor.filename),
	                          editor.auto_revert ? "on" : "off");
}

/* Toggle auto-revert for every buffer at once. */
static void cmd_global_auto_revert_mode(int fd)
{
	(void)fd;
	global_auto_revert = !global_auto_revert;
	editor_set_status_message("Global auto-revert is %s",
	                          global_auto_revert ? "on" : "off");
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
	{ "auto-revert-mode",         cmd_auto_revert_mode         },
	{ "capitalize-word",          cmd_capitalize_word          },
	{ "delete-trailing-space",    cmd_delete_trailing_space    },
	{ "downcase-word",            cmd_downcase_word            },
	{ "global-auto-revert-mode",  cmd_global_auto_revert_mode  },
	{ "goto-line",                cmd_goto_line                },
	{ "join-line",                cmd_join_line                },
	{ "not-modified",             cmd_not_modified             },
	{ "revert-buffer",            cmd_revert_buffer            },
	{ "save-buffer",              cmd_save_buffer              },
	{ "shell-command",            cmd_shell_command            },
	{ "shell-command-on-region",  cmd_shell_command_on_region  },
	{ "toggle-read-only",         cmd_toggle_read_only         },
	{ "upcase-word",              cmd_upcase_word              },
	{ "version",                  cmd_version                  },
	{ "what-cursor-position",     cmd_what_cursor_position     },
	{ "whitespace-cleanup",       cmd_whitespace_cleanup       },
	{ NULL, NULL }
};

/* Prompt "M-x", filter by typing, Tab-complete, Left/Right cycle, Enter execute. */
void editor_named_command(int fd)
{
	const char prompt[] = "M-x ";
	const int  plen     = sizeof(prompt) - 1;
	char name[64];
	char msg[512];
	int  match_idx[64];
	int  len = 0, c, i, off;
	int  sel = 0;   /* index in cmdtable of highlighted entry */

	name[0] = '\0';

	while (1) {
		/* One pass: find first matching index, count matches, record
		 * each match's cmdtable position, and locate sel within the
		 * match sequence. */
		int first = -1, nmatches = 0, sel_pos = -1;
		int budget, used, win_start, win_end, j;

		for (i = 0; cmdtable[i].name; i++) {
			if (strncmp(cmdtable[i].name, name, len) != 0) continue;
			if (first < 0)  first = i;
			if (i == sel)   sel_pos = nmatches;
			if (nmatches < (int)(sizeof(match_idx)/sizeof(match_idx[0])))
				match_idx[nmatches] = i;
			nmatches++;
		}

		/* Keep sel pointing at a matching entry. */
		if (sel < 0 || !cmdtable[sel].name ||
		    strncmp(cmdtable[sel].name, name, len) != 0) {
			sel = first;
			sel_pos = (first >= 0) ? 0 : -1;
		}

		/* "M-x <typed>  <maybe ...> ... <sel(reverse)> ... " */
		off = snprintf(msg, sizeof(msg), "%s%s  ", prompt, name);
		if (nmatches == 0) {
			off += snprintf(msg + off, sizeof(msg) - off, "[no match]");
		} else {
			/* Window the match list around sel so it stays on
			 * screen even after the user cycles past the right
			 * edge.  Budget is whatever space the prompt leaves
			 * us on the status row. */
			budget = win_total_cols - off;
			if (budget < 0) budget = 0;
			used      = 0;
			win_start = sel_pos;
			win_end   = sel_pos;

			for (j = sel_pos; j < nmatches; j++) {
				int w = (int)strlen(cmdtable[match_idx[j]].name) + 1;
				if (j > sel_pos && used + w > budget) break;
				used   += w;
				win_end = j + 1;
			}
			for (j = sel_pos - 1; j >= 0; j--) {
				int w = (int)strlen(cmdtable[match_idx[j]].name) + 1;
				if (used + w > budget) break;
				win_start = j;
				used     += w;
			}

			if (win_start > 0)
				off += snprintf(msg + off, sizeof(msg) - off, "... ");
			for (i = win_start; i < win_end; i++) {
				int ci = match_idx[i];
				if (ci == sel)
					off += snprintf(msg + off, sizeof(msg) - off,
					                "\x1b[7m%s\x1b[27m ", cmdtable[ci].name);
				else
					off += snprintf(msg + off, sizeof(msg) - off,
					                "%s ", cmdtable[ci].name);
			}
		}
		editor_set_status_message("%s", msg);
		editor.echo_cursor_col = plen + len + 1;
		editor_refresh_screen();

		c = editor_read_key(fd);

		if (c == DEL_KEY || c == CTRL_H || c == BACKSPACE) {
			if (len > 0) name[--len] = '\0';
		} else if (c == ESC || c == CTRL_G) {
			editor.echo_cursor_col = 0;
			editor_set_status_message("");
			return;
		} else if (c == ENTER) {
			editor.echo_cursor_col = 0;
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
