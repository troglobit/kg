# Makefile for kg editor
#
### Build Options
# Show a leading "~" on lines past the end of the buffer
KG_SHOW_TILDE ?= 1

CC      = gcc
CFLAGS  = -Wall -W -pedantic -std=c99 -Os
CFLAGS += -DKG_SHOW_TILDE=$(KG_SHOW_TILDE)
PROG    = kg
OBJDIR  = src
TARGET  = $(OBJDIR)/$(PROG)
MAN1    = doc/kg.1

prefix  = /usr/local
bindir  = $(prefix)/bin
mandir  = $(prefix)/share/man
man1dir = $(mandir)/man1

# Source files
SRCS = main.c tty.c syntax.c autocomplete.c buffer.c fileio.c display.c	\
       search.c basic.c word.c kbd.c yank.c undo.c help.c bufmgr.c	\
       winmgr.c cmd.c macro.c shell.c path.c rect.c

# Object and header files
OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
HDRS = $(OBJDIR)/def.h

# What clean (rm -f) and distclean (rm -rf) remove; test/include.mk appends
# its binaries, objects, and fuzzing artifacts.  Only DISTCLEANFILES may
# hold directories.
CLEANFILES     = $(OBJS)
DISTCLEANFILES = $(TARGET)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(OBJDIR)/%.o: $(OBJDIR)/%.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

# Test suite: unit checks, PTY acceptance tests, and fuzzing.
include test/include.mk

clean:
	rm -f $(CLEANFILES)

distclean: clean
	rm -rf $(DISTCLEANFILES)
	find . -name '*~' -o -name '*.orig' -o -name '*.rej' \
	       -o -name '*.bak' -o -name '*.swp' -o -name '.*.swp' | xargs rm -f
	rm -f core DEADJOE

deb:
	dpkg-buildpackage -b -us -uc

release:
	sh utils/mkrel.sh

install: $(TARGET)
	install -d $(DESTDIR)$(bindir)
	install -m 755 -s $(TARGET) $(DESTDIR)$(bindir)/$(PROG)
	install -d $(DESTDIR)$(man1dir)
	install -m 644 $(MAN1) $(DESTDIR)$(man1dir)/$(PROG).1

uninstall:
	rm -f $(DESTDIR)$(bindir)/$(PROG)
	rm -f $(DESTDIR)$(man1dir)/$(PROG).1

.PHONY: all clean distclean deb release install uninstall
