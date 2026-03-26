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
	return test_summary();
}
