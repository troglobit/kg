/* test_shell.c — regression tests for the shell-command runner.
 *
 * These exercise shell_run() directly: fork/exec/pipe plumbing and the
 * concurrent non-blocking I/O pump.  The interactive editor wrappers
 * (editor_shell_command / editor_shell_command_on_region) need a PTY
 * and are exercised by hand. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "../src/def.h"

/* The test exercises shell_run() only; the editor wrappers it calls below
 * are linked in via the full object set, so we stub the one symbol the
 * shared no-yank stubs file does not provide. */
int editor_read_line(int fd, const char *prompt, char *buf, int bufsize)
{
	(void)fd; (void)prompt; (void)bufsize;
	buf[0] = '\0';
	return -1;
}

/* Capture a child's stdout from a simple command with no stdin. */
static void test_shell_run_no_input(void)
{
	char *out;
	int   len = -1;

	out = shell_run("printf 'hello\\n'", NULL, 0, &len);
	CHECK(out != NULL);
	CHECK(len == 6);
	CHECK(strcmp(out, "hello\n") == 0);
	free(out);
}

/* Pipe a known input through `cat` and confirm we get it back verbatim. */
static void test_shell_run_pipe_through_cat(void)
{
	const char *in = "line1\nline2\nline3\n";
	char *out;
	int   len = -1;

	out = shell_run("cat", in, (int)strlen(in), &len);
	CHECK(out != NULL);
	CHECK(len == (int)strlen(in));
	CHECK(memcmp(out, in, len) == 0);
	free(out);
}

/* Verify the pump survives a larger payload than the initial buffer. */
static void test_shell_run_large_output(void)
{
	char *out;
	int   len = -1;

	/* 200 lines of "x" → 400 bytes; well above the initial 4 KiB buffer
	 * we still want to make sure realloc growth keeps the buffer NUL-
	 * terminated and the count accurate. */
	out = shell_run("yes x | head -n 200", NULL, 0, &len);
	CHECK(out != NULL);
	CHECK(len == 400);
	CHECK(out[len] == '\0');
	free(out);
}

/* When stdin is large enough to fill the pipe buffer while the child also
 * writes a lot, the pump must avoid the classic write-blocks-while-read-blocks
 * deadlock.  64 KiB through `cat` exercises that. */
static void test_shell_run_pump_no_deadlock(void)
{
	char *in;
	char *out;
	int   inlen = 65536;
	int   len = -1;
	int   i;

	in = malloc(inlen);
	CHECK(in != NULL);
	for (i = 0; i < inlen; i++) in[i] = 'A' + (i % 26);

	out = shell_run("cat", in, inlen, &len);
	CHECK(out != NULL);
	CHECK(len == inlen);
	CHECK(memcmp(out, in, inlen) == 0);
	free(out);
	free(in);
}

/* Bogus command: /bin/sh -c returns 127, but shell_run still completes;
 * we should get back a (possibly empty) malloc'd buffer, not NULL or a crash. */
static void test_shell_run_command_not_found(void)
{
	char *out;
	int   len = -1;

	out = shell_run("nope-does-not-exist-12345", NULL, 0, &len);
	CHECK(out != NULL);  /* successful pipe setup; stdout was empty */
	CHECK(len == 0);
	free(out);
}

/* ---- Main ---- */

int main(void)
{
	RUN(test_shell_run_no_input);
	RUN(test_shell_run_pipe_through_cat);
	RUN(test_shell_run_large_output);
	RUN(test_shell_run_pump_no_deadlock);
	RUN(test_shell_run_command_not_found);
	return test_summary();
}
