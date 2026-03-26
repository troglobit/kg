/* test.c — global definitions for the test harness */

#include <stdlib.h>
#include "test.h"
#include "../src/def.h"

int tests_run    = 0;
int tests_failed = 0;

void free_all_rows(void)
{
	int i;

	for (i = 0; i < editor.numrows; i++)
		editor_free_row(&editor.row[i]);
	free(editor.row);
}
