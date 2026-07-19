# Change Log

All relevant changes to the project are documented in this file.

## [UNRELEASED][]

### Changes

- kg now runs on the terminal's alternate screen, like other full-screen
  editors: the shell's content and scrollback come back intact on exit and
  suspend.  This is also what makes VTE-based terminals (gnome-terminal and
  friends) deliver shift-modified arrow keys to the editor instead of
  scrolling the scrollback.

- C-x 2 and C-x 3 now split only the current window, like in GNU Emacs: C-x 2
  into an upper and a lower half, C-x 3 into a left and a right half.  Other
  windows are not disturbed, so stacked and side-by-side layouts mix freely.
  C-x 0 gives the closed window's space to the neighbors sharing its edge.

- Shift-select by word and paragraph.  Ctrl+Shift+left/right extends the
  region one word at a time, Ctrl+Shift+up/down one paragraph, Shift+PgUp/PgDn
  one page, and Ctrl+Shift+Home/End to the buffer ends, complementing the
  character/line steps from v1.1.  The same transient-mark rules apply: the
  first unshifted key drops the region, and an explicit C-Space mark stays
  sticky.  Word motion (M-f, C-right, and the shifted variant) now lands at
  the end of the word, like in GNU Emacs and Mg, instead of at the start of
  the next one.

  As with the CUA clipboard keys in v1.1, some terminals grab these combos for
  themselves: Terminator resizes its panes with Ctrl+Shift+arrows and moves
  between them with Alt+arrows, KiTTY reserves Ctrl+Shift for its own
  shortcuts, and Gnome Terminal switches tabs on Ctrl+PgUp/PgDn.  Free the
  keys in the terminal preferences to use them in kg; see kg(1) for details.

- Move between windows with Meta and the arrow keys, like windmove in GNU
  Emacs: M-arrow selects the neighbor in that direction, quicker than lapping
  the C-x o ring in a three-window layout.  Also available as M-x
  windmove-up/-down/-left/-right.

- Resize windows with Meta, Shift and the arrow keys: the divider on that side
  of the window travels with the arrow, and C-u N moves it N rows or columns
  at once.  The GNU Emacs commands are wired up alongside: enlarge-window (C-x
  ^), enlarge-window-horizontally and shrink-window-horizontally (C-x } and
  C-x {), and balance-windows (C-x +).  A terminal resize goes to the focused
  window; the other windows keep their size and move.

- Ctrl+PgUp and Ctrl+PgDn move to the beginning and end of the buffer, like
  C-Home and C-End.

- F1 opens the built-in help (like C-h), F2 saves (C-x C-s), and F10 quits
  (C-x C-c), rounding out the function-key row alongside the F3/F4 macro keys.

### Fixes

- UTF-8 text no longer renders as inverted garbage in buffers with a language
  mode: the highlighter classified every multibyte character as non-printable.
  Most visible as broken box-drawing characters when reopening the built-in
  help (C-h C-h).

## [v1.1.0][] - 2026-05-26

> [!NOTE]
> Noteworthy additions since the last major release: *visual mark
> mode*, an ido-style picker for files/buffers/M-x, basic rectangle
> commands, C-u, and external shell commands.

### Changes

- Visual mark mode.  After C-Space the region between point and
  mark is now drawn in reverse video and tracks the cursor as it
  moves.  The highlight is per-buffer (it survives a C-x b switch
  and reappears when you return) and lines up across tabs.
  Following Emacs' transient-mark convention, the region
  deactivates on C-g, on the first edit, and after a region
  command (C-w, M-w) has consumed it; the mark itself stays set
  for the next C-x C-x.

- Shift-select and CUA clipboard shortcuts.  Shift+arrow,
  Shift+Home, and Shift+End drop the mark at point and extend
  the region as the cursor moves, so anyone coming from a
  modern GUI editor no longer has to remember C-Space first.
  The region is transient: a non-shifted key tears it down
  after the command runs, while region-consuming commands
  (C-w, M-w, C-x C-x) still see the mark intact during their
  dispatch.  Mixing styles works: an explicit C-Space mark stays
  sticky when extended with Shift+motion.

  The classic CUA clipboard trio is wired up alongside,
  mapping to the same kill ring as C-w / M-w / C-y:

      Shift+Delete   cut    (kill-region)
      Ctrl+Insert    copy   (kill-ring-save)
      Shift+Insert   paste  (yank)

  Note that some terminals (Terminator, gnome-terminal, ...)
  intercept Shift+Insert and Ctrl+Insert for their own
  clipboard handling; unbind them in the terminal preferences
  if you want kg to see them.

- Emacs ido-style picker for M-x, file, and buffer operations.
  In all three (M-x, C-x C-f, C-x b), type any fragment of the
  name to narrow the list (substring matching, with prefix matches
  ranked first).  Use backspace to go up a directory, arrow keys
  to select a file or descend a directory with enter.

