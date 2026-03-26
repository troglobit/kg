# Change Log

All relevant changes to the project are documented in this file.

## [v1.0.0][] - 2026-03-26

First release of kg, a small terminal text editor with Emacs key
bindings, based on [kilo][] by Salvatore Sanfilippo.

### Changes

- Emacs-style key bindings for navigation, editing, search, and file
  operations; see [kg.1][] for the full reference
- Multiple buffers (C-x b, C-x C-b) with a shared kill ring
- Split-window support (C-x 2 / C-x 3, cycle with C-x o)
- Syntax highlighting for C/C++, Python, Shell, Ruby, Lua, JavaScript,
  TypeScript, React, Vue, Angular, Svelte, HTML, Markdown, and Makefile;
  language detected by file extension and `#!` shebang line; includes
  hexadecimal (0x), binary (0b), and octal (0o) integer literals
- Multi-level undo (C-_)
- Incremental search (C-s / C-r) and query-replace (M-%)
- Paragraph reflow to 72 columns (M-q)
- Auto-indent and bracket/quote autocompletion
- Suspend to shell (C-z)
- C-l recenters the view; repeated presses cycle top/center/bottom;
  also probes terminal size, useful on serial console connections
- M-x command dispatcher with interactive Tab completion
- Keyboard macros: record with C-x ( or F3, stop with C-x ) or F4,
  replay with C-x e or F4
- Comment-dwim (M-;), join-line (M-^), word-case (M-u / M-l / M-c)
- Exchange-point-and-mark (C-x C-x) and open-line (C-o)
- Write-file (C-x C-w), insert-file (C-x i), revert-buffer (M-x revert-buffer)
- Built-in key binding reference (C-h)
- Man page ([kg.1][]) and `make install` / `make uninstall` support

[UNRELEASED]: https://github.com/troglobit/kg/compare/1.0.0...HEAD
[kg.1]:       https://man.troglobit.com/man1/kg.1.html
[kilo]:       https://github.com/antirez/kilo
