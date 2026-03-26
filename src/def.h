/* kg -- A very simple editor in less than 1-kilo lines of code (as counted
 *       by "cloc"). Does not depend on libcurses, directly emits VT100
 *       escapes on the terminal.
 *
 * -----------------------------------------------------------------------
 *
 * Copyright (C) 2016 Salvatore Sanfilippo <antirez at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DEF_H
#define DEF_H

#define KG_VERSION "1.0.0"

#if defined(__linux__) || defined(__CYGWIN__)
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#endif

#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h>

/* Write escape sequences to stdout; silently discard errors (best-effort). */
static inline void tty_write(const void *buf, size_t n)
{
	ssize_t r = write(STDOUT_FILENO, buf, n);
	(void)r;
}

/* Syntax highlight types */
#define HL_NORMAL 0
#define HL_NONPRINT 1
#define HL_COMMENT 2   /* Single line comment. */
#define HL_MLCOMMENT 3 /* Multi-line comment. */
#define HL_KEYWORD1 4
#define HL_KEYWORD2 5
#define HL_STRING 6
#define HL_NUMBER 7
#define HL_MATCH 8      /* Search match. */

#define HL_HIGHLIGHT_STRINGS (1<<0)
#define HL_HIGHLIGHT_NUMBERS (1<<1)
#define SHL_MARKDOWN         (1<<2) /* Use markdown-specific highlighter. */
#define SHL_MAKEFILE         (1<<3) /* Use makefile-specific highlighter. */

/* Key action codes */
enum KEY_ACTION {
	KEY_NULL = 0,       /* NULL */
	CTRL_A = 1,         /* Ctrl-a */
	CTRL_B = 2,         /* Ctrl-b */
	CTRL_C = 3,         /* Ctrl-c */
	CTRL_D = 4,         /* Ctrl-d */
	CTRL_E = 5,         /* Ctrl-e */
	CTRL_F = 6,         /* Ctrl-f */
	CTRL_G = 7,         /* Ctrl-g */
	CTRL_H = 8,         /* Ctrl-h */
	TAB = 9,            /* Tab */
	CTRL_K = 11,        /* Ctrl-k */
	CTRL_L = 12,        /* Ctrl+l */
	ENTER = 13,         /* Enter */
	CTRL_N = 14,        /* Ctrl-n */
	CTRL_O = 15,        /* Ctrl-o */
	CTRL_P = 16,        /* Ctrl-p */
	CTRL_Q = 17,        /* Ctrl-q */
	CTRL_R = 18,        /* Ctrl-r */
	CTRL_S = 19,        /* Ctrl-s */
	CTRL_U = 21,        /* Ctrl-u */
	CTRL_V = 22,        /* Ctrl-v */
	CTRL_W = 23,        /* Ctrl-w */
	CTRL_X = 24,        /* Ctrl-x */
	CTRL_Y = 25,        /* Ctrl-y */
	CTRL_Z = 26,        /* Ctrl-z */
	ESC = 27,           /* Escape */
	CTRL_UNDERSCORE = 31, /* Ctrl-_ or Ctrl-/ (undo) */
	BACKSPACE =  127,   /* Backspace */
	/* The following are just soft codes, not really reported by the
	 * terminal directly. */
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN,
	CTRL_ARROW_LEFT,
	CTRL_ARROW_RIGHT,
	CTRL_ARROW_UP,
	CTRL_ARROW_DOWN,
	CTRL_HOME,
	CTRL_END,
	ALT_F,
	ALT_B,
	ALT_D,
	ALT_G,
	ALT_V,
	ALT_W,
	ALT_Q,
	ALT_BACKSPACE,
	ALT_PCT,       /* M-% query-replace */
	ALT_SEMICOLON, /* M-; comment-dwim */
	ALT_X,         /* M-x named command */
	ALT_CARET,     /* M-^ join-line */
	ALT_U,         /* M-u upcase-word */
	ALT_L,         /* M-l downcase-word */
	ALT_C,         /* M-c capitalize-word */
	KEY_F3,        /* F3: start keyboard macro */
	KEY_F4         /* F4: stop or replay keyboard macro */
};

/* Syntax highlight definition */
struct editor_syntax {
	char *name;         /* Display name shown in mode line, e.g. "C", "Python" */
	char **filematch;
	char **keywords;
	char singleline_comment_start[5];
	char multiline_comment_start[5];
	char multiline_comment_end[5];
	int flags;
};

