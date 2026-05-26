/* test_complete.c — tests for editor_path_complete_entries().
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
	struct path_entry entries[8];
	char lcp[256];
	int n;

	setup();
	n = editor_path_complete_entries(scratch, "foob", entries, 8, lcp, sizeof(lcp));
	CHECK(n == 2);
	CHECK(strcmp(lcp, "fooba") == 0);
	CHECK(strcmp(entries[0].name, "foobar") == 0);   /* sorted alphabetically */
	CHECK(strcmp(entries[1].name, "foobaz") == 0);
	teardown();
}

/* Three entries start with "fo": foobar, foobaz, foe.  LCP is "fo". */
static void test_three_matches_lcp_two_chars(void)
{
	struct path_entry entries[8];
	char lcp[256];
	int n;

	setup();
	n = editor_path_complete_entries(scratch, "fo", entries, 8, lcp, sizeof(lcp));
	CHECK(n == 3);
	CHECK(strcmp(lcp, "fo") == 0);
	CHECK(strcmp(entries[0].name, "foe") == 0);      /* sorted */
	CHECK(strcmp(entries[1].name, "foobar") == 0);
	CHECK(strcmp(entries[2].name, "foobaz") == 0);
	teardown();
}

/* Single match yields the full entry; is_dir reflects type. */
static void test_single_file_match(void)
{
	struct path_entry entries[8];
	char lcp[256];
	int n;

	setup();
	n = editor_path_complete_entries(scratch, "REA", entries, 8, lcp, sizeof(lcp));
	CHECK(n == 1);
	CHECK(strcmp(lcp, "README") == 0);
	CHECK(strcmp(entries[0].name, "README") == 0);
	CHECK(entries[0].is_dir == 0);
	teardown();
}

static void test_single_dir_match(void)
{
	struct path_entry entries[8];
	char lcp[256];
	int n;

	setup();
	n = editor_path_complete_entries(scratch, "subd", entries, 8, lcp, sizeof(lcp));
	CHECK(n == 1);
	CHECK(strcmp(lcp, "subdir") == 0);
	CHECK(strcmp(entries[0].name, "subdir") == 0);
	CHECK(entries[0].is_dir == 1);
	teardown();
}

/* No match → 0, lcp left empty. */
static void test_no_match(void)
{
	struct path_entry entries[8];
	char lcp[256];
	int n;

	setup();
	n = editor_path_complete_entries(scratch, "xyzzy", entries, 8, lcp, sizeof(lcp));
	CHECK(n == 0);
	CHECK(lcp[0] == '\0');
	teardown();
}

/* Bad directory → -1. */
static void test_opendir_failure(void)
{
	struct path_entry entries[8];
	char lcp[256];
	int n;

	n = editor_path_complete_entries("/no/such/dir/exists-12345/", "x",
	                                  entries, 8, lcp, sizeof(lcp));
	CHECK(n == -1);
}

/* Empty prefix matches every visible entry; LCP collapses to "".  Dotfiles
 * are hidden because the prefix does not start with '.'. */
static void test_empty_prefix(void)
{
	struct path_entry entries[8];
	char lcp[256];
	int n;

	setup();
	n = editor_path_complete_entries(scratch, "", entries, 8, lcp, sizeof(lcp));
	CHECK(n == 5);            /* foobar, foobaz, foe, README, subdir */
	CHECK(lcp[0] == '\0');
	teardown();
}

/* A leading-dot prefix opts dotfiles back in. */
static void test_dot_prefix_shows_hidden(void)
{
	struct path_entry entries[8];
	char lcp[256];
	int n;

	setup();
	n = editor_path_complete_entries(scratch, ".h", entries, 8, lcp, sizeof(lcp));
	CHECK(n == 1);
	CHECK(strcmp(lcp, ".hidden") == 0);
	CHECK(strcmp(entries[0].name, ".hidden") == 0);
	teardown();
}

/* When matches exceed the entries cap, the returned count is still the
 * total; entries[] holds some `max`-sized alphabetically-sorted subset
 * of the matches (which subset depends on readdir order — callers that
 * care should narrow with a longer prefix). */
static void test_max_clamp(void)
{
	struct path_entry entries[2];
	char lcp[256];
	int n;

	setup();
	n = editor_path_complete_entries(scratch, "fo", entries, 2, lcp, sizeof(lcp));
	CHECK(n == 3);
	CHECK(strncmp(entries[0].name, "fo", 2) == 0);
	CHECK(strncmp(entries[1].name, "fo", 2) == 0);
	CHECK(strcmp(entries[0].name, entries[1].name) < 0);
	teardown();
}

/* match_rank: 0 for prefix match, 1 for mid-name, -1 for no match. */
static void test_picker_match_rank(void)
{
	/* Prefix matches rank 0. */
	CHECK(editor_picker_match_rank("buffer.c", "buf") == 0);
	CHECK(editor_picker_match_rank("buffer.c", "buffer.c") == 0);
	CHECK(editor_picker_match_rank("buffer.c", "b") == 0);

	/* Mid-name matches rank 1. */
	CHECK(editor_picker_match_rank("editor_buffer.c", "buf") == 1);
	CHECK(editor_picker_match_rank("mgr_buf", "buf") == 1);
	CHECK(editor_picker_match_rank("bufmgr.c", "mgr") == 1);

	/* No match returns -1. */
	CHECK(editor_picker_match_rank("buffer.c", "xyz") == -1);
	CHECK(editor_picker_match_rank("", "x") == -1);

	/* Empty needle ranks every candidate as a prefix match. */
	CHECK(editor_picker_match_rank("anything", "") == 0);
	CHECK(editor_picker_match_rank("", "") == 0);

	/* NULL needle is treated as empty (defensive). */
	CHECK(editor_picker_match_rank("anything", NULL) == 0);
}

/* Substring picker filtering surfaces hits found mid-name, with prefix
 * matches sorted above. */
static void test_substring_matching(void)
{
	struct path_entry entries[8];
	char lcp[256];
	int n;

	setup();
	touch("aaabuf");      /* prefix-of-mid? no — needle is "buf", "aaabuf" has buf mid-name */
	touch("buffile");     /* prefix match */
	touch("zbufz");       /* mid-name match */

	n = editor_path_complete_entries(scratch, "buf", entries, 8, lcp, sizeof(lcp));
	CHECK(n == 3);

	/* "buffile" should sort first (prefix match), then the mid-name
	 * matches in alphabetical order. */
	CHECK(strcmp(entries[0].name, "buffile") == 0);
	CHECK(strcmp(entries[1].name, "aaabuf") == 0);
	CHECK(strcmp(entries[2].name, "zbufz") == 0);

	/* LCP is computed over the prefix-matched group only, so a single
	 * prefix match yields the full name. */
	CHECK(strcmp(lcp, "buffile") == 0);
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
	RUN(test_max_clamp);
	RUN(test_picker_match_rank);
	RUN(test_substring_matching);
	return test_summary();
}