- Rectangle mode and operations.  C-x SPC sets a rectangle
  mark at point and turns the highlight rectangular: every row
  in the row range gets reverse-video on the same column span,
  lined up across tabs.  The same transient-mark tear-down
  rules apply.

  C-x r prefixes a small set of column-wise operations:

      C-x r k    cut rectangle into the rect kill ring
      C-x r y    paste at point, padding short rows with
                 spaces and appending new rows when the
                 buffer is too short
      C-x r d    delete rectangle, no save
      C-x r c    clear rectangle, replacing chars with spaces
                 and padding short rows out

  Each op is one undo step; C-_ restores the entire affected
  range, including any padding or row appends.

  The Delete key also picks up region semantics: with an active
  mark it deletes the region (rectangle or linear) without
  saving, matching the muscle memory of every modern editor.
  Without a region it remains the usual forward-character
  delete.

- Run external shell commands and pipe regions through them.  M-!
  prompts for a command, runs it, and inserts its standard output at
  point.  M-| pipes the active region through the command and
  replaces the region with the output.  Standard error is discarded
  so it cannot disturb the editor display, and no terminal is shared
  with the child; these are for non-interactive filters (sort, fmt,
  jq, sed, ...).

- Tab completion in the minibuffer for the four filename prompts
  (C-x C-f, C-x C-r, C-x C-w, C-x i).  Matching directory entries
  appear live in the echo area as you type; Tab extends the typed
  text to the longest common prefix; a trailing '/' is appended on
  a sole directory match so the next Tab descends into it.
  Dotfiles stay hidden unless the typed prefix starts with '.'.

- Detect external changes to open files.  When something else
  rewrites a file you have open, kg now shows a "(changed)" tag in
  the mode line within a couple of seconds, no keystroke needed.
  C-x C-s against such a buffer prompts to confirm before
  overwriting.  Two new M-x commands turn the warning into
  Emacs-style auto-revert: auto-revert-mode for the current buffer,
  global-auto-revert-mode for all of them.  A clean buffer whose
  file has changed on disk is silently reloaded with cursor and
  viewport preserved; modified buffers are never auto-reverted.

- C-q quoted-insert.  Reads the next byte from the terminal
  verbatim and inserts it, bypassing auto-indent and bracket
  completion.  Useful for embedding a literal Tab, Escape, or
  other control byte when editing terminfo, sendmail.cf, or any
  file where a specific byte the regular key bindings would
  intercept matters.

- C-u universal-argument.  C-u alone runs the next command four
  times; stack more C-u presses to multiply by four each (C-u C-u
  for 16, C-u C-u C-u for 64); follow C-u with digits for an
  explicit count (C-u 80 - inserts eighty dashes).  Cursor motion,
  character and word deletion, line opening, kill-line, yank,
  undo, join-line, word-case, and plain character insertion all
  honour the count.  C-g cancels a half-typed prefix; the echo
  area shows what has been accumulated.  Kill-line and yank batch
  their N iterations under a single undo record so one C-_
  reverses the whole operation.

- More Emacs-style cursor motion: M-< / M-> jump to start/end of
  buffer; M-{ / M-} walk paragraphs (aliases for C-up / C-down);
  M-m goes to the first non-whitespace character on the current
  line; M-a / M-e walk by sentence; M-r cycles the cursor through
  top, middle, and bottom of the visible window without scrolling.

- The mode line now shows only the file's basename, Emacs-style,
  rather than truncating a long path at 30 characters.  The
  directory part is still available via C-x C-b.

### Bug Fixes

- Fix a crash when opening any Markdown file that contains an
  unmatched `**` marker.  The highlighter wrote one byte past the
  row's highlight buffer; opening `doc/TODO.md` itself was a
  reliable repro.

- Fix two memory-safety bugs surfaced by AddressSanitizer: an
  out-of-bounds read in the C keyword matcher on short lines, and
  a use-after-free in the undo handler for join-line where
  editor_insert_row's realloc could leave a stale row pointer.

- Pasting a block whose lines already have leading whitespace no
  longer produces a staircase: auto-indent now defers to the raw
  newline path when a paste is in progress.

## [v1.0.1][] - 2026-03-26

### Bug Fixes

- Fix `q` quitting the editor when typed in a lone `*scratch*` buffer;
  `q` now only closes special buffers when another buffer is available

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

[UNRELEASED]: https://github.com/troglobit/kg/compare/v1.1.0...HEAD
[v1.2.0]:     https://github.com/troglobit/kg/compare/v1.1.0...v1.2.0
[v1.1.0]:     https://github.com/troglobit/kg/compare/v1.0.0...v1.1.0
[v1.0.1]:     https://github.com/troglobit/kg/compare/v1.0.0...v1.0.1
[v1.0.0]:     https://github.com/troglobit/kg/releases/tag/v1.0.0
[kg.1]:       https://man.troglobit.com/man1/kg.1.html
[kilo]:       https://github.com/antirez/kilo
