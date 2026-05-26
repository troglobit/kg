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
	int  match_idx[PICKER_MAX_ENTRIES];
	const char *names[PICKER_MAX_ENTRIES];
	int  len = 0, c, i, off;
	int  sel = 0;   /* index within match_idx[] of the highlighted entry */

	name[0] = '\0';

	while (1) {
		int total = 0, shown, first_cmd = -1;

		/* Prefix matches first, then mid-name substring matches.
		 * Two passes preserve cmdtable's alphabetical order within
		 * each rank, and keep TAB-completion's longest-common-prefix
		 * over the prefix group only (substring matches share no
		 * prefix worth extending to). */
		for (i = 0; cmdtable[i].name; i++) {
			if (editor_picker_match_rank(cmdtable[i].name, name) != 0)
				continue;
			if (first_cmd < 0) first_cmd = i;
			if (total < PICKER_MAX_ENTRIES) {
				match_idx[total] = i;
				names[total]     = cmdtable[i].name;
			}
			total++;
		}
		/* Skip the substring pass when nothing's typed: every entry
		 * already ranked as a prefix match above, so a second scan
		 * looking for rank==1 would just walk the table for nothing. */
		if (len > 0) {
			for (i = 0; cmdtable[i].name; i++) {
				if (editor_picker_match_rank(cmdtable[i].name, name) != 1)
					continue;
				if (total < PICKER_MAX_ENTRIES) {
					match_idx[total] = i;
					names[total]     = cmdtable[i].name;
				}
				total++;
			}
		}
		shown = total > PICKER_MAX_ENTRIES ? PICKER_MAX_ENTRIES : total;
		if (sel >= shown) sel = shown > 0 ? shown - 1 : 0;

		off = 0;
		editor_msg_appendf(msg, sizeof(msg), &off, "%s%s ", prompt, name);
		editor_picker_render(msg, sizeof(msg), &off, names, shown, total, sel);

		editor_set_status_message("%s", msg);
		editor.echo_cursor_col = plen + len + 1;
		editor_refresh_screen();

		c = editor_read_key(fd);

		if (c == DEL_KEY || c == CTRL_H || c == BACKSPACE) {
			if (len > 0) name[--len] = '\0';
			sel = 0;
		} else if (c == ESC || c == CTRL_G) {
			editor.echo_cursor_col = 0;
			editor_set_status_message("");
			return;
		} else if (c == ENTER) {
			editor.echo_cursor_col = 0;
			editor_set_status_message("");
			if (shown > 0 && sel >= 0 && sel < shown)
				cmdtable[match_idx[sel]].fn(fd);
			else
				editor_set_status_message("No command: %s", name);
			return;
		} else if (c == TAB) {
			/* Complete to the longest common prefix of the prefix-
			 * matched group only — substring matches share no
			 * leading text worth extending typed input to. */
			if (first_cmd >= 0) {
				const char *ref = cmdtable[first_cmd].name;
				int clen;

				for (clen = len; ref[clen]; clen++) {
					int ok = 1;

					for (i = 0; cmdtable[i].name && ok; i++) {
						if (editor_picker_match_rank(cmdtable[i].name, name) != 0)
							continue;
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
			sel = 0;
		} else if (c == ARROW_RIGHT || c == CTRL_F) {
			if (shown > 0) sel = (sel + 1) % shown;
		} else if (c == ARROW_LEFT || c == CTRL_B) {
			if (shown > 0) sel = (sel - 1 + shown) % shown;
		} else if (isprint(c) && len < (int)sizeof(name) - 1) {
			name[len++] = c;
			name[len]   = '\0';
			sel = 0;
		}
	}
}
