kg - A Small Text Editor with Emacs Keybindings
===============================================

kg is a minimal text editor inspired by Mg with Emacs keybindings and is
based on [kilo][0].

## Features

- Pure Emacs-style keybindings
- Syntax highlighting for C/C++
- Incremental search
- No dependencies (not even curses)
- Uses standard VT100 escape sequences
- Single-file implementation

## Usage

```bash
kg <filename>
```

## Keybindings

### File Operations

| Key       | Action                    |
|-----------|---------------------------|
| C-x C-s   | Save file                 |
| C-x C-c   | Quit editor               |

### Navigation

| Key       | Action                    |
|-----------|---------------------------|
| C-f       | Forward character         |
| C-b       | Backward character        |
| C-n       | Next line                 |
| C-p       | Previous line             |
| C-a       | Beginning of line         |
| C-e       | End of line               |
| PgUp/PgDn | Scroll by page            |

Arrow keys (↑ ↓ ← →) also work for navigation.

### Editing

| Key       | Action                               |
|-----------|--------------------------------------|
| C-k       | Kill line (from cursor to end)       |
| C-d       | Delete character at cursor           |
| C-h       | Delete character before cursor       |
| Backspace | Delete character before cursor       |
| Delete    | Delete character at cursor           |
| Enter     | Insert newline                       |

**C-k behavior**: When pressed at the end of a line, it joins the
current line with the next line (standard Emacs behavior).

### Search

| Key       | Action                    |
|-----------|---------------------------|
| C-s       | Incremental search        |

In search mode:

- Type to add to search query
- C-s or → or ↓ to find next occurrence
- ← or ↑ to find previous occurrence
- Enter to accept and stay at match
- ESC to cancel and return to original position
- Backspace/C-h to remove from query

### Utility

| Key       | Action                    |
|-----------|---------------------------|
| C-g       | Keyboard quit / Cancel    |
| C-l       | Refresh screen            |

### C-x Prefix Commands

After pressing C-x, the editor waits for a second key:
- **C-x C-s**: Save file
- **C-x C-c**: Quit (requires 3 presses if file has unsaved changes)
- **C-x C-g**: Cancel the C-x prefix

Any other key after C-x will display an "undefined" message.

## Origin & References

kg is based on [kilo][0] by Salvatore Sanfilippo (antirez), the original
minimal text editor that demonstrates how to build a functional editor
without dependencies in about 1000 lines of C code.

The name "kg" is a nod to "mg" (Micro Emacs), suggesting "kilo-gram" - a
minimal implementation with Emacs keybindings.

[0]: https://github.com/antirez/kilo