/* This structure represents a single line of the file we are editing. */
typedef struct erow {
	int idx;            /* Row index in the file, zero-based. */
	int size;           /* Size of the row, excluding the null term. */
	int rsize;          /* Size of the rendered row. */
	char *chars;        /* Row content. */
	char *render;       /* Row content "rendered" for screen (for TABs). */
	unsigned char *hl;  /* Syntax highlight type for each character in render.*/
	int hl_oc;          /* Row had open comment at end in last syntax highlight
	                       check. */
} erow;

/* Highlight color */
typedef struct hl_color {
	int r, g, b;
} hl_color;

/* Editor configuration state */
struct editor_config {
	int cx, cy;         /* Cursor x and y position in characters */
	int rowoff;         /* Offset of row displayed. */
	int coloff;         /* Offset of column displayed. */
	int screenrows;     /* Number of rows that we can show */
	int screencols;     /* Number of cols that we can show */
	int numrows;        /* Number of rows */
	int rawmode;        /* Is terminal raw mode enabled? */
	erow *row;          /* Rows */
	int dirty;          /* File modified but not saved. */
	char *filename;     /* Currently open filename */
	char statusmsg[160];
	time_t statusmsg_time;
	struct editor_syntax *syntax;    /* Current syntax highlight, or NULL. */
	int cx_prefix;      /* Set to 1 when C-x was pressed, waiting for next key. */
	int paste_mode;     /* If 1, we're in paste mode - disable autocomplete */
	struct timeval last_char_time; /* Time of last character for paste detection */
	int mark_set;       /* Is mark set for region selection? */
	int mark_row;       /* Mark row position */
	int mark_col;       /* Mark column position */
	int show_help;      /* If 1, display help screen instead of file content. */
	int readonly;       /* If 1, buffer is read-only (editing is blocked). */
	int last_key;       /* Last key processed, for command repetition logic. */
	int recenter_state; /* Cycle state for C-l: 0=center, 1=top, 2=bottom. */
};

/* Append buffer for efficient screen rendering */
struct abuf {
	char *b;
	int len;
};

/* Kill ring (yank buffer) for copy/paste operations */
struct kill_ring {
	char *text;         /* Killed/copied text */
	int len;            /* Length of text */
};

/* Undo operation types */
enum undo_type {
	UNDO_INSERT_CHAR,
	UNDO_DELETE_CHAR,
	UNDO_INSERT_LINE,
	UNDO_DELETE_LINE,
	UNDO_SPLIT_LINE,
	UNDO_JOIN_LINE,
	UNDO_KILL_TEXT,   /* Kill line or region */
	UNDO_YANK_TEXT,   /* Yank (paste) */
	UNDO_REFLOW_PARA  /* M-q paragraph reflow */
};

/* Single undo operation */
struct undo_op {
	enum undo_type type;
	int row;            /* Row where operation occurred */
	int col;            /* Column where operation occurred */
	int c;              /* Character (for char operations) */
	char *text;         /* Line text (for line operations) */
	int len;            /* Length of text */
	struct undo_op *next;
};

/* Undo stack */
struct undo_stack {
	struct undo_op *head;
	int size;
	int max_size;
	int clean_size;  /* Stack size at last save (-1 if never saved clean) */
};

/* Per-window viewport state. */
#define MAX_WINDOWS 8
struct editor_window {
	int bufidx;         /* Which buffer this window shows */
	int cx, cy;         /* Cursor position within window */
	int rowoff, coloff; /* Scroll offsets */
	int y, x;           /* Top-left corner on terminal (1-based) */
	int h, w;           /* Height (text rows) and width (cols) of this window */
	int active;         /* 1 if this slot is in use */
	int col_group;      /* Column group: windows with same value stack vertically;
	                       different values sit side-by-side */
};

/* Per-buffer state saved when switching away from a buffer. */
#define MAX_BUFFERS 20
struct editor_buffer {
	int cx, cy;
	int rowoff, coloff;
	int numrows;
	erow *row;
	int dirty;
	char *filename;
	struct editor_syntax *syntax;
	int mark_set;
	int mark_row, mark_col;
	struct undo_stack undostack; /* per-buffer undo chain */
	int active;                 /* 1 if this slot is in use */
	int readonly;               /* 1 if buffer is read-only */
};

/* Global editor state */
extern struct editor_config editor;
extern int running;
extern int suppress_undo;
extern struct kill_ring killring;
extern struct undo_stack undostack;
extern struct editor_buffer buflist[MAX_BUFFERS];
extern int buf_current; /* index into buflist[] of the active buffer */
extern int buf_count;   /* number of active buffers */

extern struct editor_window winlist[MAX_WINDOWS];
extern int win_current;     /* index into winlist[] of the active window */
extern int win_count;       /* number of active windows */
extern int win_total_rows;  /* terminal rows (set by update_window_size) */
extern int win_total_cols;  /* terminal cols (set by update_window_size) */

