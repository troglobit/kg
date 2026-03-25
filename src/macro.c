/* ========================= Keyboard macros ================================= */

#include "def.h"

#define MACRO_MAX 1024

static int macro_keys[MACRO_MAX];
static int macro_len       = 0;
static int macro_pos       = 0;
static int macro_recording = 0;
static int macro_replaying = 0;

int macro_is_recording(void) { return macro_recording; }

/* Called by editor_read_key: append key to buffer while recording.
 * Skipped during replay so we don't corrupt the buffer with replayed keys. */
void macro_on_key(int key)
{
	if (macro_recording && !macro_replaying && macro_len < MACRO_MAX)
		macro_keys[macro_len++] = key;
}

/* Called by editor_read_key: return next pre-recorded key during replay,
 * or -1 when the buffer is exhausted (fall back to terminal). */
int macro_next_key(void)
{
	if (macro_replaying && macro_pos < macro_len)
		return macro_keys[macro_pos++];
	return -1;
}

/* C-x ( or F3: begin recording.  Resets any previously recorded macro. */
void macro_start(void)
{
	if (macro_replaying) {
		editor_set_status_message("Can't define macro while replaying");
		return;
	}
	macro_len       = 0;
	macro_recording = 1;
	editor_set_status_message("Defining macro...");
}

/* C-x ) or F4 (while recording): stop recording.
 * trim: number of stop-key codes to discard from the end of the buffer
 * (the stop keys themselves were recorded before we knew to stop). */
void macro_stop(int trim)
{
	if (!macro_recording) {
		editor_set_status_message("Not defining a macro");
		return;
	}
	macro_recording = 0;
	macro_len -= trim;
	if (macro_len < 0) macro_len = 0;
	editor_set_status_message("Macro defined (%d key%s)",
	                          macro_len, macro_len == 1 ? "" : "s");
}

/* C-x e or F4 (when not recording): replay the last recorded macro. */
void macro_replay(int fd)
{
	if (macro_recording) {
		editor_set_status_message("Can't replay while defining macro");
		return;
	}
	if (macro_replaying)
		return;    /* no recursion */
	if (macro_len == 0) {
		editor_set_status_message("No macro defined");
		return;
	}
	macro_replaying = 1;
	macro_pos       = 0;
	while (macro_pos < macro_len && running)
		editor_process_keypress(fd);
	macro_replaying = 0;
	if (running)
		editor_set_status_message("Macro replayed (%d key%s)",
		                          macro_len, macro_len == 1 ? "" : "s");
}
