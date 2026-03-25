# TODO In Priority Order

## Missing Mg features

Features and keybindings present in Mg but missing from kg, roughly
ordered by value vs implementation effort.

### High value, straightforward

- [x] **M-% query-replace**: Interactive find-and-replace.  Prompt for
      search string then replacement, step through matches with y/n/!
      (replace all)/q.  The incremental search machinery in search.c is
      a natural starting point.

- [ ] **C-x C-w write-file**: Save buffer to a different filename (Save
      As).  Prompts for a new name, writes, and updates the buffer's
      filename.

- [ ] **C-x i insert-file**: Insert the contents of a file at point.

- [ ] **M-; comment-dwim**: Toggle or insert a line comment using the
      current buffer's syntax.  Requires plumbing comment-start strings
      from the syntax table through to an editing command.

- [ ] **C-x C-x exchange-point-and-mark**: Swap cursor and mark.  Lets
      you visually inspect the other end of a region or bounce between two
      positions.  Trivial to implement.

- [ ] **C-o open-line**: Insert a newline at point without advancing the
      cursor.  The classic "make room above the next line" command.  One
      liner.

### Medium value

- [ ] **Keyboard macros  C-x ( / C-x ) / C-x e**: Record and replay a
      sequence of keystrokes.  Even a single-level macro (no nesting)
      covers the vast majority of real use.

- [ ] **C-u universal-argument**: Numeric prefix for repeating commands
      (e.g. C-u 8 C-f moves forward 8 chars).  Moderate plumbing work
      but unlocks power-user workflows.

- [ ] **C-t transpose-chars**: Swap the character before point with the
      one at point.  Indispensable for fixing the most common typing errors.

- [ ] **M-u / M-l / M-c  upcase / downcase / capitalize word**: Operate
      on the word from point forward.  word.c already walks words; just
      add tolower/toupper passes.

- [ ] **M-y yank-pop**: After C-y, cycle backwards through the kill ring
      with repeated M-y presses.  Requires expanding the kill ring from a
      single slot to a small ring (Emacs default is 60; even 8 would cover
      most use).

### Lower priority / larger scope

- [ ] **M-z zap-to-char**: Kill from point up to and including a
      prompted character.

- [ ] **M-x named commands**: Execute a command by name.  Opens the door
      to toggles (auto-fill, overwrite-mode) without burning key bindings.
      Significant infrastructure but makes kg extensible.

- [ ] **Multi-entry kill ring**: Prerequisite for M-y yank-pop.  Keep
      the ring bounded (8–16 entries) to avoid unbounded memory growth.

- [ ] **M-^ join-line**: Join current line with the previous one
      (complement to the C-k-at-EOL join-with-next that kg already has).

- [ ] **M-\\ delete-horizontal-space** and **M-SPC just-one-space**:
      Delete all whitespace around point, or collapse it to a single
      space.

## Important (DONE)

- [x] Refactor code to Linux style, variable decl. at top of context sorted
      in reverse chrismas tree style, lines can be up to 110 chars long
- [x] Add hash-bang fallback for detection of syntax highlighting, e.g., #!/bin/sh
- [x] Fix delete key
- [x] Add auto-indent à la Mg (consider M-x auto-indent-mode toggle or
      C-j vs Enter distinction)
- [x] Add built-in help, similar in style to whay my fork of Mg has, see
      ~/src/mg/ for details
- [x] Add markdown-mode with syntax highlighting
- [x] Add support for multiple buffers, supporting copy-paste between
      them, i.e., shared kill ring
- [x] Add support for split windows, both horizontal and vertical
- [x] Change the mode line to be more similar to Emacs, the current
      active window marker '**' is so easily mistaken for "aha a modifed
      buffer", we could instead use ansi escape sequences to set all the
      non-selected windows as "dim"
- [x] With two buffers open, we should only need to ask "are you sure"
      if any buffer is modified and not saved when exiting
- [x] Testing and stability to reach "usable" level
- [x] Add support for Emacs' M-q to "reflow" a paragraph

## Maybe Later

- [ ] Send alt screen sequences if TERM=xterm: "\033[?1049h" and "\033[?1049l"
- [ ] Add support for modes.  E.g., c-mode with bindings for
      compile/make which opens a compile buffer in a window below
