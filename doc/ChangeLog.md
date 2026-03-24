# Change Log

All relevant changes to the project are documented in this file.

## [v1.0.0][] - 2026-03-xx

First release of kg, a small terminal text editor with Emacs key
bindings, based on [kilo][] by Salvatore Sanfilippo.

### Changes

- Emacs-style key bindings for navigation, editing, search, and file
  operations; familiar to users of Emacs, Mg, or Nano
- Multiple buffers: open several files at once, switch with C-x b,
  list with C-x C-b; kill ring is shared across all buffers so text
  can be copied between them
- Split-window support: divide the screen horizontally (C-x 2) or
  vertically (C-x 3), cycle windows with C-x o
- Syntax highlighting for C/C++, Python, Shell, Ruby, Lua, JavaScript,
  TypeScript, React, Vue, Angular, Svelte, HTML, Markdown, and Makefile;
  language detected by file extension and `#!` shebang line
- Multi-level undo (C-_) with intelligent grouping of related edits
- Incremental search forward and backward (C-s / C-r)
- Paragraph reflow to 72 columns with M-q, preserving indentation and
  recorded as a single undo step
- Auto-indent on Enter and bracket/quote autocompletion
- Suspend to shell with C-z; terminal is fully restored on `fg`
- C-l recenters the view so the current line lands in the middle of the
  window; pressing C-l again cycles to the top, then the bottom, then
  back to center — also probes the terminal size, useful over serial
  console connections where resize events are not delivered
- Built-in key binding reference toggled with C-h
- Man page ([kg.1][]) and `make install` / `make uninstall` support

[UNRELEASED]: https://github.com/troglobit/kg/compare/1.0.0...HEAD
[kg.1]:       https://man.troglobit.com/man1/kg.1.html
[kilo]:       https://github.com/antirez/kilo
