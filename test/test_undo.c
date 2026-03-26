/* test_undo.c — regression tests for the undo stack */

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

/* ---- Tests ---- */

/* Inserting a character is undone by removing it. */
static void test_insert_char(void)
{
	setup();
	editor_insert_row(0, "hllo", 4);
	editor.cx = 1;                         /* cursor after 'h'  */
	editor_insert_char('e');               /* "hllo" → "hello"  */

	CHECK(editor.row[0].size == 5);
	CHECK(memcmp(editor.row[0].chars, "hello", 5) == 0);

	editor_undo();

	CHECK(editor.row[0].size == 4);
	CHECK(memcmp(editor.row[0].chars, "hllo", 4) == 0);
	teardown();
}

/* Deleting the character before the cursor is undone by reinserting it. */
static void test_delete_char(void)
{
	setup();
	editor_insert_row(0, "hello", 5);
	editor.cx = 1;                         /* cursor after 'h'  */
	editor_del_char();                     /* "hello" → "ello"  */

	CHECK(editor.row[0].size == 4);
	CHECK(memcmp(editor.row[0].chars, "ello", 4) == 0);

	editor_undo();

	CHECK(editor.row[0].size == 5);
	CHECK(memcmp(editor.row[0].chars, "hello", 5) == 0);
	teardown();
}

/* Forward-deleting a character is undone by reinserting it. */
static void test_forward_delete_char(void)
{
	setup();
	editor_insert_row(0, "hello", 5);
	editor.cx = 0;
	editor_del_forward_char();             /* "hello" → "ello"  */

	CHECK(editor.row[0].size == 4);
	CHECK(memcmp(editor.row[0].chars, "ello", 4) == 0);

	editor_undo();

	CHECK(editor.row[0].size == 5);
	CHECK(memcmp(editor.row[0].chars, "hello", 5) == 0);
	teardown();
}

/* Inserting a newline at column 0 creates an empty row that undo removes. */
static void test_insert_line(void)
{
	setup();
	editor_insert_row(0, "hello", 5);
	editor.cx = 0;
	editor.cy = 0;
	editor_insert_newline();               /* inserts empty row before "hello" */

	CHECK(editor.numrows == 2);
	CHECK(editor.row[0].size == 0);
	CHECK(memcmp(editor.row[1].chars, "hello", 5) == 0);

	editor_undo();

	CHECK(editor.numrows == 1);
	CHECK(memcmp(editor.row[0].chars, "hello", 5) == 0);
	teardown();
}

/* Splitting a line mid-word is undone by rejoining it. */
static void test_split_line(void)
{
	setup();
	editor_insert_row(0, "hello", 5);
	editor.cx = 2;                         /* cursor after "he"  */
	editor_insert_newline();               /* "hello" → "he" / "llo" */

	CHECK(editor.numrows == 2);
	CHECK(editor.row[0].size == 2);
	CHECK(memcmp(editor.row[0].chars, "he", 2) == 0);
	CHECK(editor.row[1].size == 3);
	CHECK(memcmp(editor.row[1].chars, "llo", 3) == 0);

	editor_undo();

	CHECK(editor.numrows == 1);
	CHECK(editor.row[0].size == 5);
	CHECK(memcmp(editor.row[0].chars, "hello", 5) == 0);
	teardown();
}

/* Joining two lines (M-^) is undone by splitting them back.
 * The undo record stores the original content of the deleted row,
 * including its leading whitespace, so the split is exact. */
static void test_join_line(void)
{
	char *newchars;

	setup();
	editor_insert_row(0, "hello", 5);
	editor_insert_row(1, "  world", 7);

	/* Push the UNDO_JOIN_LINE record as editor_join_line would:
	 *   prev_row=0, join_col=5 (original end of row[0]),
	 *   text = original row[1] content including leading spaces */
	undo_push(UNDO_JOIN_LINE, 0, 5, 0, "  world", 7);

	/* Perform join: "hello" + " " + "world" = "hello world"
	 * (leading whitespace stripped from row[1], space inserted at join point) */
	newchars = realloc(editor.row[0].chars, 12);
	editor.row[0].chars     = newchars;
	editor.row[0].chars[5]  = ' ';
	memcpy(editor.row[0].chars + 6, "world", 5);
	editor.row[0].size      = 11;
	editor.row[0].chars[11] = '\0';
	editor_update_row(&editor.row[0]);
	suppress_undo = 1;
	editor_del_row(1);
	suppress_undo = 0;

	CHECK(editor.numrows == 1);
	CHECK(editor.row[0].size == 11);
	CHECK(memcmp(editor.row[0].chars, "hello world", 11) == 0);

	editor_undo();

	CHECK(editor.numrows == 2);
	CHECK(editor.row[0].size == 5);
	CHECK(memcmp(editor.row[0].chars, "hello", 5) == 0);
	CHECK(editor.row[1].size == 7);
	CHECK(memcmp(editor.row[1].chars, "  world", 7) == 0);
	teardown();
}

