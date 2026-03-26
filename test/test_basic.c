/* test_basic.c — regression tests for editor_goto_line_direct */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "../src/def.h"

/* ---- Helpers ---- */

/* Build a file with `n` rows, each containing "lineNN" (6 chars). */
static void setup(int n)
{
	int i;
	char buf[8];

	free_all_rows();
	memset(&editor, 0, sizeof(editor));
	editor.screenrows = 24;
	editor.screencols = 80;

	for (i = 0; i < n; i++) {
		buf[0] = 'r'; buf[1] = '0' + i / 10; buf[2] = '0' + i % 10;
		buf[3] = '\0';
		editor_insert_row(i, buf, 3);
	}
}

static void teardown(void)
{
	free_all_rows();
	editor.row     = NULL;
	editor.numrows = 0;
}

/* ---- Tests ---- */

/* Goto line 1 in a 10-row file: first row, cursor at column 0, no scroll. */
static void test_goto_line_1(void)
{
	setup(10);

	editor_goto_line_direct(1, 0);

	CHECK(editor.rowoff == 0);
	CHECK(editor.cy     == 0);
	CHECK(editor.cx     == 0);
	CHECK(editor.coloff == 0);
	teardown();
}

/* Goto line 5: still fits on screen without scrolling (screenrows=24). */
static void test_goto_line_5(void)
{
	setup(10);

	editor_goto_line_direct(5, 0);

	/* filerow=4, rowoff=max(0,4-12)=0, cy=4 */
	CHECK(editor.rowoff == 0);
	CHECK(editor.cy     == 4);
	CHECK(editor.cx     == 0);
	teardown();
}

/* Goto the last line of a 10-row file. */
static void test_goto_last_line(void)
{
	setup(10);

	editor_goto_line_direct(10, 0);

	/* filerow=9, rowoff=max(0,9-12)=0, cy=9 */
	CHECK(editor.rowoff == 0);
	CHECK(editor.cy     == 9);
	teardown();
}

/* Line number beyond numrows is clamped to the last line. */
static void test_goto_line_clamp_high(void)
{
	setup(5);

	editor_goto_line_direct(999, 0);

	/* clamped to line 5, filerow=4, rowoff=0, cy=4 */
	CHECK(editor.cy == 4);
	teardown();
}

/* Line 0 and negative are clamped to line 1. */
static void test_goto_line_clamp_low(void)
{
	setup(5);

	editor_goto_line_direct(0, 0);
	CHECK(editor.cy == 0);

	editor_goto_line_direct(-10, 0);
	CHECK(editor.cy == 0);
	teardown();
}

/* col=1 and col=0 both land at column 0. */
static void test_goto_col_one_is_zero(void)
{
	setup(5);

	editor_goto_line_direct(1, 1);
	CHECK(editor.cx == 0);

	editor_goto_line_direct(1, 0);
	CHECK(editor.cx == 0);
	teardown();
}

/* col=3 (1-based) lands at cx=2. */
static void test_goto_col_explicit(void)
{
	setup(5);   /* each row is "rNN" — 3 chars */

	editor_goto_line_direct(1, 3);

	/* filecol = col-1 = 2, within row size=3, cx=2 */
	CHECK(editor.cx == 2);
	teardown();
}

/* col beyond row size is clamped to row size. */
static void test_goto_col_clamp(void)
{
	setup(5);   /* rows have 3 chars */

	editor_goto_line_direct(1, 99);

	/* filecol clamped to row->size=3, cx=3 */
	CHECK(editor.cx == 3);
	teardown();
}

/* For a large file, goto centers the target line vertically.
 * With screenrows=24 and screenrows/2=12, line 20 (filerow=19) gives
 * rowoff=19-12=7, cy=19-7=12. */
static void test_goto_line_centering(void)
{
	setup(30);

	editor_goto_line_direct(20, 0);

	CHECK(editor.rowoff == 7);
	CHECK(editor.cy     == 12);
	teardown();
}

/* Empty file: goto is a no-op and does not crash. */
static void test_goto_line_empty_file(void)
{
	setup(0);

	editor_goto_line_direct(1, 0);   /* must not crash */

	CHECK(editor.numrows == 0);
	teardown();
}

/* ---- Main ---- */

int main(void)
{
	RUN(test_goto_line_1);
	RUN(test_goto_line_5);
	RUN(test_goto_last_line);
	RUN(test_goto_line_clamp_high);
	RUN(test_goto_line_clamp_low);
	RUN(test_goto_col_one_is_zero);
	RUN(test_goto_col_explicit);
	RUN(test_goto_col_clamp);
	RUN(test_goto_line_centering);
	RUN(test_goto_line_empty_file);
	return test_summary();
}
