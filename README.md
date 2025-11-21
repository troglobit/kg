kg - A Small Text Editor with Emacs Keybindings
===============================================

kg is a minimal text editor inspired by Mg with Emacs keybindings and is
based on [kilo][0] and its descendants.

## Features

- Pure Emacs-style keybindings
- Syntax highlighting for many programming languages
- Incremental search
- No dependencies (not even curses)
- Uses standard VT100 escape sequences
- Single-file implementation
- Graceful terminal resize handling

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

| Key             | Action                    |
|-----------------|---------------------------|
| C-f             | Forward character         |
| C-b             | Backward character        |
| C-n             | Next line                 |
| C-p             | Previous line             |
| C-a / Home      | Beginning of line         |
| C-e / End       | End of line               |
| C-v             | Page down                 |
| M-v             | Page up                   |
| PgUp/PgDn       | Scroll by page            |
| M-f / C-Right   | Forward word              |
| M-b / C-Left    | Backward word             |
| C-Up            | Previous paragraph        |
| C-Down          | Next paragraph            |
| C-Home          | Beginning of document     |
| C-End           | End of document           |

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

### Copy/Paste (Kill Ring)

| Key       | Action                               |
|-----------|--------------------------------------|
| C-Space   | Set mark (start selection)           |
| C-w       | Kill region (cut selection)          |
| M-w       | Copy region (copy selection)         |
| C-y       | Yank (paste from kill ring)          |

**Usage**: Press C-Space to set the mark, move the cursor to select text,
then use C-w to cut or M-w to copy. Use C-y to paste. C-k also saves killed
text to the kill ring, and consecutive C-k commands append to the same entry.

### Undo

| Key       | Action                               |
|-----------|--------------------------------------|
| C-_ / C-/ | Undo last change                     |

**Note**: Undo maintains a stack of up to 1000 operations. Each character
insertion, deletion, line split, and line join is recorded separately.

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

## Language Support

kg provides syntax highlighting for 13 programming languages with
automatic detection based on file extension:

| Language   | Extensions                            | Features                             |
|------------|---------------------------------------|--------------------------------------|
| C/C++      | `.c`, `.h`, `.cpp`, `.hpp`, `.cc`     | Keywords, types, comments            |
| Python     | `.py`, `.pyw`, `.pyi`, `.pyx`         | Keywords, built-ins (len, map, etc.) |
| Shell      | `.sh`, `.bash`, `.zsh`, profile files | Keywords, commands, variables        |
| JavaScript | `.js`, `.jsx`, `.mjs`, `.cjs`         | ES6+, async/await, built-ins         |
| Rust       | `.rs`, `.rlib`                        | Keywords, std types, traits          |
| Java       | `.java`, `.class`                     | Keywords, common classes             |
| TypeScript | `.ts`, `.tsx`, `.d.ts`                | JS + TS types, interfaces            |
| C#         | `.cs`, `.csx`                         | Keywords, .NET types                 |
| PHP        | `.php`, `.phtml`, `.php3-5`           | Keywords, superglobals               |
| Ruby       | `.rb`, `.rbw`, `.rake`, `.gemspec`    | Keywords, built-in methods           |
| Swift      | `.swift`                              | Keywords, Foundation types           |
| SQL        | `.sql`, `.ddl`, `.dml`                | SQL keywords, functions              |
| Dart       | `.dart`                               | Keywords, Flutter types              |

## Origin & References

kg is based on [kilo][0] by Salvatore Sanfilippo (antirez), the original
minimal text editor that demonstrates how to build a functional editor
without dependencies in about 1000 lines of C code.

The name "kg" is a nod to "mg" (Micro Emacs), suggesting "kilo-gram" - a
minimal implementation with Emacs keybindings.

[0]: https://github.com/antirez/kilo
