[![2][]][1] [![4][]][3] [![6][]][5] <a href="https://www.flaticon.com/free-icons/kg" title="Kg icons created by alekseyvanin - Flaticon"><img src="doc/kg.png" width=100 align="right"></a>

# Light Weight UTF-8 Terminal Text Editor

kg is a minimal UTF-8 text editor inspired by Mg with Emacs keybindings,
based on [kilo][0] and its descendants.  Suitable for editing system
files or doing simple fixes, on remote systems where a full blown GUI
editor does not work.

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

```
kg [-RVh] [file ...]
```

| Option | Description                  |
|--------|------------------------------|
| `-R`   | Open file(s) read-only       |
| `-V`   | Print version and exit       |
| `-h`   | Print this help and exit     |

Multiple files can be opened at once, each in its own buffer.
A man page is available online at [man.troglobit.com](https://man.troglobit.com/man1/kg.1.html).

## Keybindings

### File Operations

| Key       | Action                              |
|-----------|-------------------------------------|
| C-x C-s   | Save current buffer                 |
| C-x s     | Save all modified buffers           |
| C-x C-c   | Quit editor                         |

### Buffers

| Key       | Action                              |
|-----------|-------------------------------------|
| C-x C-f   | Open file in new buffer             |
| C-x C-r   | Open file read-only                 |
| C-x C-q   | Toggle read-only mode               |
| C-x b     | Switch to buffer (interactive)      |
| C-x k     | Kill (close) current buffer         |
| C-x C-b   | Open buffer list                    |

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

### Goto Line

| Key       | Action                    |
|-----------|---------------------------|
| M-g       | Goto line (prompts for `line` or `line:col`) |

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
| C-x s     | Save all modified buffers           |
| C-x C-r   | Open file read-only                 |
| C-x C-q   | Toggle read-only mode               |
| C-x C-b   | List buffers                        |
| C-x 2     | Split window horizontally           |
| C-x 3     | Split window vertically             |
| C-x o     | Switch to other window              |
| C-x 0     | Delete current window               |
| C-x 1     | Delete all other windows            |
| C-x C-g   | Cancel the C-x prefix               |
| M-g       | Goto line                           |

## Building and Installing

```bash
make
sudo make install          # installs to /usr/local/bin and /usr/local/share/man/man1
```

Override the prefix or use DESTDIR for staged installs:

```bash
make install prefix=/usr
make install DESTDIR=/tmp/pkg
```

To uninstall:

```bash
sudo make uninstall
```

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
| Makefile   | `Makefile`, `.mk`, `.mak`             | Directives, special targets          |
| Markdown   | `.md`, `.markdown`, `.mkd`            | Headings, code, bold, links          |

## Origin & References

kg is based on [kilo][0] by Salvatore Sanfilippo (antirez), the original
minimal text editor that demonstrates how to build a functional editor
without dependencies in about 1000 lines of C code.

The name "kg" is a nod to "mg" (Micro Emacs), suggesting "kilo-gram" - a
minimal implementation with Emacs keybindings.

[0]: https://github.com/antirez/kilo
[1]: https://en.wikipedia.org/wiki/BSD_licenses
[2]: https://img.shields.io/badge/License-BSD%202--Clause-green.svg
[3]: https://github.com/troglobit/kg/actions/workflows/build.yml/
[4]: https://github.com/troglobit/kg/actions/workflows/build.yml/badge.svg
[5]: https://github.com/troglobit/kg/releases
[6]: https://img.shields.io/github/v/release/troglobit/kg
