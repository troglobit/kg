/* ======================== Keyboard event handling ========================= */

#include "def.h"

/* Repeat counts big enough to overflow the multiply, or to wedge the
 * editor for minutes, are clamped; nothing sensible repeats more often. */
#define PREFIX_ARG_MAX 100000

/* C-u universal-argument: accumulate a numeric prefix.  Returns 1 if `c`
 * was part of the in-progress prefix (digit, another C-u, or C-g cancel)
 * and the caller should stop processing this key.  Returns 0 if `c` is a
 * real command — by then editor.prefix_pending is cleared and the count
 * is committed in editor.prefix_arg, waiting to be picked up. */
static int handle_universal_arg(int c)
{
	if (!editor.prefix_pending) {
		if (c == CTRL_U) {
			editor.prefix_pending   = 1;
			editor.prefix_arg       = 4;
			editor.prefix_no_digits = 1;
			editor_set_status_message("C-u");
			return 1;
		}
		return 0;
	}

	if (c == CTRL_U) {
		editor.prefix_arg *= 4;
		if (editor.prefix_arg > PREFIX_ARG_MAX)
			editor.prefix_arg = PREFIX_ARG_MAX;
		editor_set_status_message("C-u %d", editor.prefix_arg);
		return 1;
	}
	if (c >= '0' && c <= '9') {
		int digit = c - '0';
		if (editor.prefix_no_digits) {
			editor.prefix_arg       = digit;
			editor.prefix_no_digits = 0;
		} else {
			editor.prefix_arg = editor.prefix_arg * 10 + digit;
		}
		if (editor.prefix_arg > PREFIX_ARG_MAX)
			editor.prefix_arg = PREFIX_ARG_MAX;
		editor_set_status_message("C-u %d", editor.prefix_arg);
		return 1;
	}
	if (c == CTRL_G) {
		editor.prefix_pending   = 0;
		editor.prefix_arg       = 0;
		editor.prefix_no_digits = 0;
		editor_set_status_message("");
		return 1;
	}

	editor.prefix_pending   = 0;
	editor.prefix_no_digits = 0;
	return 0;
}

/* Map a shift+motion key to its unshifted motion, or 0 when `c` is not
 * one.  Shift+motion extends a shift-selected region instead of tearing
 * it down; the motion itself dispatches like the unshifted key. */
static int shift_motion_base(int c)
{
	switch (c) {
	case SHIFT_ARROW_LEFT:       return ARROW_LEFT;
	case SHIFT_ARROW_RIGHT:      return ARROW_RIGHT;
	case SHIFT_ARROW_UP:         return ARROW_UP;
	case SHIFT_ARROW_DOWN:       return ARROW_DOWN;
	case SHIFT_HOME:             return HOME_KEY;
	case SHIFT_END:              return END_KEY;
	case SHIFT_PAGE_UP:          return PAGE_UP;
	case SHIFT_PAGE_DOWN:        return PAGE_DOWN;
	case CTRL_SHIFT_ARROW_LEFT:  return CTRL_ARROW_LEFT;
	case CTRL_SHIFT_ARROW_RIGHT: return CTRL_ARROW_RIGHT;
	case CTRL_SHIFT_ARROW_UP:    return CTRL_ARROW_UP;
	case CTRL_SHIFT_ARROW_DOWN:  return CTRL_ARROW_DOWN;
	case CTRL_SHIFT_HOME:        return CTRL_HOME;
	case CTRL_SHIFT_END:         return CTRL_END;
	}
	return 0;
}

/* Quit (C-x C-c, F10), prompting when modified file-backed buffers
 * would be lost. */
