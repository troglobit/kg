/* test_winmgr.c — regression tests for the window layout.
 *
 * Links winmgr.o alone: the globals it shares with main.c/bufmgr.c and
 * the two functions it calls out to are defined here, not in stubs.c
 * (which owns the winlist globals winmgr.c itself provides). */

#include <stdio.h>
#include <string.h>
#include "test.h"
#include "../src/def.h"

struct editor_config editor;
int running       = 1;
int suppress_undo = 0;

struct editor_buffer buflist[MAX_BUFFERS];
int buf_current = 0;
int buf_count   = 0;

struct undo_stack undostack;

void editor_set_status_message(const char *fmt, ...) { (void)fmt; }
void buf_save_current_state(void) {}
void editor_free_row(erow *row) { (void)row; }   /* for test.c, unused here */

/* ---- Helpers ---- */

static void setup(void)
{
	memset(&editor, 0, sizeof(editor));
	memset(buflist, 0, sizeof(buflist));
	buflist[0].active = 1;
	win_total_rows = 24;
	win_total_cols = 80;
	win_init();
}

/* The window bands (text rows + mode line, columns + divider/right
 * edge) must tile the screen area above the echo row exactly: no
 * overlaps, nothing out of bounds, no gaps. */
static int tiles_ok(void)
{
	int i, j, area = 0;

	for (i = 0; i < MAX_WINDOWS; i++) {
		struct editor_window *a = &winlist[i];

		if (!a->active)
			continue;
		if (a->y < 1 || a->x < 1 || a->h < 1 || a->w < 2)
			return 0;
		if (a->y + a->h > win_total_rows - 1)
			return 0;
		if (a->x + a->w > win_total_cols + 1)
			return 0;
		area += (a->h + 1) * (a->w + 1);
		for (j = i + 1; j < MAX_WINDOWS; j++) {
			struct editor_window *b = &winlist[j];

			if (!b->active)
				continue;
			if (a->y <= b->y + b->h && b->y <= a->y + a->h &&
			    a->x <= b->x + b->w && b->x <= a->x + a->w)
				return 0;
		}
	}
	return area == (win_total_rows - 1) * (win_total_cols + 1);
}

/* ---- Tests ---- */

static void test_init_single_window(void)
{
	setup();
	CHECK(win_count == 1);
	CHECK(winlist[0].y == 1 && winlist[0].x == 1);
	CHECK(winlist[0].h == 22 && winlist[0].w == 80);
	CHECK(tiles_ok());
}

/* C-x 3 after C-x 2 must split only the current (top) window; the
 * bottom window keeps its full width.  Regression: the old col_group
 * layout split the entire terminal instead. */
static void test_split_h_then_v_splits_current_only(void)
{
	int top, right, bottom, i;

	setup();
	top = win_current;
	win_split_horizontal();
	win_split_vertical();

	CHECK(win_count == 3);
	CHECK(tiles_ok());

	right = bottom = -1;
	for (i = 0; i < MAX_WINDOWS; i++) {
		if (!winlist[i].active || i == top)
			continue;
		if (winlist[i].y == winlist[top].y)
			right = i;
		else
			bottom = i;
	}
	CHECK(right >= 0 && bottom >= 0);

	/* The two halves sit side by side in the old top band... */
	CHECK(winlist[right].h == winlist[top].h);
	CHECK(winlist[right].x == winlist[top].x + winlist[top].w + 1);
	/* ...and the bottom window still spans the full terminal width. */
	CHECK(winlist[bottom].w == 80);
	CHECK(winlist[bottom].x == 1);
}

/* The transpose: C-x 2 after C-x 3 splits only the left window; the
 * right window keeps its full height. */
static void test_split_v_then_h_splits_current_only(void)
{
	int left, right, below, i;

	setup();
	left = win_current;
	win_split_vertical();
	win_split_horizontal();

	CHECK(win_count == 3);
	CHECK(tiles_ok());

	right = below = -1;
	for (i = 0; i < MAX_WINDOWS; i++) {
		if (!winlist[i].active || i == left)
			continue;
		if (winlist[i].x == winlist[left].x)
			below = i;
		else
			right = i;
	}
	CHECK(right >= 0 && below >= 0);

	CHECK(winlist[right].h == 22);
	CHECK(winlist[below].w == winlist[left].w);
	CHECK(winlist[below].y == winlist[left].y + winlist[left].h + 1);
}

/* Deleting the bottom full-width window gives its rows to both top
 * halves; deleting one half gives its columns to the other. */
static void test_delete_absorbs_space(void)
{
	setup();
	win_split_horizontal();
	win_split_vertical();

	win_move_dir(0, 1);          /* into the bottom window */
	win_delete_current();
	CHECK(win_count == 2);
	CHECK(tiles_ok());

	win_delete_current();        /* one of the side-by-side halves */
	CHECK(win_count == 1);
	CHECK(tiles_ok());
	CHECK(winlist[win_current].h == 22 && winlist[win_current].w == 80);
}

/* M-S-arrow moves the divider; it stops at the minimum window size. */
static void test_resize_moves_divider_and_clamps(void)
{
	int i, top;

	setup();
	top = win_current;
	win_split_horizontal();
	CHECK(winlist[top].h == 10);

	win_resize_dir(0, 1, 3);     /* divider down: current grows */
	CHECK(winlist[top].h == 13);
	CHECK(tiles_ok());

	for (i = 0; i < 30; i++)     /* shrink far past the minimum */
		win_resize_dir(0, -1, 1);
	CHECK(winlist[top].h == 2);
	CHECK(tiles_ok());
}

/* C-x + evens the heights back out after a resize. */
static void test_balance_evens_out(void)
{
	int top;

	setup();
	top = win_current;
	win_split_horizontal();
	win_resize_dir(0, 1, 5);
	CHECK(winlist[top].h == 15);

	win_balance();
	CHECK(winlist[top].h == 10);
	CHECK(tiles_ok());
}

/* A terminal resize keeps the other windows' sizes: the current
 * window absorbs the delta. */
static void test_term_resize_absorbs_in_current(void)
{
	int top, bottom;

	setup();
	top = win_current;
	win_split_horizontal();
	win_move_dir(0, 1);
	bottom = win_current;
	CHECK(winlist[top].h == 10 && winlist[bottom].h == 11);

	win_total_rows = 34;         /* grow the terminal by ten rows */
	win_term_resize();
	CHECK(tiles_ok());
	CHECK(winlist[top].h == 10);
	CHECK(winlist[bottom].h == 21);

	win_total_rows = 24;         /* and back */
	win_term_resize();
	CHECK(tiles_ok());
	CHECK(winlist[top].h == 10);
	CHECK(winlist[bottom].h == 11);
}

/* ---- Main ---- */

int main(void)
{
	RUN(test_init_single_window);
	RUN(test_split_h_then_v_splits_current_only);
	RUN(test_split_v_then_h_splits_current_only);
	RUN(test_delete_absorbs_space);
	RUN(test_resize_moves_divider_and_clamps);
	RUN(test_balance_evens_out);
	RUN(test_term_resize_absorbs_in_current);
	return test_summary();
}