/* Killing to end of line is undone by reinserting the killed text. */
static void test_kill_line(void)
{
	setup();
	editor_insert_row(0, "hello", 5);
	editor.cx = 2;                         /* cursor after "he"  */
	editor_kill_line();                    /* "hello" → "he"     */

	CHECK(editor.row[0].size == 2);
	CHECK(memcmp(editor.row[0].chars, "he", 2) == 0);

	editor_undo();

	CHECK(editor.row[0].size == 5);
	CHECK(memcmp(editor.row[0].chars, "hello", 5) == 0);
	teardown();
}

/* A yanked span is deleted by its undo record.
 * editor_insert_text_raw (used by yank) sets suppress_undo internally,
 * so the UNDO_YANK_TEXT record is the only one on the stack. */
static void test_yank_text(void)
{
	setup();
	editor_insert_row(0, "abhellocd", 9);
	editor.cx = 2;                         /* cursor before 'h'  */

	/* Record simulates what editor_yank / insert_text_raw would push */
	undo_push(UNDO_YANK_TEXT, 0, 2, 0, "hello", 5);

	editor_undo();                         /* del 5 chars from col 2 */

	CHECK(editor.row[0].size == 4);
	CHECK(memcmp(editor.row[0].chars, "abcd", 4) == 0);
	teardown();
}

/* Paragraph reflow is undone by deleting the reflowed rows and restoring
 * the original lines from the '\n'-delimited text saved in the record. */
static void test_reflow_para(void)
{
	setup();
	/* One reflowed line standing in for what editor_reflow_paragraph produced */
	editor_insert_row(0, "hello world", 11);

	/* Record as editor_reflow_paragraph would push it:
	 *   row=0, col=1 (number of reflowed rows to delete),
	 *   text = original two lines joined with '\n' */
	undo_push(UNDO_REFLOW_PARA, 0, 1, 0, "hello\nworld", 11);

	editor_undo();

	CHECK(editor.numrows == 2);
	CHECK(editor.row[0].size == 5);
	CHECK(memcmp(editor.row[0].chars, "hello", 5) == 0);
	CHECK(editor.row[1].size == 5);
	CHECK(memcmp(editor.row[1].chars, "world", 5) == 0);
	teardown();
}

/* Undoing back to the saved state (clean_size) clears the dirty flag. */
static void test_dirty_tracking(void)
{
	setup();
	editor_insert_row(0, "hi", 2);
	editor.dirty = 0;

	editor_insert_char('!');      /* undostack.size = 1 */
	undo_mark_clean();            /* clean_size = 1     */
	editor_insert_char('?');      /* undostack.size = 2 */

	/* Undo '?': size drops to 1 == clean_size → dirty cleared */
	editor_undo();
	CHECK(editor.dirty == 0);
	CHECK(editor.row[0].size == 3);   /* "hi!" */

	/* Make dirty again, undo back to clean again */
	editor_insert_char('@');
	editor_undo();
	CHECK(editor.dirty == 0);
	teardown();
}

/* Undoing on an empty stack is a safe no-op. */
static void test_nothing_to_undo(void)
{
	setup();
	editor_insert_row(0, "test", 4);

	editor_undo();   /* empty stack — must not crash */

	CHECK(undostack.size == 0);
	CHECK(editor.numrows == 1);
	teardown();
}

/* M-u / M-l / M-c push two LIFO records: KILL_TEXT (original) then
 * YANK_TEXT (transformed).  Two consecutive undos must restore the
 * original text exactly. */
static void test_word_case_two_records(void)
{
	setup();
	editor_insert_row(0, "hello", 5);

	/* Simulate upcase: overwrite row chars in-place */
	memcpy(editor.row[0].chars, "HELLO", 5);
	editor_update_row(&editor.row[0]);
	editor.dirty = 1;

	/* Records as do_word_case pushes them */
	undo_push(UNDO_KILL_TEXT, 0, 0, 0, "hello", 5);   /* original  */
	undo_push(UNDO_YANK_TEXT, 0, 0, 0, "HELLO", 5);   /* transformed */

	/* First undo (YANK_TEXT): deletes "HELLO" leaving an empty row */
	editor_undo();
	CHECK(editor.row[0].size == 0);

	/* Second undo (KILL_TEXT): reinserts "hello" */
	editor_undo();
	CHECK(editor.row[0].size == 5);
	CHECK(memcmp(editor.row[0].chars, "hello", 5) == 0);
	teardown();
}

/* ---- Main ---- */

int main(void)
{
	RUN(test_insert_char);
	RUN(test_delete_char);
	RUN(test_forward_delete_char);
	RUN(test_insert_line);
	RUN(test_split_line);
	RUN(test_join_line);
	RUN(test_kill_line);
	RUN(test_yank_text);
	RUN(test_reflow_para);
	RUN(test_dirty_tracking);
	RUN(test_nothing_to_undo);
	RUN(test_word_case_two_records);
	return test_summary();
}
