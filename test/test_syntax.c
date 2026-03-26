/* test_syntax.c — regression tests for syntax highlighting */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "../src/def.h"

/* HLDB is defined in syntax.c; not declared in def.h since it is an
 * implementation detail, but tests need direct access to pick a language. */
extern struct editor_syntax HLDB[];

/* ---- Helpers ---- */

static void setup(struct editor_syntax *syn)
{
	free_all_rows();
	memset(&editor, 0, sizeof(editor));
	editor.screenrows = 24;
	editor.screencols = 80;
	editor.syntax     = syn;
}

static void teardown(void)
{
	free_all_rows();
	editor.row     = NULL;
	editor.numrows = 0;
}

/* ---- is_separator tests ---- */

static void test_is_separator_whitespace(void)
{
	CHECK(is_separator(' '));
	CHECK(is_separator('\t'));
	CHECK(is_separator('\n'));
}

static void test_is_separator_nul(void)
{
	CHECK(is_separator('\0'));
}

static void test_is_separator_punct(void)
{
	CHECK(is_separator('('));
	CHECK(is_separator(')'));
	CHECK(is_separator('['));
	CHECK(is_separator(']'));
	CHECK(is_separator(';'));
	CHECK(is_separator(','));
	CHECK(is_separator('+'));
	CHECK(is_separator('='));
}

static void test_is_separator_alnum_false(void)
{
	CHECK(!is_separator('a'));
	CHECK(!is_separator('Z'));
	CHECK(!is_separator('0'));
	CHECK(!is_separator('_'));
}

/* ---- editor_syntax_to_color tests ---- */

static void test_syntax_to_color(void)
{
	CHECK(editor_syntax_to_color(HL_COMMENT)   == 36);
	CHECK(editor_syntax_to_color(HL_MLCOMMENT) == 36);
	CHECK(editor_syntax_to_color(HL_KEYWORD1)  == 33);
	CHECK(editor_syntax_to_color(HL_KEYWORD2)  == 32);
	CHECK(editor_syntax_to_color(HL_STRING)    == 35);
	CHECK(editor_syntax_to_color(HL_NUMBER)    == 31);
	CHECK(editor_syntax_to_color(HL_MATCH)     == 34);
	CHECK(editor_syntax_to_color(HL_NORMAL)    == 37);
}

/* ---- C syntax tests (HLDB[0]) ---- */

/* "int x;" → "int" is a type keyword (HL_KEYWORD2, the trailing-| group). */
static void test_c_type_keyword(void)
{
	setup(&HLDB[0]);
	editor_insert_row(0, "int x;", 6);

	CHECK(editor.row[0].hl[0] == HL_KEYWORD2);
	CHECK(editor.row[0].hl[1] == HL_KEYWORD2);
	CHECK(editor.row[0].hl[2] == HL_KEYWORD2);
	CHECK(editor.row[0].hl[3] == HL_NORMAL);   /* space after keyword */
	teardown();
}

/* "return 0;" → "return" is a control keyword (HL_KEYWORD1). */
static void test_c_ctrl_keyword(void)
{
	setup(&HLDB[0]);
	editor_insert_row(0, "return 0;", 9);

	CHECK(editor.row[0].hl[0] == HL_KEYWORD1);
	CHECK(editor.row[0].hl[5] == HL_KEYWORD1);
	CHECK(editor.row[0].hl[6] == HL_NORMAL);   /* space */
	CHECK(editor.row[0].hl[7] == HL_NUMBER);   /* 0 */
	teardown();
}

/* A double-quoted string literal is HL_STRING throughout. */
static void test_c_string(void)
{
	int i;

	setup(&HLDB[0]);
	editor_insert_row(0, "\"hello\"", 7);

	for (i = 0; i < 7; i++)
		CHECK(editor.row[0].hl[i] == HL_STRING);
	teardown();
}

/* A single-line comment "//" colours the rest of the line HL_COMMENT. */
static void test_c_line_comment(void)
{
	int i;

	setup(&HLDB[0]);
	editor_insert_row(0, "// comment", 10);

	for (i = 0; i < 10; i++)
		CHECK(editor.row[0].hl[i] == HL_COMMENT);
	teardown();
}

/* A decimal integer literal is HL_NUMBER. */
static void test_c_integer(void)
{
	setup(&HLDB[0]);
	editor_insert_row(0, "42", 2);

	CHECK(editor.row[0].hl[0] == HL_NUMBER);
	CHECK(editor.row[0].hl[1] == HL_NUMBER);
	teardown();
}

/* A hex literal 0xff is fully HL_NUMBER. */
static void test_c_hex(void)
{
	int i;

	setup(&HLDB[0]);
	editor_insert_row(0, "0xff", 4);

	for (i = 0; i < 4; i++)
		CHECK(editor.row[0].hl[i] == HL_NUMBER);
	teardown();
}

/* A binary literal 0b101 is fully HL_NUMBER. */
static void test_c_binary(void)
{
	int i;

	setup(&HLDB[0]);
	editor_insert_row(0, "0b101", 5);

	for (i = 0; i < 5; i++)
		CHECK(editor.row[0].hl[i] == HL_NUMBER);
	teardown();
}

/* Identifiers that merely contain a keyword substring are not highlighted. */
static void test_c_no_partial_keyword(void)
{
	setup(&HLDB[0]);
	editor_insert_row(0, "returning", 9);   /* not "return" */

	CHECK(editor.row[0].hl[0] == HL_NORMAL);
	teardown();
}

