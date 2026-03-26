/* test_autocomplete.c — regression tests for autopair lookup */

#include <stdio.h>
#include "test.h"
#include "../src/def.h"

/* ---- Tests ---- */

static void test_find_close_char_all_pairs(void)
{
	CHECK(editor_find_close_char('{')  == '}');
	CHECK(editor_find_close_char('[')  == ']');
	CHECK(editor_find_close_char('(')  == ')');
	CHECK(editor_find_close_char('"')  == '"');
	CHECK(editor_find_close_char('\'') == '\'');
	CHECK(editor_find_close_char('`')  == '`');
	CHECK(editor_find_close_char('<')  == '>');
}

/* Closing characters are not opening characters themselves. */
static void test_find_close_char_not_for_closers(void)
{
	CHECK(editor_find_close_char('}') == 0);
	CHECK(editor_find_close_char(']') == 0);
	CHECK(editor_find_close_char(')') == 0);
	CHECK(editor_find_close_char('>') == 0);
}

/* Unrelated characters return 0. */
static void test_find_close_char_unknown(void)
{
	CHECK(editor_find_close_char('a') == 0);
	CHECK(editor_find_close_char('0') == 0);
	CHECK(editor_find_close_char('!') == 0);
	CHECK(editor_find_close_char(' ') == 0);
	CHECK(editor_find_close_char(0)   == 0);
}

/* ---- Main ---- */

int main(void)
{
	RUN(test_find_close_char_all_pairs);
	RUN(test_find_close_char_not_for_closers);
	RUN(test_find_close_char_unknown);
	return test_summary();
}
