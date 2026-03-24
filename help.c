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
	"│ C-f/C-b  fwd/back       │ BS/C-d   del back       │ C-x C-s  save           │",
	"│ C-n/C-p  dn/up          │ DEL      del fwd        │ C-x s    save all       │",
	"│ C-a/C-e  bol/eol        │ C-k      kill line      │ C-x C-f  open file      │",
	"│ C-v/M-v  page dn/up     │ C-y      yank           │ C-x C-r  open r/o       │",
	"│ M-f/M-b  word fw/bk     │ C-w      kill region    │ C-x C-q  read-only      │",
	"│ C-up/dn  paragraph      │ M-w      copy region    │ C-x C-c  quit           │",
	"│ C-Home   beg of file    │ C-SPC    set mark       │ C-x b    sel buffer     │",
	"│ C-End    end of file    │ C-_      undo           │ C-x k    kill buffer    │",
	"│ M-g      goto line      │                         │ C-x C-b  list bufs      │",
	"├─────────────────────────┼─────────────────────────┼─────────────────────────┤",
	"│ SEARCH                  │ MISC                    │ WINDOWS                 │",
	"├─────────────────────────┼─────────────────────────┼─────────────────────────┤",
	"│ C-s      search fwd     │ C-g      cancel         │ C-x 2    split horiz    │",
	"│ C-r      search bk      │ C-l      redraw         │ C-x 3    split vert     │",
	"│                         │ C-h      help           │ C-x o    other window   │",
	"│                         │ C-z      suspend        │ C-x 0    del window     │",
	"│                         │                         │ C-x 1    del others     │",
	"└─────────────────────────┴─────────────────────────┴─────────────────────────┘",
	NULL
};

void editorToggleHelp(void)
{
	E.show_help = !E.show_help;
}

void editorDrawHelp(struct abuf *ab, int nrows)
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
			while (*p && cols < E.screencols) {
				if (((unsigned char)*p & 0xC0) != 0x80) cols++;
				p++;
			}
			abAppend(ab, s, p - s);
		}
		abAppend(ab, "\x1b[0K\r\n", 6);
	}
}
