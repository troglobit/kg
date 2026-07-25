# Change Log

All relevant changes to the project are documented in this file.

## [v1.2.0][] - 2026-07-25

### Changes

- Point-editing commands from GNU Emacs: C-t transpose-chars, M-h
  mark-paragraph, M-z zap-to-char, M-\ delete-horizontal-space, M-SPC
  just-one-space, and M-digit as a numeric prefix alongside C-u.  Each
  editing command is also available as an M-x alias.  From Björn
  Dahlgren's fork.

- Per-buffer fill column, set with C-x f (set-fill-column).  M-q reflow was
  fixed at 72 columns; the width is now a per-buffer setting, defaulting to
  72 and seeded with the current value when C-x f prompts, and it rides
  along with the buffer across switches like auto-revert does.

- The minibuffer answer can be edited with the usual Emacs keys -- C-a/C-e,
  C-b/C-f, C-d, C-k, Home/End and the arrows -- and C-q inserts the next
  keystroke literally.  From Björn Dahlgren's fork.

- The M-x, buffer, and file pickers highlight the selected candidate in
  reverse video instead of braces and bold, and a candidate too long for
  the line keeps its start and end visible with an elided middle -- so
  arrowing through long, similar names no longer looks stuck.

- Killing the last buffer with C-x k drops to an empty *scratch* buffer
  instead of quitting kg, like GNU Emacs.

- Quitting with C-x C-c offers to save each modified buffer first, like
  GNU Emacs, and asks to confirm the exit only if any are left unsaved --
  rather than discarding every unsaved change at once.

- M-x sort-lines sorts the lines the region covers into byte order -- like
  sort(1) in the C locale, so case-sensitive and not numeric -- as a single
  undo step.  From Björn Dahlgren's fork.

- M-x read-only-mode toggles a buffer read-only, alongside C-x C-q and
  M-x toggle-read-only.  A read-only buffer now refuses every
  buffer-modifying command, by keystroke or by M-x alike, enforced at the
  point of mutation so editing keys like M-u, M-^, and C-o can no longer
  slip through.  The mode line marks a read-only buffer with `%%`, or `%*`
  when it has unsaved changes, like GNU Emacs.  The read-only-mode command
  is from Björn Dahlgren's fork.

- Opening a file without write permission now enters read-only mode,
  like GNU Emacs, so a file you cannot save back is flagged up front
  instead of only at save time.

- Saving is atomic.  The new contents go to a temporary file beside the
  target, which is fsync'd and renamed over it, so an interrupted or failed
  write can no longer truncate the file -- a reader sees either the old file
  or the complete new one.  The file's mode and owner are preserved, and a
  symlinked target is replaced in place.  From Björn Dahlgren's fork.

- Saving keeps the previous version as a `foo~` backup, like GNU Emacs.
  The first save of a session renames the file being replaced to `foo~`
  before writing the new contents, so a fat-fingered edit or an accidental
  deletion is one `mv foo~ foo` away.  Turn it off with M-x
  make-backup-files.

- M-x require-final-newline makes saving add a trailing newline to a file
  that lacks one, giving the buffer the empty last line like GNU Emacs.
  Off by default, so a file without a final newline still round-trips
  untouched unless you ask for one.

- Incremental search behaves more like GNU Emacs.  The view holds still
  while the match is on screen and only recentres once it scrolls off,
  instead of yanking every hit to the top of the window.  A motion key
  pressed mid-search (C-a, C-f, M-f, M-<, ...) ends the search and runs,
  leaving point on the match — at its end searching forward, its start
  searching back — and C-Space ends the search with the mark set there.
  C-r now searches backward from point rather than always scanning
  forward from the top.  From Björn Dahlgren's fork.

- Incremental search and query-replace fold case for an all-lowercase
  query and match exactly once the query contains an uppercase letter,
  like GNU Emacs.  From Björn Dahlgren's fork.

- Files round-trip byte for byte.  A file without a final newline keeps
  it that way through a save, and a file ending in a newline shows a
  trailing empty line, like GNU Emacs; kg no longer forces a trailing
  newline onto a file that had none.  From Björn Dahlgren's fork.

- Lines past the end of the buffer traditionally carrying a leading "~", this
  can now be disabled at build-time, to make kg resemble GNU Emacs.  Build
  with `make KG_SHOW_TILDE=0` to disable.  From Björn Dahlgren's fork.

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
  the next one, and crosses line boundaries the way backward-word already did.

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

- Home and End are now recognised from VT220 and rxvt terminals, which send
  the numbered forms (ESC[1~/ESC[4~ and ESC[7~/ESC[8~) rather than the
  usual sequences.  From Björn Dahlgren's fork.

### Fixes

- UTF-8 text no longer renders as inverted garbage in buffers with a language
  mode: the highlighter classified every multibyte character as non-printable.
  Most visible as broken box-drawing characters when reopening the built-in
  help (C-h C-h).

- M-; (comment-dwim) on a region no longer comments its last line when the
  region ends at that line's column 0, matching Emacs' comment-region: a
  region from the start of line N to the start of line N+2 now comments only
  N and N+1.  From Björn Dahlgren's fork.

- Undo of a line join restores an empty line again.  Backspacing at the
  start of a blank line and then undoing used to drop the blank line, because
  the undo record carried no text for the zero-length line.

- A batch of crash and memory-safety fixes surfaced by the new keypress fuzz
  harness: out-of-bounds cursor and edit handling when point sat past a short
  line or the buffer shrank underneath it, a SIGWINCH handler that ran unsafe
  work (malloc, write) mid-signal and could corrupt the heap during a resize,
  and a memset() on the NULL highlight buffer of an empty row.

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

[UNRELEASED]: https://github.com/troglobit/kg/compare/v1.2.0...HEAD
[v1.2.0]:     https://github.com/troglobit/kg/compare/v1.1.0...v1.2.0
[v1.1.0]:     https://github.com/troglobit/kg/compare/v1.0.0...v1.1.0
[v1.0.1]:     https://github.com/troglobit/kg/compare/v1.0.0...v1.0.1
[v1.0.0]:     https://github.com/troglobit/kg/releases/tag/v1.0.0
[kg.1]:       https://man.troglobit.com/man1/kg.1.html
[kilo]:       https://github.com/antirez/kilo
