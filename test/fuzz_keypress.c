#include "../src/def.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void fuzz_set_input(const unsigned char *data, size_t len);
int fuzz_input_fd(void);
void fuzz_clear_input(void);

static void free_rows(void)
{
	int i;

	for (i = 0; i < editor.numrows; i++) {
		editor_free_row(&editor.row[i]);
	}
	free(editor.row);
	editor.row = NULL;
	editor.numrows = 0;
}

static void reset_state(void)
{
	memset(&editor, 0, sizeof(editor));
	memset(buflist, 0, sizeof(buflist));
	memset(winlist, 0, sizeof(winlist));
	running = 1;
	suppress_undo = 0;
	global_auto_revert = 0;
	buf_current = 0;
	buf_count = 1;
	win_current = 0;
	win_count = 1;
	win_total_rows = 24;
	win_total_cols = 80;
	editor.screenrows = 22;
	editor.screencols = 80;
	editor.desired_visual_col = -1;
	editor.filename = strdup("fuzz.txt");
	winlist[0].active = 1;
	winlist[0].bufidx = 0;
	winlist[0].y = 1;
	winlist[0].x = 1;
	winlist[0].h = 22;
	winlist[0].w = 80;
	kill_ring_init();
	undo_init();
	macro_reset();
}

static void seed_buffer(const uint8_t *data, size_t size)
{
	size_t i;
	char line[64];
	int len = 0;

	for (i = 0; i < size; i++) {
		unsigned char ch = data[i];

		if ((ch & 0x0f) == 0) {
			editor_insert_row(editor.numrows, line, len);
			len = 0;
			continue;
		}
		if ((ch & 0x03) == 0) {
			ch = '\t';
		} else {
			ch = ' ' + (ch % 95);
		}
		if (len < (int)sizeof(line) - 1) {
			line[len++] = (char)ch;
		}
	}
	if (len > 0 || editor.numrows == 0) {
		editor_insert_row(editor.numrows, line, len);
	}
	editor.dirty = 0;
	undo_mark_clean();
	buflist[0].active = 1;
	buflist[0].cx = editor.cx;
	buflist[0].cy = editor.cy;
	buflist[0].rowoff = editor.rowoff;
	buflist[0].coloff = editor.coloff;
	buflist[0].numrows = editor.numrows;
	buflist[0].row = editor.row;
	buflist[0].dirty = editor.dirty;
	buflist[0].filename = editor.filename;
	buflist[0].syntax = editor.syntax;
}

static void teardown_state(void)
{
	fuzz_clear_input();
	undo_free();
	rect_kill_ring_free();
	kill_ring_free();
	free(editor.filename);
	editor.filename = NULL;
	free_rows();
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	size_t seed_len;
	int fd;

	reset_state();
	seed_len = size > 32 ? size / 4 : size;
	seed_buffer(data, seed_len);
	fuzz_set_input(data + seed_len, size - seed_len);
	fd = fuzz_input_fd();

	while (running) {
		editor_process_keypress(fd);
	}

	teardown_state();
	return 0;
}
