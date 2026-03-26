/* test_word.c — regression tests for word-case, join-line, and comment-dwim */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "../src/def.h"

extern struct editor_syntax HLDB[];   /* defined in syntax.c */

/* ---- Helpers ---- */

static void setup(void)
{
	free_all_rows();
	memset(&editor, 0, sizeof(editor));
	editor.screenrows = 24;
	editor.screencols = 80;
	suppress_undo     = 0;
	undo_free();
	undo_init();
}

static void teardown(void)
{
	free_all_rows();
	editor.row     = NULL;
	editor.numrows = 0;
	undo_free();
}

/* Position the cursor at the start of the first row. */
static void cursor_home(void)
{
	editor.cx = editor.cy = editor.rowoff = editor.coloff = 0;
}

/* ---- Word-case tests ---- */

/* Upcasing "hello" produces "HELLO" and lands the cursor after the word. */
static void test_upcase_word(void)
{
	setup();
	editor_insert_row(0, "hello", 5);
	cursor_home();

	editor_upcase_word();

	CHECK(editor.row[0].size == 5);
	CHECK(memcmp(editor.row[0].chars, "HELLO", 5) == 0);
	CHECK(editor.cx == 5);
	teardown();
}

/* Downcasing "HELLO" produces "hello". */
static void test_downcase_word(void)
{
	setup();
	editor_insert_row(0, "HELLO", 5);
	cursor_home();

	editor_downcase_word();

	CHECK(editor.row[0].size == 5);
	CHECK(memcmp(editor.row[0].chars, "hello", 5) == 0);
	teardown();
}

/* Capitalizing "hELLO" produces "Hello" (first char upper, rest lower). */
static void test_capitalize_word(void)
{
	setup();
	editor_insert_row(0, "hELLO", 5);
	cursor_home();

	editor_capitalize_word();

	CHECK(editor.row[0].size == 5);
	CHECK(memcmp(editor.row[0].chars, "Hello", 5) == 0);
	teardown();
}

/* Cursor positioned within leading whitespace: the whitespace is skipped and
 * only the word itself is transformed. */
static void test_upcase_word_skips_leading_space(void)
{
	setup();
	editor_insert_row(0, " hello", 6);
	cursor_home();   /* cursor before the space */

	editor_upcase_word();

	CHECK(memcmp(editor.row[0].chars, " HELLO", 6) == 0);
	CHECK(editor.cx == 6);
	teardown();
}

/* Two consecutive upcases on "hello world" transform both words. */
static void test_upcase_two_words(void)
{
	setup();
	editor_insert_row(0, "hello world", 11);
	cursor_home();

	editor_upcase_word();   /* "HELLO world", cx=5 */
	/* cursor is now at col 5 (the space); upcase again moves to "world" */
	editor_upcase_word();   /* "HELLO WORLD", cx=11 */

	CHECK(memcmp(editor.row[0].chars, "HELLO WORLD", 11) == 0);
	teardown();
}

/* ---- Join-line tests ---- */

/* Joining two plain lines inserts a space at the join point. */
static void test_join_line_basic(void)
{
	setup();
	editor_insert_row(0, "hello", 5);
	editor_insert_row(1, "world", 5);
	cursor_home();
	editor.cy = 1;

	editor_join_line();

	CHECK(editor.numrows == 1);
	CHECK(editor.row[0].size == 11);
	CHECK(memcmp(editor.row[0].chars, "hello world", 11) == 0);
	teardown();
}

/* Leading whitespace on the lower line is stripped before joining. */
static void test_join_line_strips_indent(void)
{
	setup();
	editor_insert_row(0, "hello", 5);
	editor_insert_row(1, "  world", 7);
	cursor_home();
	editor.cy = 1;

	editor_join_line();

	CHECK(editor.numrows == 1);
	CHECK(editor.row[0].size == 11);
	CHECK(memcmp(editor.row[0].chars, "hello world", 11) == 0);
	teardown();
}

/* Joining when the upper line is empty: no space is inserted. */
static void test_join_line_empty_upper(void)
{
	setup();
	editor_insert_row(0, "", 0);
	editor_insert_row(1, "world", 5);
	cursor_home();
	editor.cy = 1;

	editor_join_line();

	CHECK(editor.numrows == 1);
	CHECK(editor.row[0].size == 5);
	CHECK(memcmp(editor.row[0].chars, "world", 5) == 0);
	teardown();
}

/* Joining on the first row is a safe no-op. */
static void test_join_line_first_row_noop(void)
{
	setup();
	editor_insert_row(0, "only row", 8);
	cursor_home();

	editor_join_line();

	CHECK(editor.numrows == 1);
	CHECK(editor.row[0].size == 8);
	teardown();
}

/* Cursor lands at the join point (original end of the upper line). */
static void test_join_line_cursor_at_join(void)
{
	setup();
	editor_insert_row(0, "hello", 5);
	editor_insert_row(1, "world", 5);
	cursor_home();
	editor.cy = 1;

	editor_join_line();

	/* join_col was 5 (size of "hello"), coloff=0, so cx should be 5 */
	CHECK(editor.cx == 5);
	teardown();
}

/* ---- Comment-dwim tests ---- */

/* Adding a comment prepends "// " to an uncommented line. */
static void test_comment_dwim_add(void)
{
	setup();
	editor.syntax = &HLDB[0];   /* C syntax: scs = "//" */
	editor_insert_row(0, "int x;", 6);
	cursor_home();
	editor.mark_set = 0;

	editor_comment_dwim();

	CHECK(editor.row[0].size == 9);
	CHECK(memcmp(editor.row[0].chars, "// int x;", 9) == 0);
	teardown();
}

/* Removing a comment strips "// " from an already-commented line. */
static void test_comment_dwim_remove(void)
{
	setup();
	editor.syntax = &HLDB[0];
	editor_insert_row(0, "// int x;", 9);
	cursor_home();
	editor.mark_set = 0;

	editor_comment_dwim();

	CHECK(editor.row[0].size == 6);
	CHECK(memcmp(editor.row[0].chars, "int x;", 6) == 0);
	teardown();
}

/* No syntax set: comment_dwim is a no-op (does not crash). */
static void test_comment_dwim_no_syntax(void)
{
	setup();
	editor.syntax = NULL;
	editor_insert_row(0, "hello", 5);
	cursor_home();

	editor_comment_dwim();   /* must not crash */

	CHECK(editor.row[0].size == 5);
	teardown();
}

/* ---- Main ---- */

int main(void)
{
	RUN(test_upcase_word);
	RUN(test_downcase_word);
	RUN(test_capitalize_word);
	RUN(test_upcase_word_skips_leading_space);
	RUN(test_upcase_two_words);
	RUN(test_join_line_basic);
	RUN(test_join_line_strips_indent);
	RUN(test_join_line_empty_upper);
	RUN(test_join_line_first_row_noop);
	RUN(test_join_line_cursor_at_join);
	RUN(test_comment_dwim_add);
	RUN(test_comment_dwim_remove);
	RUN(test_comment_dwim_no_syntax);
	return test_summary();
}
