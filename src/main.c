/* Kilo -- A very simple editor in less than 1-kilo lines of code (as counted
 *         by "cloc"). Does not depend on libcurses, directly emits VT100
 *         escapes on the terminal.
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

#include "def.h"

struct editor_config editor;
int running = 1;
int suppress_undo = 0;

void init_editor(void)
{
	editor.cx = 0;
	editor.cy = 0;
	editor.rowoff = 0;
	editor.coloff = 0;
	editor.numrows = 0;
	editor.row = NULL;
	editor.dirty = 0;
	editor.filename = NULL;
	editor.syntax = NULL;
	editor.cx_prefix = 0;
	editor.paste_mode = 0;
	editor.mark_set = 0;
	editor.mark_row = 0;
	editor.mark_col = 0;
	editor.readonly = 0;
	gettimeofday(&editor.last_char_time, NULL);
	kill_ring_init();
	undo_init();
	update_window_size();
	win_init();
	signal(SIGWINCH, handle_sig_winch);
}

static int usage(FILE *fp, int rc)
{
	fprintf(fp, "Usage: kg [-RVh] [file ...]\n"
	            "\n"
	            "Options:\n"
	            "  -R  Open file(s) read-only\n"
	            "  -V  Print version and exit\n"
	            "  -h  Print this help and exit\n");
	return rc;
}

int main(int argc, char **argv)
{
	int opt, readonly = 0;

	while ((opt = getopt(argc, argv, "RVh")) != -1) {
		switch (opt) {
		case 'R':
			readonly = 1;
			break;
		case 'V':
			printf("kg %s\n", KG_VERSION);
			return 0;
		case 'h':
			return usage(stdout, 0);
		default:
			return usage(stderr, 1);
		}
	}

	init_editor();
	buf_load_args(argc - optind, argv + optind, readonly);
	enable_raw_mode(STDIN_FILENO);
	editor_set_status_message(
		"HELP: C-x C-s = save | C-x C-c = quit | C-s = search | C-k = kill line | C-h = help");
	while (running) {
		editor_refresh_screen();
		editor_process_keypress(STDIN_FILENO);
	}
	return 0;
}
