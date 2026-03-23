/* ============================= Built-in help =============================== */

#include <string.h>
#include "def.h"

static const char *help_lines[] = {
	" kg -- key bindings  (press any key to return)",
	"",
	" MOVEMENT              EDITING               FILE",
	" C-f/C-b   fwd/back    BS        backspace   C-x C-s  save",
	" C-n/C-p   down/up     DEL/C-d   del fwd     C-x C-c  quit",
	" C-a/C-e   bol/eol     C-k       kill line   C-s/C-r  search",
	" C-v/M-v   pgdn/pgup   C-y       yank        C-_      undo",
	" M-f/M-b   word fw/bk  C-w       kill region C-l      redraw",
	" C-HOME    beg file    M-w       copy region",
	" C-END     end file    C-SPC     set mark",
	" C-up/dn   paragraph",
	NULL
};

void editorToggleHelp(void)
{
	E.show_help = !E.show_help;
}

void editorDrawHelp(struct abuf *ab, int nrows)
{
	int nhelp = sizeof(help_lines) / sizeof(help_lines[0]) - 1; /* exclude NULL */
	int i, len;

	for (i = 0; i < nrows; i++) {
		if (i < nhelp) {
			len = strlen(help_lines[i]);
			if (len > E.screencols) len = E.screencols;
			abAppend(ab, help_lines[i], len);
		}
		abAppend(ab, "\x1b[0K\r\n", 6);
	}
}