static void editor_quit(int fd)
{
	int i, ndirty = 0;

	/* Offer to save each modified buffer, like GNU Emacs; C-g at a
	 * save prompt cancels the quit. */
	if (buf_save_all(fd))
		return;

	/* Count what the user left unsaved.  buf_save_all flushed the current
	 * buffer into its slot, so every slot's dirty flag is authoritative. */
	for (i = 0; i < MAX_BUFFERS; i++) {
		if (!buflist[i].active || !buflist[i].dirty) continue;
		if (is_special_buffer(buflist[i].filename)) continue;
		ndirty++;
	}
	if (ndirty) {
		int answer;
		editor_set_status_message(
			ndirty == 1 ? "One modified buffer remains, exit anyway? (y/n) "
			            : "%d modified buffers remain, exit anyway? (y/n) ",
			ndirty);
		editor_refresh_screen();
		answer = editor_read_key(fd);
		if (answer != 'y' && answer != 'Y') {
			editor_set_status_message("");
			return;
		}
	}
	running = 0;
}

/* Process events arriving from the standard input, which is, the user
 * is typing stuff on the terminal. */
void editor_process_keypress(int fd)
{
	struct timeval tv;
	int c = editor_read_key_idle(fd);
	char *fname_before = editor.filename;
	int dirty_before = editor.dirty;
	int was_shift_select = editor.shift_select;
	long elapsed;
	int base = 0;
	int n;

	/* Paste mode detection: if characters arrive very quickly (< 30ms apart),
	 * we're likely in a paste operation, so disable autocompletion */
	gettimeofday(&tv, NULL);
	if (tv.tv_sec == editor.last_char_time.tv_sec) {
		elapsed = (tv.tv_usec - editor.last_char_time.tv_usec);
		if (elapsed < 30000)
			editor.paste_mode = 1; /* 30ms threshold */
	} else if (tv.tv_sec - editor.last_char_time.tv_sec > 0) {
		editor.paste_mode = 0;
	}
	editor.last_char_time = tv;

	/* Handle C-x r rectangle ops (second key after C-x r).  Every op
	 * here mutates the buffer, so a read-only buffer rejects them
	 * outright; only C-g (cancel) still has any business reaching
	 * the inner switch. */
	if (editor.rect_prefix) {
		editor.rect_prefix = 0;
		if (editor.readonly && c != CTRL_G) {
			editor_set_status_message("Buffer is read-only");
			return;
		}
		switch (c) {
		case 'k': case CTRL_K: editor_kill_rect();   break;
		case 'd':              editor_delete_rect(); break;
		case 'c':              editor_clear_rect();  break;
		case 'y': case CTRL_Y: editor_yank_rect();   break;
		case CTRL_G:           editor_set_status_message(""); break;
		default:               editor_set_status_message("C-x r %c is undefined", c); break;
		}
		return;
	}

	/* Handle C-x prefix commands */
	if (editor.cx_prefix) {
		editor.cx_prefix = 0;
		switch (c) {
		case CTRL_C:    /* C-x C-c: Quit */
			editor_quit(fd);
			break;
		case CTRL_S:    /* C-x C-s: Save */
			editor_save(fd);
			break;
		case 'f':       /* C-x f: Set fill column (per-buffer) */
			editor_set_fill_column(fd);
			break;
		case 's':       /* C-x s: Save all modified buffers */
			buf_save_all(fd);
			break;
		case CTRL_F:    /* C-x C-f: Open file in new buffer */
			buf_open_file(fd);
			break;
		case CTRL_R:    /* C-x C-r: Open file read-only */
			buf_open_file_read_only(fd);
			break;
		case 'b':       /* C-x b: Interactive buffer select */
			buf_select_interactive(fd);
			break;
		case 'k':       /* C-x k: Kill current buffer */
			buf_kill(fd);
			break;
		case CTRL_B:    /* C-x C-b: Open buffer list */
			buf_open_list();
			break;
		case '2':       /* C-x 2: Split window horizontally */
			win_split_horizontal();
			break;
		case '3':       /* C-x 3: Split window vertically */
			win_split_vertical();
			break;
		case 'o':       /* C-x o: Other window */
			win_cycle_next();
			break;
		case '0':       /* C-x 0: Delete current window */
			win_delete_current();
			break;
		case '1':       /* C-x 1: Delete other windows */
			win_delete_others();
			break;
		case '+':       /* C-x +: Balance windows */
			win_balance();
			break;
		case '^':       /* C-x ^: Enlarge window */
			win_enlarge_v(1);
			break;
		case '}':       /* C-x }: Enlarge window horizontally */
			win_enlarge_h(1);
			break;
		case '{':       /* C-x {: Shrink window horizontally */
			win_enlarge_h(-1);
			break;
		case CTRL_X:    /* C-x C-x: Exchange point and mark */
			editor_exchange_point_and_mark();
			break;
		case CTRL_W:    /* C-x C-w: Write file (save as) */
			editor_write_file(fd);
			break;
		case 'i':       /* C-x i: Insert file at point */
			editor_insert_file(fd);
			break;
		case CTRL_Q:    /* C-x C-q: Toggle read-only */
			editor.readonly = !editor.readonly;
			editor_set_status_message(editor.readonly ? "Read-only" : "Writable");
			break;
		case '(':       /* C-x (: Start keyboard macro */
			macro_start();
			break;
		case ')':       /* C-x ): Stop keyboard macro (trim C-x + ')' from buffer) */
			macro_stop(2);
			break;
		case 'e':       /* C-x e: Execute keyboard macro */
			macro_replay(fd);
			break;
		case ' ':       /* C-x SPC: rectangle-mark-mode */
			editor_rect_mode_toggle();
			break;
		case 'r':       /* C-x r-: rectangle operation prefix */
			editor.rect_prefix = 1;
			editor_set_status_message("C-x r-");
			break;
		case CTRL_G:    /* C-x C-g: Cancel C-x prefix */
			editor_set_status_message("");
			break;
		default:
			editor_set_status_message("C-x %c is undefined", c);
			break;
		}
		return;
	}

	if (handle_universal_arg(c))
		return;

	/* Consume the pending C-u count now, before any of the early-return
	 * filters below have a chance to drop the key.  Whichever branch ends
	 * up handling this keystroke either uses `n` or implicitly discards
	 * it; either way the next keypress starts fresh. */
	if (editor.prefix_arg > 0) {
		n = editor.prefix_arg;
		editor.prefix_arg = 0;
		editor_set_status_message("");
	} else {
		n = 1;
	}

	/* q closes special *...* buffers, but only if another buffer exists */
	if (c == 'q' && is_special_buffer(editor.filename) && buf_count > 1) {
		buf_kill(fd);
		return;
	}

	/* In a read-only buffer such as the *Buffer List*, Enter opens the item
	 * at point.  Editing itself is refused by the mutation commands, which
	 * bail via editor_readonly_blocked(). */
	if (editor.readonly && c == ENTER) {
		buf_ibuffer_select();
		return;
	}

	/* Reset cycle states if the previous key wasn't the cycling command. */
	if (editor.last_key != CTRL_L)
		editor.recenter_state = 0;
	if (editor.last_key != ALT_R)
		editor.window_line_state = 0;
	editor.last_key = c;

	/* Shift+motion: drop the mark at the current position the first
	 * time the user starts a shift-selected region, so subsequent
	 * shift+motion extends it.  If a region is already on-screen we
	 * just extend.  The key then dispatches as the unshifted motion. */
	base = shift_motion_base(c);
	if (base) {
		if (!editor.mark_highlight) {
			editor_set_mark_silent();
			editor.shift_select = 1;
		}
		c = base;
	}

	/* Regular key processing */
	switch (c) {
	case KEY_NULL:      /* Ctrl+Space - set mark */
		editor_set_mark();
		break;
	case ENTER:         /* Enter */
		while (n--) editor_insert_newline();
		break;
	case CTRL_A:        /* Beginning of line */
		editor_move_cursor(HOME_KEY);
		break;
	case CTRL_B:        /* Backward char */
		while (n--) editor_move_cursor(ARROW_LEFT);
		break;
	case CTRL_D:        /* Delete char forward */
		while (n--) editor_del_forward_char();
		break;
	case CTRL_E:        /* End of line */
		editor_move_cursor(END_KEY);
		break;
	case CTRL_F:        /* Forward char */
		while (n--) editor_move_cursor(ARROW_RIGHT);
		break;
	case CTRL_G:        /* Keyboard quit / cancel */
		editor.mark_highlight = 0;
		editor.rect_mode = 0;
		editor_snap_cx_to_row();
		editor_set_status_message("");
		break;
	case CTRL_K:        /* Kill line */
		if (n > 1) {
			/* Kill N logical lines (each = content-to-EOL + newline),
			 * matching Emacs' C-u N C-k.  editor_kill_line() is a half-
			 * step primitive, so we count *newlines* removed (numrows
			 * dropped) rather than iterations.  A stalled kill_ring tells
			 * us we hit EOF and should stop. */
			int start_row     = editor.rowoff + editor.cy;
			int start_col     = editor.coloff + editor.cx;
			int prev_kill_len = killring.len;
			int newlines_left = n;
			int killed_len;
			suppress_undo = 1;
			while (newlines_left > 0) {
				int before_numrows  = editor.numrows;
				int before_ring_len = killring.len;
				editor_kill_line();
				if (killring.len == before_ring_len)
					break;
				if (editor.numrows < before_numrows)
					newlines_left--;
			}
			suppress_undo = 0;
			killed_len = killring.len - prev_kill_len;
			if (killed_len > 0)
				undo_push(UNDO_KILL_TEXT, start_row, start_col, 0,
					  killring.text + prev_kill_len, killed_len);
		} else {
			editor_kill_line();
		}
		break;
	case CTRL_O:        /* Open line */
		while (n--) editor_open_line();
		break;
	case CTRL_N:        /* Next line */
		while (n--) editor_move_cursor(ARROW_DOWN);
		break;
	case CTRL_P:        /* Previous line */
		while (n--) editor_move_cursor(ARROW_UP);
		break;
	case CTRL_S:        /* Incremental search forward */
		editor_find(fd, 1);
		break;
	case CTRL_R:        /* Incremental search backward */
		editor_find(fd, -1);
		break;
	case CTRL_Q: {
		int key;

		if (editor_readonly_blocked())
			break;
		key = editor_read_raw_byte(fd);
		if (running)
			editor_insert_char(key);
		break;
	}
	case CTRL_V:        /* Page down */
		if (editor.cy != editor.screenrows - 1)
			editor.cy = editor.screenrows - 1;
		{
		int times = editor.screenrows;
		while (times--)
			editor_move_cursor(ARROW_DOWN);
		}
		break;
	case CTRL_W:        /* Kill region (cut) */
	case SHIFT_DELETE:  /* CUA cut */
		editor_kill_region();
		break;
	case CTRL_X:        /* C-x prefix */
		editor.cx_prefix = 1;
		editor_set_status_message("C-x-");
		return;
	case CTRL_Y:        /* Yank (paste) */
	case SHIFT_INSERT:  /* CUA paste */
		if (editor_readonly_blocked())
			break;
		if (n > 1 && killring.text && killring.len > 0) {
			/* Batch N yanks under one undo: UNDO_YANK_TEXT reverses by
			 * deleting len chars forward, so the record must carry the
			 * full N-copy payload size for the reversal to be complete. */
			int start_row = editor.rowoff + editor.cy;
			int start_col = editor.coloff + editor.cx;
			int total_len = n * killring.len;
			char *combined = malloc(total_len);
			if (combined) {
				int i;
				for (i = 0; i < n; i++)
					memcpy(combined + i * killring.len,
					       killring.text, killring.len);
				undo_push(UNDO_YANK_TEXT, start_row, start_col, 0,
					  combined, total_len);
				free(combined);
				suppress_undo = 1;
				while (n--) editor_yank();
				suppress_undo = 0;
			} else {
				while (n--) editor_yank();
			}
		} else {
			editor_yank();
		}
		break;
	case CTRL_UNDERSCORE: /* Undo (C-_ or C-/) */
		while (n--) editor_undo();
		break;
	case CTRL_H:        /* Help */
		buf_open_help();
		break;
	case CTRL_Z:        /* Suspend to shell */
		editor_suspend();
		break;
	case BACKSPACE:     /* Backspace */
		while (n--) editor_del_char();
		break;
	case DEL_KEY:       /* Forward delete; consumes an active region first. */
		if (editor.mark_set && editor.mark_highlight)
			editor_delete_region_or_char();
		else
			while (n--) editor_del_forward_char();
		break;
	case PAGE_UP:
	case PAGE_DOWN:
		if (c == PAGE_UP && editor.cy != 0)
			editor.cy = 0;
		else if (c == PAGE_DOWN && editor.cy != editor.screenrows - 1)
			editor.cy = editor.screenrows - 1;
		{
		int times = editor.screenrows;
		while (times--)
			editor_move_cursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
		}
		break;

	case ARROW_UP:
	case ARROW_DOWN:
	case ARROW_LEFT:
	case ARROW_RIGHT:
		while (n--) editor_move_cursor(c);
		break;
	case HOME_KEY:
	case END_KEY:
		editor_move_cursor(c);
		break;
	case CTRL_HOME:
	case CTRL_PAGE_UP:
	case ALT_LT:
		editor_move_to_beginning();
		break;
	case CTRL_END:
	case CTRL_PAGE_DOWN:
	case ALT_GT:
		editor_move_to_end();
		break;
	case ALT_G:         /* Goto line */
		editor_goto_line(fd);
		break;
	case CTRL_ARROW_LEFT:
	case ALT_B:
		while (n--) editor_move_word_backward();
		break;
	case CTRL_ARROW_RIGHT:
	case ALT_F:
		while (n--) editor_move_word_forward();
		break;
	case ALT_D:             /* Kill word forward */
		while (n--) editor_kill_word_forward();
		break;
	case ALT_BACKSPACE:     /* Kill word backward */
		while (n--) editor_kill_word_backward();
		break;
	case CTRL_ARROW_UP:
	case ALT_LBRACE:
		while (n--) editor_move_paragraph_backward();
		break;
	case CTRL_ARROW_DOWN:
	case ALT_RBRACE:
		while (n--) editor_move_paragraph_forward();
		break;
	case ALT_M:         /* M-m: back-to-indentation */
		editor_move_to_indentation();
		break;
	case ALT_A:         /* M-a: backward sentence */
		while (n--) editor_move_sentence_backward();
		break;
	case ALT_E:         /* M-e: forward sentence */
		while (n--) editor_move_sentence_forward();
		break;
	case ALT_R:         /* M-r: top/middle/bottom of window cycle */
		editor_move_to_window_line();
		break;
	case ALT_ARROW_LEFT:    /* M-arrow: select window in that direction */
		win_move_dir(-1, 0);
		break;
	case ALT_ARROW_RIGHT:
		win_move_dir(1, 0);
		break;
	case ALT_ARROW_UP:
		win_move_dir(0, -1);
		break;
	case ALT_ARROW_DOWN:
		win_move_dir(0, 1);
		break;
	case ALT_SHIFT_ARROW_LEFT:  /* M-S-arrow: divider travels with arrow */
		win_resize_dir(-1, 0, n);
		break;
	case ALT_SHIFT_ARROW_RIGHT:
		win_resize_dir(1, 0, n);
		break;
	case ALT_SHIFT_ARROW_UP:
		win_resize_dir(0, -1, n);
		break;
	case ALT_SHIFT_ARROW_DOWN:
		win_resize_dir(0, 1, n);
		break;
	case ALT_V:         /* Page up */
		if (editor.cy != 0)
			editor.cy = 0;
		{
		int times = editor.screenrows;
		while (times--)
			editor_move_cursor(ARROW_UP);
		}
		break;
	case ALT_W:         /* Copy region */
	case CTRL_INSERT:   /* CUA copy */
		editor_copy_region();
		break;
	case ALT_Q:         /* Reflow paragraph */
		editor_reflow_paragraph();
		break;
	case ALT_PCT:       /* Query replace */
		editor_query_replace(fd);
		break;
	case ALT_SEMICOLON: /* Toggle line comment */
		editor_comment_dwim();
		break;
	case ALT_X:         /* Named command */
		editor_named_command(fd);
		break;
	case ALT_CARET:     /* Join current line with previous */
		while (n--) editor_join_line();
		break;
	case ALT_U:         /* Upcase word forward */
		while (n--) editor_upcase_word();
		break;
	case ALT_L:         /* Downcase word forward */
		while (n--) editor_downcase_word();
		break;
	case ALT_C:         /* Capitalize word forward */
		while (n--) editor_capitalize_word();
		break;
	case ALT_BANG:      /* Shell command */
		editor_shell_command(fd);
		break;
	case ALT_PIPE:      /* Shell command on region */
		editor_shell_command_on_region(fd);
		break;
	case KEY_F1:        /* F1: Help */
		buf_open_help();
		break;
	case KEY_F2:        /* F2: Save */
		editor_save(fd);
		break;
	case KEY_F10:       /* F10: Quit */
		editor_quit(fd);
		break;
	case KEY_F3:        /* F3: Start keyboard macro */
		macro_start();
		break;
	case KEY_F4:        /* F4: Stop if recording, else execute */
		if (macro_is_recording())
			macro_stop(1);
		else
			macro_replay(fd);
		break;
	case CTRL_L: {      /* Recenter: cycle center → top → bottom */
		int filerow = editor.rowoff + editor.cy;
		switch (editor.recenter_state) {
		case 0: /* center */
			editor.rowoff = filerow - editor.screenrows / 2;
			break;
		case 1: /* top */
			editor.rowoff = filerow;
			break;
		default: /* bottom */
			editor.rowoff = filerow - (editor.screenrows - 1);
			break;
		}
		if (editor.rowoff < 0) editor.rowoff = 0;
		editor.cy = filerow - editor.rowoff;
		editor.recenter_state = (editor.recenter_state + 1) % 3;
		probe_window_size();
		tty_write("\x1b[2J", 4);
		editor_refresh_screen();
		break;
	}
	case ESC:
		/* Nothing to do for ESC in this mode. */
		break;
	default:
		/* Filter out control characters and non-printable characters.
		 * Only allow printable ASCII (32-126) and TAB.  (ENTER is handled
		 * as its own case above and would never reach here.)  Repeats N
		 * times when a C-u prefix preceded the key. */
		if (c == TAB || (c >= 32 && c < 127))
			while (n--) editor_insert_char_auto_complete(c);
		/* Silently ignore all other control/non-printable characters */
		break;
	}

	/* Any command that modified the buffer (insertion, deletion, undo,
	 * etc.) deactivates the visual mark region — matching Emacs'
	 * transient-mark-mode convention.  The filename guard avoids
	 * stomping the highlight that was just restored from a buffer slot
	 * when the user switched buffers (C-x b, C-x C-f). */
	if (editor.filename == fname_before && editor.dirty != dirty_before) {
		editor.mark_highlight = 0;
		editor.rect_mode = 0;
		editor_snap_cx_to_row();
	}

	/* Tear down a shift-selected region after the command has had its
	 * say.  Done last so C-w / M-w / C-x C-x can still see the mark
	 * during their dispatch.  Extender keys keep the region alive; a
	 * C-x prefix keystroke also keeps it (the follow-up may consume
	 * the region). */
	if (was_shift_select && !editor.cx_prefix && !base) {
		editor.shift_select = 0;
		editor.mark_set = 0;
		editor.mark_highlight = 0;
		editor.rect_mode = 0;
		editor_snap_cx_to_row();
	}

	/* Goal column is only valid between consecutive vertical motions —
	 * any other key invalidates it.  Keep-list mirrors every key that
	 * eventually routes through editor_move_cursor(ARROW_UP/DOWN). */
	if (c != ARROW_UP && c != ARROW_DOWN &&
	    c != PAGE_UP  && c != PAGE_DOWN  &&
	    c != CTRL_N   && c != CTRL_P     &&
	    c != CTRL_V   && c != ALT_V)
		editor.desired_visual_col = -1;
}
