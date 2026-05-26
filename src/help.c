/* ============================= Built-in help =============================== */

#include "def.h"

/*
 * 3-column table, 79 chars wide.
 * Each cell: 1 leading space + 9-char key field + 15-char desc field = 25 chars.
 * Row:  │<cell25>│<cell25>│<cell25>│  = 4 + 75 = 79 chars.
 * Sep:  ├<25─>┼<25─>┼<25─>┤          = 79 chars.
 *
 * NULL-terminated.  Lines are loaded verbatim into the *help* buffer by
 * buf_open_help() in bufmgr.c, then rendered through the regular display
 * pipeline so scrolling, mode-line, and 'q' to close come for free.
 */
const char *kg_help_lines[] = {
	"┌─ key bindings ──────────────────────────────────────────────────────────────┐",
	"├─────────────────────────┬─────────────────────────┬─────────────────────────┤",
	"│ NAVIGATION              │ EDITING                 │ FILES & BUFFERS         │",
	"├─────────────────────────┼─────────────────────────┼─────────────────────────┤",
	"│ C-f/C-b  fwd/back char  │ BS       del back       │ C-x C-s  save           │",
	"│ C-n/C-p  down/up line   │ C-d/DEL  del fwd        │ C-x s    save all       │",
	"│ C-a/C-e  bol/eol        │ C-k      kill line      │ C-x C-f  open file      │",
	"│ C-v/M-v  page dn/up     │ C-_      undo           │ C-x C-r  open r/o       │",
	"│ M-f/M-b  word fwd/bk    │ C-o      open line      │ C-x C-w  write file     │",
	"│ M-d/M-BS kill word ±    │ C-q      quoted insert  │ C-x i    insert file    │",
	"│ C-up/dn  paragraph      │ M-^      join line      │ C-x b    sel buffer     │",
	"│ M-</M->  beg/end buf    │ M-u/M-l  up/down word   │ C-x C-b  list bufs      │",
	"│ M-m      to indent      │ M-c      cap word       │ C-x k    kill buffer    │",
	"│ M-a/M-e  sentence ±     │ M-q      reflow para    │ C-x C-q  read-only      │",
	"│ M-r      window line    │ M-;      comment line   │ C-x C-c  quit           │",
	"│ M-g      goto line      │ M-x      named command  │                         │",
	"├─────────────────────────┼─────────────────────────┼─────────────────────────┤",
	"│ REGION & SELECTION      │ RECTANGLES              │ WINDOWS · SEARCH · MISC │",
	"├─────────────────────────┼─────────────────────────┼─────────────────────────┤",
	"│ C-SPC      set mark     │ C-x SPC  rect mark mode │ C-x 2/3  split h / v    │",
	"│ C-x C-x    exch mark    │ C-x r k  kill rect      │ C-x o    other window   │",
	"│ S-arrow    extend       │ C-x r y  yank rect      │ C-x 0/1  del wnd/others │",
	"│ C-w/S-Del  cut          │ C-x r d  delete rect    │ C-s/C-r  search fwd/bk  │",
	"│ M-w/C-Ins  copy         │ C-x r c  clear rect     │ M-%      query replace  │",
	"│ C-y/S-Ins  paste        │                         │ C-l      recenter       │",
	"│ DEL        del region   │ MACROS                  │ C-g      cancel         │",
	"│ M-!/M-|    shell cmd ±  │ C-x (/F3 begin macro    │ C-h      help           │",
	"│                         │ C-x )/F4 end macro      │ C-z      suspend        │",
	"│                         │ C-x e/F4 exec macro     │ C-u      numeric arg    │",
	"└─────────────────────────┴─────────────────────────┴─────────────────────────┘",
	"           M- = Esc/Meta/Alt      C- = Ctrl       S- = Shift",
	NULL
};
