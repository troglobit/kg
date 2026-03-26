/* test_yank.c — regression tests for the kill ring */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "../src/def.h"

/* ---- Helpers ---- */

static void setup(void)    { kill_ring_init(); }
static void teardown(void) { kill_ring_free(); }

/* ---- Tests ---- */

/* After init the ring is empty. */
static void test_init_empty(void)
{
	setup();
	CHECK(kill_ring_get() == NULL);
	teardown();
}

/* Setting text makes it retrievable and NUL-terminated. */
static void test_set_get(void)
{
	setup();
	kill_ring_set("hello", 5);
	CHECK(kill_ring_get() != NULL);
	CHECK(memcmp(kill_ring_get(), "hello", 5) == 0);
	CHECK(kill_ring_get()[5] == '\0');
	teardown();
}

/* Setting with len=0 is a no-op; the ring stays empty. */
static void test_set_zero_len(void)
{
	setup();
	kill_ring_set("x", 0);
	CHECK(kill_ring_get() == NULL);
	teardown();
}

/* A second set replaces the first content. */
static void test_set_replaces(void)
{
	setup();
	kill_ring_set("hello", 5);
	kill_ring_set("world", 5);
	CHECK(memcmp(kill_ring_get(), "world", 5) == 0);
	teardown();
}

/* Appending to an empty ring acts like set. */
static void test_append_to_empty(void)
{
	setup();
	kill_ring_append("hello", 5);
	CHECK(kill_ring_get() != NULL);
	CHECK(memcmp(kill_ring_get(), "hello", 5) == 0);
	teardown();
}

/* Consecutive appends concatenate the text. */
static void test_append_concatenates(void)
{
	setup();
	kill_ring_append("hello", 5);
	kill_ring_append(" world", 6);
	CHECK(memcmp(kill_ring_get(), "hello world", 11) == 0);
	teardown();
}

/* Three consecutive appends accumulate correctly. */
static void test_append_three(void)
{
	setup();
	kill_ring_append("foo", 3);
	kill_ring_append("bar", 3);
	kill_ring_append("baz", 3);
	CHECK(memcmp(kill_ring_get(), "foobarbaz", 9) == 0);
	teardown();
}

/* Appending with len=0 is a no-op; existing content is preserved. */
static void test_append_zero_len(void)
{
	setup();
	kill_ring_set("hello", 5);
	kill_ring_append("x", 0);
	CHECK(memcmp(kill_ring_get(), "hello", 5) == 0);
	teardown();
}

/* Free clears the ring to NULL. */
static void test_free_clears(void)
{
	setup();
	kill_ring_set("hello", 5);
	kill_ring_free();
	CHECK(kill_ring_get() == NULL);
	teardown();
}

/* Double free is safe (idempotent). */
static void test_free_idempotent(void)
{
	setup();
	kill_ring_set("hello", 5);
	kill_ring_free();
	kill_ring_free();   /* must not crash */
	CHECK(kill_ring_get() == NULL);
	teardown();
}

/* Set after free works correctly. */
static void test_set_after_free(void)
{
	setup();
	kill_ring_set("first", 5);
	kill_ring_free();
	kill_ring_set("second", 6);
	CHECK(memcmp(kill_ring_get(), "second", 6) == 0);
	teardown();
}

/* ---- Main ---- */

int main(void)
{
	RUN(test_init_empty);
	RUN(test_set_get);
	RUN(test_set_zero_len);
	RUN(test_set_replaces);
	RUN(test_append_to_empty);
	RUN(test_append_concatenates);
	RUN(test_append_three);
	RUN(test_append_zero_len);
	RUN(test_free_clears);
	RUN(test_free_idempotent);
	RUN(test_set_after_free);
	return test_summary();
}
