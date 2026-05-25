/* test_buffer.c — regression tests for row-level buffer operations */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "../src/def.h"

/* ---- Helpers ---- */

static void setup(void)
{
	free_all_rows();
	memset(&editor, 0, sizeof(editor));
	editor.screenrows = 24;
	editor.screencols = 80;
}

static void teardown(void)
{
	free_all_rows();
	editor.row     = NULL;
	editor.numrows = 0;
}

/* ---- Tests ---- */

/* Three rows joined produce "line1\nline2\nline3\n". */
static void test_rows_to_string(void)
{
	char *s;
	int len;

	setup();
	editor_insert_row(0, "line1", 5);
	editor_insert_row(1, "line2", 5);
	editor_insert_row(2, "line3", 5);

	s = editor_rows_to_string(editor.row, editor.numrows, &len);

	CHECK(len == 18);   /* (5+1) * 3 */
	CHECK(memcmp(s, "line1\nline2\nline3\n", 18) == 0);
	free(s);
	teardown();
}

/* Single empty row produces "\n". */
static void test_rows_to_string_empty_row(void)
{
	char *s;
	int len;

	setup();
	editor_insert_row(0, "", 0);

	s = editor_rows_to_string(editor.row, editor.numrows, &len);

	CHECK(len == 1);
	CHECK(s[0] == '\n');
	free(s);
	teardown();
}

/* Inserting in the middle shifts chars right. */
static void test_row_insert_char_middle(void)
{
	setup();
	editor_insert_row(0, "hllo", 4);

	editor_row_insert_char(&editor.row[0], 1, 'e');

	CHECK(editor.row[0].size == 5);
	CHECK(memcmp(editor.row[0].chars, "hello", 5) == 0);
	teardown();
}

/* Inserting at position 0 prepends. */
static void test_row_insert_char_front(void)
{
	setup();
	editor_insert_row(0, "ello", 4);

	editor_row_insert_char(&editor.row[0], 0, 'h');

	CHECK(editor.row[0].size == 5);
	CHECK(memcmp(editor.row[0].chars, "hello", 5) == 0);
	teardown();
}

/* Inserting at the end (pos == size) appends. */
static void test_row_insert_char_end(void)
{
	setup();
	editor_insert_row(0, "hell", 4);

	editor_row_insert_char(&editor.row[0], 4, 'o');

	CHECK(editor.row[0].size == 5);
	CHECK(memcmp(editor.row[0].chars, "hello", 5) == 0);
	teardown();
}

/* Inserting beyond the current end pads with spaces. */
static void test_row_insert_char_beyond(void)
{
	setup();
	editor_insert_row(0, "hi", 2);

	editor_row_insert_char(&editor.row[0], 5, '!');

	CHECK(editor.row[0].size == 6);
	CHECK(editor.row[0].chars[2] == ' ');
	CHECK(editor.row[0].chars[3] == ' ');
	CHECK(editor.row[0].chars[4] == ' ');
	CHECK(editor.row[0].chars[5] == '!');
	teardown();
}

/* Deleting a char in the middle shifts the rest left. */
static void test_row_del_char_middle(void)
{
	setup();
	editor_insert_row(0, "hxello", 6);

	editor_row_del_char(&editor.row[0], 1);   /* remove 'x' */

	CHECK(editor.row[0].size == 5);
	CHECK(memcmp(editor.row[0].chars, "hello", 5) == 0);
	teardown();
}

/* Deleting at position 0 removes the first character. */
static void test_row_del_char_first(void)
{
	setup();
	editor_insert_row(0, "xhello", 6);

	editor_row_del_char(&editor.row[0], 0);

	CHECK(editor.row[0].size == 5);
	CHECK(memcmp(editor.row[0].chars, "hello", 5) == 0);
	teardown();
}

/* Deleting at or beyond size is a safe no-op. */
static void test_row_del_char_oob(void)
{
	setup();
	editor_insert_row(0, "hello", 5);

	editor_row_del_char(&editor.row[0], 5);    /* at size — no-op */
	editor_row_del_char(&editor.row[0], 99);   /* way out — no-op */

	CHECK(editor.row[0].size == 5);
	CHECK(memcmp(editor.row[0].chars, "hello", 5) == 0);
	teardown();
}

/* Appending a string extends the row. */
static void test_row_append_string(void)
{
	setup();
	editor_insert_row(0, "hello", 5);

	editor_row_append_string(&editor.row[0], " world", 6);

	CHECK(editor.row[0].size == 11);
	CHECK(memcmp(editor.row[0].chars, "hello world", 11) == 0);
	teardown();
}

/* Appending to an empty row produces the appended string. */
static void test_row_append_string_to_empty(void)
{
	setup();
	editor_insert_row(0, "", 0);

	editor_row_append_string(&editor.row[0], "hello", 5);

	CHECK(editor.row[0].size == 5);
	CHECK(memcmp(editor.row[0].chars, "hello", 5) == 0);
	teardown();
}

/* A tab at column 0 expands to 7 spaces (fills to the 8-column tab stop). */
static void test_update_row_tab_at_col0(void)
{
	char buf[2];
	int i;

	setup();
	buf[0] = TAB;
	buf[1] = '\0';
	editor_insert_row(0, buf, 1);

	/* The code inserts one space then keeps adding spaces while
	 * (idx+1)%8 != 0, stopping at idx=7, so rsize == 7. */
	CHECK(editor.row[0].rsize == 7);
	for (i = 0; i < 7; i++)
		CHECK(editor.row[0].render[i] == ' ');
	teardown();
}

