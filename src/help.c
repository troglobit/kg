/* ============================= Built-in help =============================== */

#include <string.h>
#include "def.h"

/*
 * 3-column table, 79 chars wide.
 * Each cell: 1 leading space + 9-char key field + 15-char desc field = 25 chars.
 * Row:  │<cell25>│<cell25>│<cell25>│  = 4 + 75 = 79 chars.
 * Sep:  ├<25─>┼<25─>┼<25─>┤          = 79 chars.
 */
static const char *help_lines[] = {
	"┌─ key bindings ─────────────────────────────────── press any key to dismiss ─┐",
	"├─────────────────────────┬─────────────────────────┬─────────────────────────┤",
	"│ NAVIGATION              │ EDITING                 │ FILES & BUFFERS         │",
	"├─────────────────────────┼─────────────────────────┼─────────────────────────┤",
	"│ C-f/C-b  fwd/back       │ BS       del back       │ C-x C-s  save           │",
	"│ C-n/C-p  dn/up          │ C-d/DEL  del fwd        │ C-x s    save all       │",
	"│ C-a/C-e  bol/eol        │ C-k      kill line      │ C-x C-f  open file      │",
	"│ C-v/M-v  page dn/up     │ C-y      yank           │ C-x C-r  open r/o       │",
	"│ M-f/M-b  word fwd/bk    │ C-w      kill region    │ C-x C-q  read-only      │",
	"│ M-d      kill word fwd  │ M-w      copy region    │ C-x C-c  quit           │",
	"│ M-BS     kill word bk   │ C-SPC    set mark       │ C-x b    sel buffer     │",
	"│ C-up/dn  paragraph      │ C-_      undo           │ C-x k    kill buffer    │",
	"│ C-Home   beg of file    │                         │ C-x C-b  list bufs      │",
	"│ C-End    end of file    │                         │ C-x C-w  write file     │",
	"│                         │                         │ C-x i    insert file    │",
	"├─────────────────────────┼─────────────────────────┼─────────────────────────┤",
	"│ SEARCH                  │ MISC                    │ WINDOWS                 │",
	"├─────────────────────────┼─────────────────────────┼─────────────────────────┤",
	"│ C-s      search fwd     │ C-g      cancel         │ C-x 2    split horiz    │",
	"│ C-r      search bk      │ C-l      recenter       │ C-x 3    split vert     │",
	"│ M-%      query replace  │ C-h      help           │ C-x o    other window   │",
	"│ M-;      comment line   │ C-z      suspend        │ C-x 0    del window     │",
	"│                         │ M-g      goto line      │ C-x 1    del others     │",
	"│                         │ M-q      reflow para    │                         │",
	"└─────────────────────────┴─────────────────────────┴─────────────────────────┘",
	NULL
};

void editor_toggle_help(void)
{
	editor.show_help = !editor.show_help;
}

void editor_draw_help(struct abuf *ab, int nrows)
{
	int nhelp = sizeof(help_lines) / sizeof(help_lines[0]) - 1;
	int i;

	for (i = 0; i < nrows; i++) {
		if (i < nhelp) {
			/* UTF-8 aware clip: count display columns, not bytes.
			 * Box-drawing and other multi-byte chars are single-width,
			 * so we count non-continuation bytes as one column each. */
			const char *s = help_lines[i];
			const char *p = s;
			int cols = 0;
			while (*p && cols < editor.screencols) {
				if (((unsigned char)*p & 0xC0) != 0x80) cols++;
				p++;
			}
			ab_append(ab, s, p - s);
		}
		ab_append(ab, "\x1b[0K\r\n", 6);
	}
}
