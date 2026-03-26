/* stubs.c — global definitions and no-op stubs for symbols not under test */

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

/* Globals normally defined in yank.c */
struct kill_ring killring;

/* No-op stubs for display and kill-ring functions not under test */
void editor_set_status_message(const char *fmt, ...) { (void)fmt; }
void kill_ring_set(char *text, int len)    { (void)text; (void)len; }
void kill_ring_append(char *text, int len) { (void)text; (void)len; }
char *kill_ring_get(void) { return NULL; }
int  editor_read_line(int fd, const char *prompt, char *buf, int bufsize)
{
    (void)fd; (void)prompt; (void)bufsize;
    buf[0] = '\0';
    return 0;
}
