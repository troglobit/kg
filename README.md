kg - A Small Text Editor with Emacs Keybindings
===============================================

kg is a minimal text editor inspired by Mg with Emacs keybindings and is
based on [kilo][0] and its descendants.

## Features

- Pure Emacs-style keybindings
- Syntax highlighting for many programming languages
- Multiple buffers with shared kill ring
- Incremental search
- Auto-indent
- Built-in help screen (C-h)
- No dependencies (not even curses)
- Uses standard VT100 escape sequences
- Graceful terminal resize handling

## Usage

```bash
kg <filename> [filename ...]
```

Multiple files can be opened at once, each in its own buffer.

## Keybindings

### File Operations

| Key       | Action                    |
|-----------|---------------------------|
| C-x C-s   | Save file                 |
| C-x C-c   | Quit editor               |

### Buffers

| Key       | Action                              |
|-----------|-------------------------------------|
| C-x C-f   | Open file in new buffer             |
| C-x b     | Cycle to next buffer                |
| C-x k     | Kill (close) current buffer         |
| C-x C-b   | List open buffers in status bar     |

The kill ring is shared across all buffers, so text killed in one buffer
can be yanked in another.

### Windows

| Key       | Action                              |
|-----------|-------------------------------------|
| C-x 2     | Split window horizontally           |
| C-x 3     | Split window vertically             |
| C-x o     | Switch to other window              |
| C-x 0     | Delete current window               |
| C-x 1     | Delete all other windows            |

Multiple windows can show different buffers (or the same buffer at
different positions). The kill ring is shared across all windows.

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
| Backspace | Delete character before cursor       |
| Delete    | Delete character at cursor           |
| Enter     | Insert newline (with auto-indent)    |

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

Undo history is per-buffer. Each buffer maintains up to 1000 operations.

### Search

| Key       | Action                    |
|-----------|---------------------------|
| C-s       | Incremental search        |
| C-r       | Incremental reverse search|

In search mode:

- Type to add to search query
- C-s or → or ↓ to find next occurrence
- C-r or ← or ↑ to find previous occurrence
- Enter to accept and stay at match
- ESC to cancel and return to original position
- Backspace to remove from query

### Utility

| Key       | Action                    |
|-----------|---------------------------|
| C-g       | Keyboard quit / Cancel    |
| C-l       | Refresh screen            |
| C-h       | Toggle help screen        |

### C-x Prefix Commands

After pressing C-x, the editor waits for a second key:

| Key       | Action                              |
|-----------|-------------------------------------|
| C-x C-s   | Save file                           |
| C-x C-c   | Quit                                |
| C-x C-f   | Open file in new buffer             |
| C-x b     | Cycle to next buffer                |
| C-x k     | Kill current buffer                 |
| C-x C-b   | List buffers                        |
| C-x 2     | Split window horizontally           |
| C-x 3     | Split window vertically             |
| C-x o     | Switch to other window              |
| C-x 0     | Delete current window               |
| C-x 1     | Delete all other windows            |
| C-x C-g   | Cancel the C-x prefix               |

## Language Support

kg provides syntax highlighting for 19 languages with automatic detection
based on file extension:

| Language   | Extensions                            | Features                             |
|------------|---------------------------------------|--------------------------------------|
| C/C++      | `.c`, `.h`, `.cpp`, `.hpp`, `.cc`     | Keywords, types, comments            |
| Python     | `.py`, `.pyw`, `.pyi`, `.pyx`         | Keywords, built-ins                  |
| Shell      | `.sh`, `.bash`, `.zsh`, profile files | Keywords, commands, variables        |
| JavaScript | `.js`, `.mjs`, `.cjs`                 | ES6+, async/await, built-ins         |
| TypeScript | `.ts`, `.tsx`, `.d.ts`                | JS + TS types, interfaces            |
| Rust       | `.rs`, `.rlib`                        | Keywords, std types, traits          |
| Java       | `.java`                               | Keywords, common classes             |
| C#         | `.cs`, `.csx`                         | Keywords, .NET types                 |
| PHP        | `.php`, `.phtml`                      | Keywords, superglobals               |
| Ruby       | `.rb`, `.rbw`, `.rake`                | Keywords, built-in methods           |
| Swift      | `.swift`                              | Keywords, Foundation types           |
| SQL        | `.sql`, `.ddl`, `.dml`                | SQL keywords, functions              |
| Dart       | `.dart`                               | Keywords, Flutter types              |
| HTML       | `.html`, `.htm`, `.xhtml`             | Tags, common attributes              |
| React/JSX  | `.jsx`                                | JS + React hooks and API             |
| Vue        | `.vue`                                | JS + Composition/Options API         |
| Angular    | `.component.ts`, `.service.ts`, …    | TS + decorators, directives          |
| Svelte     | `.svelte`                             | JS + Svelte lifecycle/stores         |
| Markdown   | `.md`, `.markdown`, `.mkd`            | Headings, code, bold, links          |

## Origin & References

kg is based on [kilo][0] by Salvatore Sanfilippo (antirez), the original
minimal text editor that demonstrates how to build a functional editor
without dependencies in about 1000 lines of C code.

The name "kg" is a nod to "mg" (Micro Emacs), suggesting "kilo-gram" - a
minimal implementation with Emacs keybindings.

[0]: https://github.com/antirez/kilo
