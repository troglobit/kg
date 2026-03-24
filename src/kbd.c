/* ======================== Keyboard event handling ========================= */

#include "def.h"

/* Process events arriving from the standard input, which is, the user
 * is typing stuff on the terminal. */
void editor_process_keypress(int fd)
{
	struct timeval tv;
	int c = editor_read_key(fd);
	long elapsed;

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

	/* Help screen: any key dismisses it. */
	if (editor.show_help) {
		editor.show_help = 0;
		return;
	}

	/* Handle C-x prefix commands */
	if (editor.cx_prefix) {
		editor.cx_prefix = 0;
		switch (c) {
		case CTRL_C: {  /* C-x C-c: Quit */
			int i, ndirty = 0;
			/* Count modified real-file buffers (exclude *special* ones). */
			if (editor.dirty && !is_special_buffer(editor.filename))
				ndirty++;
			for (i = 0; i < MAX_BUFFERS; i++) {
				if (!buflist[i].active || i == buf_current) continue;
				if (!buflist[i].dirty) continue;
				if (is_special_buffer(buflist[i].filename)) continue;
				ndirty++;
			}
			if (ndirty) {
				int answer;
				editor_set_status_message(
					ndirty == 1 ? "Modified buffer, really quit? (y/n) "
					            : "%d modified buffers, really quit? (y/n) ",
					ndirty);
				editor_refresh_screen();
				answer = editor_read_key(fd);
				if (answer != 'y' && answer != 'Y') {
					editor_set_status_message("");
					return;
				}
			}
			running = 0;
			break;
		}
		case CTRL_S:    /* C-x C-s: Save */
			editor_save(fd);
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
		case CTRL_Q:    /* C-x C-q: Toggle read-only */
			editor.readonly = !editor.readonly;
			editor_set_status_message(editor.readonly ? "Read-only" : "Writable");
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

	/* q closes special *...* buffers without dirty prompt */
	if (c == 'q' && is_special_buffer(editor.filename)) {
		buf_kill(fd);
		return;
	}

	/* Read-only mode: Enter opens the item at point; editing is blocked. */
	if (editor.readonly) {
		if (c == ENTER) {
			buf_ibuffer_select();
			return;
		}
		if (c == BACKSPACE || c == DEL_KEY || c == CTRL_D ||
		    c == CTRL_K    || c == CTRL_W  || c == CTRL_Y ||
		    c == CTRL_UNDERSCORE || c == TAB ||
		    (c >= 32 && c < 127)) {
			editor_set_status_message("Buffer is read-only");
			return;
		}
	}

	/* Reset recenter cycle if the previous key was not C-l. */
	if (editor.last_key != CTRL_L)
		editor.recenter_state = 0;
	editor.last_key = c;

	/* Regular key processing */
	switch (c) {
	case KEY_NULL:      /* Ctrl+Space - set mark */
		editor_set_mark();
		break;
	case ENTER:         /* Enter */
		editor_insert_newline();
		break;
	case CTRL_A:        /* Beginning of line */
		editor_move_cursor(HOME_KEY);
		break;
	case CTRL_B:        /* Backward char */
		editor_move_cursor(ARROW_LEFT);
		break;
	case CTRL_D:        /* Delete char forward */
		editor_del_forward_char();
		break;
	case CTRL_E:        /* End of line */
		editor_move_cursor(END_KEY);
		break;
	case CTRL_F:        /* Forward char */
		editor_move_cursor(ARROW_RIGHT);
		break;
	case CTRL_G:        /* Keyboard quit / cancel */
		editor_set_status_message("");
		break;
	case CTRL_K:        /* Kill line */
		editor_kill_line();
		break;
	case CTRL_N:        /* Next line */
		editor_move_cursor(ARROW_DOWN);
		break;
	case CTRL_P:        /* Previous line */
		editor_move_cursor(ARROW_UP);
		break;
	case CTRL_S:        /* Incremental search */
		editor_find(fd);
		break;
	case CTRL_R:        /* Incremental search */
		editor_find(fd);
		break;
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
		editor_kill_region();
		break;
	case CTRL_X:        /* C-x prefix */
		editor.cx_prefix = 1;
		editor_set_status_message("C-x-");
		return;
	case CTRL_Y:        /* Yank (paste) */
		editor_yank();
		break;
	case CTRL_UNDERSCORE: /* Undo (C-_ or C-/) */
		editor_undo();
		break;
	case CTRL_H:        /* Help */
		editor_toggle_help();
		break;
	case CTRL_Z:        /* Suspend to shell */
		editor_suspend();
		break;
	case BACKSPACE:     /* Backspace */
		editor_del_char();
		break;
	case DEL_KEY:       /* Forward delete */
		editor_del_forward_char();
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
	case HOME_KEY:
	case END_KEY:
		editor_move_cursor(c);
		break;
	case CTRL_HOME:
		editor_move_to_beginning();
		break;
	case CTRL_END:
		editor_move_to_end();
		break;
	case ALT_G:         /* Goto line */
		editor_goto_line(fd);
		break;
	case CTRL_ARROW_LEFT:
	case ALT_B:
		editor_move_word_backward();
		break;
	case CTRL_ARROW_RIGHT:
	case ALT_F:
		editor_move_word_forward();
		break;
	case ALT_D:             /* Kill word forward */
		editor_kill_word_forward();
		break;
	case ALT_BACKSPACE:     /* Kill word backward */
		editor_kill_word_backward();
		break;
	case CTRL_ARROW_UP:
		editor_move_paragraph_backward();
		break;
	case CTRL_ARROW_DOWN:
		editor_move_paragraph_forward();
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
		editor_copy_region();
		break;
	case ALT_Q:         /* Reflow paragraph */
		editor_reflow_paragraph();
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
		 * Only allow printable ASCII (32-126) and TAB/ENTER.
		 * This prevents weird characters from appearing when pressing
		 * unhandled key combinations. In the future, we can add a compose
		 * key for UTF-8 input. */
		if (c == TAB || c == ENTER) {
			/* Allow TAB and ENTER */
			editor_insert_char_auto_complete(c);
		} else if (c >= 32 && c < 127) {
			/* Printable ASCII characters */
			editor_insert_char_auto_complete(c);
		}
		/* Silently ignore all other control/non-printable characters */
		break;
	}
}