/* bufmgr.c */
void buf_save_current_state(void);
int  editor_read_line(int fd, const char *prompt, char *buf, int bufsize);
void buf_load_args(int nfiles, char **filenames, int readonly);
void buf_select_interactive(int fd);
void buf_open_file(int fd);
void buf_open_file_read_only(int fd);
void buf_kill(int fd);
void buf_save_all(int fd);
void buf_open_list(void);
void buf_ibuffer_select(void);

/* winmgr.c */
void win_init(void);
void win_reflow(void);
void win_save_active_view(void);
void win_restore_active_view(void);
void win_split_horizontal(void);
void win_split_vertical(void);
void win_cycle_next(void);
void win_delete_current(void);
void win_delete_others(void);

/* autocomplete.c */
int editor_find_close_char(int open_char);
void editor_insert_char_auto_complete(int c);

/* basic.c */
void editor_move_cursor(int key);
void editor_move_to_beginning(void);
void editor_move_to_end(void);
void editor_goto_line_direct(int line, int col);
void editor_goto_line(int fd);

/* buffer.c */
void editor_update_row(erow *row);
void editor_insert_row(int at, char *s, size_t len);
void editor_free_row(erow *row);
void editor_del_row(int at);
char *editor_rows_to_string(erow *rows, int numrows, int *buflen);
void editor_row_insert_char(erow *row, int at, int c);
void editor_row_append_string(erow *row, char *s, size_t len);
void editor_row_del_char(erow *row, int at);
void editor_insert_char(int c);
void editor_insert_newline_raw(void);
void editor_insert_text_raw(const char *text, int len);
void editor_insert_newline(void);
void editor_open_line(void);
void editor_del_char(void);
void editor_del_forward_char(void);
void editor_kill_line(void);

/* Returns 1 if filename belongs to a special/system buffer (NULL or starts with '*'). */
static inline int is_special_buffer(const char *filename)
{
	return !filename || filename[0] == '*';
}

/* help.c */
void editor_toggle_help(void);
void editor_draw_help(struct abuf *ab, int nrows);

/* display.c */
void ab_append(struct abuf *ab, const char *s, int len);
void ab_free(struct abuf *ab);
void editor_refresh_screen(void);
void editor_set_status_message(const char *fmt, ...);

/* fileio.c */
int editor_open(char *filename);
int editor_save(int fd);
void editor_write_file(int fd);
void editor_insert_file(int fd);

/* kbd.c */
void editor_process_keypress(int fd);

/* cmd.c */
void editor_named_command(int fd);

/* macro.c */
int  macro_is_recording(void);
void macro_on_key(int key);
int  macro_next_key(void);
void macro_start(void);
void macro_stop(int trim);
void macro_replay(int fd);

/* search.c */
void editor_find(int fd);
void editor_query_replace(int fd);

/* syntax.c */
int is_separator(int c);
int editor_row_has_open_comment(erow *row);
void editor_update_syntax(erow *row);
int editor_syntax_to_color(int hl);
void editor_select_syntax_highlight(char *filename);

/* tty.c */
void disable_raw_mode(int fd);
void editor_at_exit(void);
int enable_raw_mode(int fd);
void editor_suspend(void);
int editor_read_key(int fd);
int get_cursor_position(int ifd, int ofd, int *rows, int *cols);
int get_window_size(int ifd, int ofd, int *rows, int *cols);
void update_window_size(void);
void probe_window_size(void);
void handle_sig_winch(int unused);

/* word.c */
void editor_move_word_forward(void);
void editor_move_word_backward(void);
void editor_move_paragraph_forward(void);
void editor_move_paragraph_backward(void);
void editor_kill_word_forward(void);
void editor_kill_word_backward(void);
void editor_join_line(void);
void editor_upcase_word(void);
void editor_downcase_word(void);
void editor_capitalize_word(void);
void editor_reflow_paragraph(void);
void editor_comment_dwim(void);

/* yank.c */
void kill_ring_init(void);
void kill_ring_free(void);
void kill_ring_set(char *text, int len);
void kill_ring_append(char *text, int len);
char *kill_ring_get(void);
void editor_set_mark(void);
void editor_exchange_point_and_mark(void);
void editor_kill_region(void);
void editor_copy_region(void);
void editor_yank(void);

/* undo.c */
void undo_init(void);
void undo_free(void);
void undo_push(enum undo_type type, int row, int col, int c, char *text, int len);
void editor_undo(void);
void undo_mark_clean(void);

/* main.c */
void init_editor(void);

#endif /* DEF_H */
