# Makefile for kg editor

CC      = gcc
CFLAGS  = -Wall -W -pedantic -std=c99 -Os
PROG    = kg
OBJDIR  = src
TARGET  = $(OBJDIR)/$(PROG)
MAN1    = doc/kg.1

prefix  = /usr/local
bindir  = $(prefix)/bin
mandir  = $(prefix)/share/man
man1dir = $(mandir)/man1

# Source files
SRCS = main.c tty.c syntax.c autocomplete.c buffer.c fileio.c \
       display.c search.c basic.c word.c kbd.c yank.c undo.c help.c bufmgr.c winmgr.c cmd.c macro.c \
       shell.c path.c rect.c

# Object and header files
OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
HDRS = $(OBJDIR)/def.h

# Test infrastructure
TESTDIR  = test
TESTBINS = $(TESTDIR)/test_undo $(TESTDIR)/test_buffer \
           $(TESTDIR)/test_syntax $(TESTDIR)/test_yank \
           $(TESTDIR)/test_autocomplete $(TESTDIR)/test_word \
           $(TESTDIR)/test_basic $(TESTDIR)/test_region \
           $(TESTDIR)/test_shell $(TESTDIR)/test_complete
# Source objects needed by tests (subset of OBJS, no main/tty/display/etc.)
TEST_SRCS_OBJS = $(OBJDIR)/undo.o $(OBJDIR)/buffer.o $(OBJDIR)/syntax.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(OBJDIR)/%.o: $(OBJDIR)/%.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

check: $(TESTBINS)
	@pass=0; fail=0; \
	for t in $(TESTBINS); do \
		name=$$(basename $$t); \
		log=$$(mktemp); \
		if $$t >$$log 2>&1; then \
			echo "PASS: $$name"; pass=$$((pass+1)); \
		else \
			echo "FAIL: $$name"; fail=$$((fail+1)); \
			cat $$log; \
		fi; \
		rm -f $$log; \
	done; \
	total=$$((pass+fail)); \
	echo ""; \
	echo "============================================================================"; \
	echo "Testsuite summary for kg"; \
	echo "============================================================================"; \
	printf "# TOTAL: %d\n# PASS:  %d\n# SKIP:  0\n# XFAIL: 0\n# FAIL:  %d\n# XPASS: 0\n# ERROR: 0\n" \
	       $$total $$pass $$fail; \
	echo "============================================================================"; \
	[ $$fail -eq 0 ]

# Per-test linker prerequisites beyond the common test_%.o + test.o.
# The static pattern rule below pulls these in via secondary expansion.
EXTRA_undo         := $(TESTDIR)/stubs.o          $(TEST_SRCS_OBJS)
EXTRA_buffer       := $(TESTDIR)/stubs.o          $(TEST_SRCS_OBJS)
EXTRA_syntax       := $(TESTDIR)/stubs.o          $(TEST_SRCS_OBJS)
EXTRA_yank         := $(TESTDIR)/stubs_noyank.o   $(OBJDIR)/yank.o $(OBJDIR)/rect.o $(TEST_SRCS_OBJS)
EXTRA_autocomplete := $(TESTDIR)/stubs.o $(TESTDIR)/stubs_extra.o $(OBJDIR)/autocomplete.o $(TEST_SRCS_OBJS)
EXTRA_word         := $(TESTDIR)/stubs.o $(TESTDIR)/stubs_extra.o $(OBJDIR)/word.o $(TEST_SRCS_OBJS)
EXTRA_basic        := $(TESTDIR)/stubs.o          $(OBJDIR)/basic.o $(TEST_SRCS_OBJS)
EXTRA_region       := $(TESTDIR)/stubs_noyank.o   $(OBJDIR)/yank.o $(OBJDIR)/rect.o $(TEST_SRCS_OBJS)
EXTRA_shell        := $(TESTDIR)/stubs_noyank.o   $(OBJDIR)/shell.o $(OBJDIR)/yank.o $(OBJDIR)/rect.o $(OBJDIR)/buffer.o $(OBJDIR)/undo.o $(OBJDIR)/syntax.o
EXTRA_complete     := $(TESTDIR)/stubs.o          $(OBJDIR)/path.o $(TEST_SRCS_OBJS)

.SECONDEXPANSION:
$(TESTBINS): $(TESTDIR)/test_%: $(TESTDIR)/test_%.o $(TESTDIR)/test.o $$(EXTRA_$$*)
	$(CC) $(CFLAGS) -o $@ $^

$(TESTDIR)/%.o: $(TESTDIR)/%.c $(HDRS)
	$(CC) $(CFLAGS) -I$(OBJDIR) -c $< -o $@

clean:
	rm -f $(OBJS) $(TESTDIR)/*.o

distclean: clean
	rm -f $(TARGET) $(TESTBINS)
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

.PHONY: all clean distclean check deb release install uninstall