/* "a<TAB>b" — tab after 'a' expands to fill up to the tab stop at render[7]. */
static void test_update_row_tab_mid(void)
{
	char buf[4];

	setup();
	buf[0] = 'a';
	buf[1] = TAB;
	buf[2] = 'b';
	buf[3] = '\0';
	editor_insert_row(0, buf, 3);

	/* 'a' at render[0]; tab fills render[1..6]; 'b' at render[7]. */
	CHECK(editor.row[0].rsize == 8);
	CHECK(editor.row[0].render[0] == 'a');
	CHECK(editor.row[0].render[7] == 'b');
	teardown();
}

/* Plain text is reproduced unchanged in the render buffer. */
static void test_update_row_no_tabs(void)
{
	setup();
	editor_insert_row(0, "hello", 5);

	CHECK(editor.row[0].rsize == 5);
	CHECK(memcmp(editor.row[0].render, "hello", 5) == 0);
	teardown();
}

/* ---- editor_visual_col / editor_chars_col_at_visual ---- */

/* ASCII rows: visual col equals byte col. */
static void test_visual_col_ascii(void)
{
	setup();
	editor_insert_row(0, "hello", 5);

	CHECK(editor_visual_col(&editor.row[0], 0) == 0);
	CHECK(editor_visual_col(&editor.row[0], 3) == 3);
	CHECK(editor_visual_col(&editor.row[0], 5) == 5);
	teardown();
}

/* Tab at col 0 advances vcol to 7 (kg's "next 8-stop minus 1"). */
static void test_visual_col_tab(void)
{
	setup();
	editor_insert_row(0, "\tabc", 4);

	CHECK(editor_visual_col(&editor.row[0], 0) == 0);
	CHECK(editor_visual_col(&editor.row[0], 1) == 7);   /* past tab */
	CHECK(editor_visual_col(&editor.row[0], 2) == 8);   /* +'a' */
	CHECK(editor_visual_col(&editor.row[0], 4) == 10);  /* past 'abc' */
	teardown();
}

/* UTF-8 continuation bytes contribute zero visual width. */
static void test_visual_col_utf8(void)
{
	setup();
	/* "a…b" — 'a' + 3-byte ellipsis + 'b' = 5 bytes, 3 visual cols. */
	editor_insert_row(0, "a\xe2\x80\xa6""b", 5);

	CHECK(editor_visual_col(&editor.row[0], 0) == 0);
	CHECK(editor_visual_col(&editor.row[0], 1) == 1);   /* past 'a' */
	CHECK(editor_visual_col(&editor.row[0], 4) == 2);   /* past '…' */
	CHECK(editor_visual_col(&editor.row[0], 5) == 3);   /* past 'b' */
	teardown();
}

/* Past end of row: byte offset becomes (row->size + virtual offset). */
static void test_visual_col_past_eol(void)
{
	setup();
	editor_insert_row(0, "abc", 3);

	CHECK(editor_visual_col(&editor.row[0], 5) == 5);   /* 3 + 2 virtual */
	CHECK(editor_visual_col(&editor.row[0], 10) == 10);
	teardown();
}

/* chars_col_at_visual round-trips with visual_col on glyph boundaries. */
static void test_chars_col_round_trip(void)
{
	int byte;

	setup();
	editor_insert_row(0, "a\tb", 3);

	/* For each byte boundary in the row, visual_col→chars_col_at_visual
	 * round-trips back to the same byte. */
	for (byte = 0; byte <= 3; byte++) {
		int vcol = editor_visual_col(&editor.row[0], byte);
		CHECK(editor_chars_col_at_visual(&editor.row[0], vcol) == byte);
	}
	teardown();
}

/* A target that falls inside a tab's expansion rounds down to the
 * tab's start byte (closest representable position when cx is a byte). */
static void test_chars_col_inside_tab(void)
{
	setup();
	editor_insert_row(0, "\tabc", 4);   /* tab fills vcols 0..6, 'a' at 7 */

	CHECK(editor_chars_col_at_visual(&editor.row[0], 0) == 0);   /* tab start */
	CHECK(editor_chars_col_at_visual(&editor.row[0], 3) == 0);   /* mid-tab → start */
	CHECK(editor_chars_col_at_visual(&editor.row[0], 7) == 1);   /* 'a' */
	CHECK(editor_chars_col_at_visual(&editor.row[0], 8) == 2);   /* 'b' */
	teardown();
}

/* Past the row's visual end, chars_col_at_visual returns row->size plus
 * the virtual offset — supports the rect-mode "cursor in virtual space"
 * convention. */
static void test_chars_col_past_eol(void)
{
	setup();
	editor_insert_row(0, "abc", 3);

	CHECK(editor_chars_col_at_visual(&editor.row[0], 3) == 3);
	CHECK(editor_chars_col_at_visual(&editor.row[0], 5) == 5);   /* +2 virtual */
	CHECK(editor_chars_col_at_visual(&editor.row[0], 10) == 10);
	teardown();
}

/* ---- Main ---- */

int main(void)
{
	RUN(test_rows_to_string);
	RUN(test_rows_to_string_empty_row);
	RUN(test_row_insert_char_middle);
	RUN(test_row_insert_char_front);
	RUN(test_row_insert_char_end);
	RUN(test_row_insert_char_beyond);
	RUN(test_row_del_char_middle);
	RUN(test_row_del_char_first);
	RUN(test_row_del_char_oob);
	RUN(test_row_append_string);
	RUN(test_row_append_string_to_empty);
	RUN(test_update_row_tab_at_col0);
	RUN(test_update_row_tab_mid);
	RUN(test_update_row_no_tabs);
	RUN(test_visual_col_ascii);
	RUN(test_visual_col_tab);
	RUN(test_visual_col_utf8);
	RUN(test_visual_col_past_eol);
	RUN(test_chars_col_round_trip);
	RUN(test_chars_col_inside_tab);
	RUN(test_chars_col_past_eol);
	return test_summary();
}