/* ---- Makefile syntax tests (HLDB[18]) ---- */

/* "all: src" → the target name "all" is HL_KEYWORD1. */
static void test_make_target(void)
{
	setup(&HLDB[18]);
	CHECK(strcmp(HLDB[18].name, "Makefile") == 0);   /* guard: index drift */
	editor_insert_row(0, "all: src", 8);

	CHECK(editor.row[0].hl[0] == HL_KEYWORD1);
	CHECK(editor.row[0].hl[1] == HL_KEYWORD1);
	CHECK(editor.row[0].hl[2] == HL_KEYWORD1);
	CHECK(editor.row[0].hl[3] == HL_NORMAL);   /* ':' not highlighted */
	teardown();
}

/* "CC = gcc" → the variable "CC" is HL_KEYWORD2, "=" is HL_KEYWORD1. */
static void test_make_simple_assignment(void)
{
	setup(&HLDB[18]);
	editor_insert_row(0, "CC = gcc", 8);

	CHECK(editor.row[0].hl[0] == HL_KEYWORD2);
	CHECK(editor.row[0].hl[1] == HL_KEYWORD2);
	CHECK(editor.row[0].hl[2] == HL_NORMAL);   /* space */
	CHECK(editor.row[0].hl[3] == HL_KEYWORD1); /* '=' */
	teardown();
}

/* "CFLAGS := -Wall" → ":=" is a compound assignment operator (HL_KEYWORD1). */
static void test_make_compound_assignment(void)
{
	setup(&HLDB[18]);
	editor_insert_row(0, "CFLAGS := -Wall", 15);

	CHECK(editor.row[0].hl[0] == HL_KEYWORD2);   /* C */
	CHECK(editor.row[0].hl[6] == HL_NORMAL);     /* space */
	CHECK(editor.row[0].hl[7] == HL_KEYWORD1);   /* ':' of ':=' */
	CHECK(editor.row[0].hl[8] == HL_KEYWORD1);   /* '=' of ':=' */
	teardown();
}

/* "# comment" → the whole line is HL_COMMENT. */
static void test_make_comment(void)
{
	int i;

	setup(&HLDB[18]);
	editor_insert_row(0, "# comment", 9);

	for (i = 0; i < 9; i++)
		CHECK(editor.row[0].hl[i] == HL_COMMENT);
	teardown();
}

/* ---- Markdown syntax tests (HLDB[19]) ---- */

/* "# Heading" → fully HL_KEYWORD1. */
static void test_md_atx_heading(void)
{
	int i;

	setup(&HLDB[19]);
	CHECK(strcmp(HLDB[19].name, "Markdown") == 0);   /* guard: index drift */
	editor_insert_row(0, "# Heading", 9);

	for (i = 0; i < 9; i++)
		CHECK(editor.row[0].hl[i] == HL_KEYWORD1);
	teardown();
}

/* "> blockquote" → fully HL_COMMENT. */
static void test_md_blockquote(void)
{
	int i;

	setup(&HLDB[19]);
	editor_insert_row(0, "> quote", 7);

	for (i = 0; i < 7; i++)
		CHECK(editor.row[0].hl[i] == HL_COMMENT);
	teardown();
}

/* ``` fence line → fully HL_STRING. */
static void test_md_fenced_code_fence(void)
{
	int i;

	setup(&HLDB[19]);
	editor_insert_row(0, "```", 3);

	for (i = 0; i < 3; i++)
		CHECK(editor.row[0].hl[i] == HL_STRING);
	teardown();
}

/* Setext heading underline "=====" → fully HL_KEYWORD1. */
static void test_md_setext_underline(void)
{
	int i;

	setup(&HLDB[19]);
	/* Two rows: the heading text and the underline.
	 * markdown_syntax checks row->idx+1 to detect setext underlines,
	 * so both rows must exist with correct idx values. */
	editor_insert_row(0, "Hello", 5);
	editor_insert_row(1, "=====", 5);
	/* editor_insert_row updates syntax before incrementing numrows, so the
	 * "row above underline" re-trigger fires with numrows=1 and misses.
	 * Re-trigger row 0 now that numrows is correct. */
	editor_update_row(&editor.row[0]);

	/* The underline row itself is HL_KEYWORD1. */
	for (i = 0; i < 5; i++)
		CHECK(editor.row[1].hl[i] == HL_KEYWORD1);

	/* The heading text row is re-highlighted as HL_KEYWORD1 too. */
	for (i = 0; i < 5; i++)
		CHECK(editor.row[0].hl[i] == HL_KEYWORD1);
	teardown();
}

/* ---- Main ---- */

int main(void)
{
	RUN(test_is_separator_whitespace);
	RUN(test_is_separator_nul);
	RUN(test_is_separator_punct);
	RUN(test_is_separator_alnum_false);
	RUN(test_syntax_to_color);
	RUN(test_c_type_keyword);
	RUN(test_c_ctrl_keyword);
	RUN(test_c_string);
	RUN(test_c_line_comment);
	RUN(test_c_integer);
	RUN(test_c_hex);
	RUN(test_c_binary);
	RUN(test_c_no_partial_keyword);
	RUN(test_make_target);
	RUN(test_make_simple_assignment);
	RUN(test_make_compound_assignment);
	RUN(test_make_comment);
	RUN(test_md_atx_heading);
	RUN(test_md_blockquote);
	RUN(test_md_fenced_code_fence);
	RUN(test_md_setext_underline);
	return test_summary();
}
