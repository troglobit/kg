/* test_complete.c — tests for editor_path_complete().
 *
 * Builds a scratch directory under /tmp with a known set of entries, then
 * exercises completion against various prefixes and verifies match counts,
 * longest-common-prefix output, and the is-directory flag for sole matches. */

#define _DEFAULT_SOURCE   /* for mkdtemp under -std=c99 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "test.h"
#include "../src/def.h"

static char scratch[256];

static void touch(const char *name)
{
	char path[512];
	FILE *fp;

	snprintf(path, sizeof(path), "%s/%s", scratch, name);
	fp = fopen(path, "w");
	if (fp) fclose(fp);
}

static void mkscratchdir(const char *name)
{
	char path[512];

	snprintf(path, sizeof(path), "%s/%s", scratch, name);
	mkdir(path, 0700);
}

static void setup(void)
{
	char tmpl[] = "/tmp/kg-complete-XXXXXX";
	const char *p = mkdtemp(tmpl);

	CHECK(p != NULL);
	snprintf(scratch, sizeof(scratch), "%s/", p);

	touch("foobar");
	touch("foobaz");
	touch("foe");
	touch("README");
	touch(".hidden");
	mkscratchdir("subdir");
}

static void rmtree(const char *path)
{
	struct dirent *de;
	struct stat st;
	DIR *dp = opendir(path);
	char child[512];

	if (!dp) return;
	while ((de = readdir(dp)) != NULL) {
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
		snprintf(child, sizeof(child), "%s/%s", path, de->d_name);
		if (lstat(child, &st) == 0 && S_ISDIR(st.st_mode))
			rmtree(child);
		else
			unlink(child);
	}
	closedir(dp);
	rmdir(path);
}

static void teardown(void)
{
	rmtree(scratch);
}

/* Two files match "foob": LCP collapses to "fooba". */
static void test_multiple_matches_lcp(void)
{
	char lcp[256];
	int is_dir = -1;
	int n;

	setup();
	n = editor_path_complete(scratch, "foob", lcp, sizeof(lcp), &is_dir, NULL, NULL, 0);
	CHECK(n == 2);
	CHECK(strcmp(lcp, "fooba") == 0);
	CHECK(is_dir == 0);   /* not a single match → flag stays 0 */
	teardown();
}

/* Three entries start with "fo": foobar, foobaz, foe.  LCP is "fo". */
static void test_three_matches_lcp_two_chars(void)
{
	char lcp[256];
	int is_dir = 0;
	int n;

	setup();
	n = editor_path_complete(scratch, "fo", lcp, sizeof(lcp), &is_dir, NULL, NULL, 0);
	CHECK(n == 3);
	CHECK(strcmp(lcp, "fo") == 0);
	teardown();
}

/* Single match yields the full entry; is_dir reflects type. */
static void test_single_file_match(void)
{
	char lcp[256];
	int is_dir = -1;
	int n;

	setup();
	n = editor_path_complete(scratch, "REA", lcp, sizeof(lcp), &is_dir, NULL, NULL, 0);
	CHECK(n == 1);
	CHECK(strcmp(lcp, "README") == 0);
	CHECK(is_dir == 0);
	teardown();
}

static void test_single_dir_match(void)
{
	char lcp[256];
	int is_dir = 0;
	int n;

	setup();
	n = editor_path_complete(scratch, "subd", lcp, sizeof(lcp), &is_dir, NULL, NULL, 0);
	CHECK(n == 1);
	CHECK(strcmp(lcp, "subdir") == 0);
	CHECK(is_dir == 1);
	teardown();
}

/* No match → 0, lcp left empty. */
static void test_no_match(void)
{
	char lcp[256];
	int is_dir = 0;
	int n;

	setup();
	n = editor_path_complete(scratch, "xyzzy", lcp, sizeof(lcp), &is_dir, NULL, NULL, 0);
	CHECK(n == 0);
	CHECK(lcp[0] == '\0');
	teardown();
}

/* Bad directory → -1. */
static void test_opendir_failure(void)
{
	char lcp[256];
	int is_dir = 0;
	int n;

	n = editor_path_complete("/no/such/dir/exists-12345/", "x", lcp, sizeof(lcp), &is_dir, NULL, NULL, 0);
	CHECK(n == -1);
}

/* Empty prefix matches every visible entry; LCP collapses to "".  Dotfiles
 * and the ".." entry are hidden because the prefix does not start with '.'. */
static void test_empty_prefix(void)
{
	char lcp[256];
	int is_dir = 0;
	int n;

	setup();
	n = editor_path_complete(scratch, "", lcp, sizeof(lcp), &is_dir, NULL, NULL, 0);
	CHECK(n == 5);            /* foobar, foobaz, foe, README, subdir */
	CHECK(lcp[0] == '\0');
	teardown();
}

/* A leading-dot prefix opts dotfiles back in. */
static void test_dot_prefix_shows_hidden(void)
{
	char lcp[256];
	int is_dir = 0;
	int n;

	setup();
	n = editor_path_complete(scratch, ".h", lcp, sizeof(lcp), &is_dir, NULL, NULL, 0);
	CHECK(n == 1);
	CHECK(strcmp(lcp, ".hidden") == 0);
	teardown();
}

/* The `names` out-buffer collects matching names space-separated and
 * advances names_off. */
static void test_names_buffer(void)
{
	char lcp[256];
	char names[256];
	int is_dir = 0;
	int off = 0;
	int n;

	setup();
	names[0] = '\0';
	n = editor_path_complete(scratch, "foo", lcp, sizeof(lcp), &is_dir,
	                         names, &off, sizeof(names));
	CHECK(n == 2);
	CHECK(off > 0);
	CHECK(strstr(names, "foobar ") != NULL);
	CHECK(strstr(names, "foobaz ") != NULL);
	teardown();
}

int main(void)
{
	RUN(test_multiple_matches_lcp);
	RUN(test_three_matches_lcp_two_chars);
	RUN(test_single_file_match);
	RUN(test_single_dir_match);
	RUN(test_no_match);
	RUN(test_opendir_failure);
	RUN(test_empty_prefix);
	RUN(test_dot_prefix_shows_hidden);
	RUN(test_names_buffer);
	return test_summary();
}
