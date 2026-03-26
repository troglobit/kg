/* stubs_noyank.c — globals and stubs for test binaries that link yank.o.
 * Omits killring (defined in yank.c) and kill_ring_* stubs (ditto). */

#include <stdarg.h>
#include "../src/def.h"

/* Globals normally defined in main.c */
struct editor_config editor;
int running       = 1;
int suppress_undo = 0;

/* Globals normally defined in bufmgr.c */
struct editor_buffer buflist[MAX_BUFFERS];
int buf_current = 0;
int buf_count   = 0;

/* Globals normally defined in winmgr.c */
struct editor_window winlist[MAX_WINDOWS];
int win_current    = 0;
int win_count      = 0;
int win_total_rows = 24;
int win_total_cols = 80;

/* No-op stub for display function not under test */
void editor_set_status_message(const char *fmt, ...) { (void)fmt; }
