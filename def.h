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

#define KG_VERSION "0.1.0"

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
	CTRL_P = 16,        /* Ctrl-p */
	CTRL_Q = 17,        /* Ctrl-q */
	CTRL_R = 18,        /* Ctrl-r */
	CTRL_S = 19,        /* Ctrl-s */
	CTRL_U = 21,        /* Ctrl-u */
	CTRL_X = 24,        /* Ctrl-x */
	ESC = 27,           /* Escape */
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
	ALT_F,
	ALT_B
};

/* Syntax highlight definition */
struct editorSyntax {
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
typedef struct hlcolor {
	int r, g, b;
} hlcolor;

/* Editor configuration state */
struct editorConfig {
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
	char statusmsg[80];
	time_t statusmsg_time;
	struct editorSyntax *syntax;    /* Current syntax highlight, or NULL. */
	int cx_prefix;      /* Set to 1 when C-x was pressed, waiting for next key. */
	int paste_mode;     /* If 1, we're in paste mode - disable autocomplete */
	struct timeval last_char_time; /* Time of last character for paste detection */
};

/* Append buffer for efficient screen rendering */
struct abuf {
	char *b;
	int len;
};

/* Global editor state */
extern struct editorConfig E;
extern int running;

/* autocomplete.c */
int editorFindCloseChar(int open_char);
void editorInsertCharAutoComplete(int c);

/* basic.c */
void editorMoveCursor(int key);

/* buffer.c */
void editorUpdateRow(erow *row);
void editorInsertRow(int at, char *s, size_t len);
void editorFreeRow(erow *row);
void editorDelRow(int at);
char *editorRowsToString(int *buflen);
void editorRowInsertChar(erow *row, int at, int c);
void editorRowAppendString(erow *row, char *s, size_t len);
void editorRowDelChar(erow *row, int at);
void editorInsertChar(int c);
void editorInsertNewline(void);
void editorDelChar(void);
void editorKillLine(void);

/* display.c */
void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);
void editorRefreshScreen(void);
void editorSetStatusMessage(const char *fmt, ...);

/* fileio.c */
int editorOpen(char *filename);
int editorSave(void);

/* kbd.c */
void editorProcessKeypress(int fd);

/* search.c */
void editorFind(int fd);

/* syntax.c */
int is_separator(int c);
int editorRowHasOpenComment(erow *row);
void editorUpdateSyntax(erow *row);
int editorSyntaxToColor(int hl);
void editorSelectSyntaxHighlight(char *filename);

/* tty.c */
void disableRawMode(int fd);
void editorAtExit(void);
int enableRawMode(int fd);
int editorReadKey(int fd);
int getCursorPosition(int ifd, int ofd, int *rows, int *cols);
int getWindowSize(int ifd, int ofd, int *rows, int *cols);
void updateWindowSize(void);
void handleSigWinCh(int unused);

/* word.c */
void editorMoveWordForward(void);
void editorMoveWordBackward(void);

/* main.c */
void initEditor(void);
int editorFileWasModified(void);

#endif /* DEF_H */
