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
       display.c search.c basic.c word.c kbd.c yank.c undo.c help.c bufmgr.c winmgr.c cmd.c macro.c

# Object and header files
OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
HDRS = $(OBJDIR)/def.h

# Test infrastructure
TESTDIR  = test
TESTBINS = $(TESTDIR)/test_undo $(TESTDIR)/test_buffer \
           $(TESTDIR)/test_syntax $(TESTDIR)/test_yank \
           $(TESTDIR)/test_autocomplete $(TESTDIR)/test_word \
           $(TESTDIR)/test_basic $(TESTDIR)/test_region
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

$(TESTDIR)/test_undo: $(TESTDIR)/test_undo.o $(TESTDIR)/test.o $(TESTDIR)/stubs.o $(TEST_SRCS_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(TESTDIR)/test_buffer: $(TESTDIR)/test_buffer.o $(TESTDIR)/test.o $(TESTDIR)/stubs.o $(TEST_SRCS_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(TESTDIR)/test_syntax: $(TESTDIR)/test_syntax.o $(TESTDIR)/test.o $(TESTDIR)/stubs.o $(TEST_SRCS_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(TESTDIR)/test_yank: $(TESTDIR)/test_yank.o $(TESTDIR)/test.o $(TESTDIR)/stubs_noyank.o \
        $(OBJDIR)/yank.o $(TEST_SRCS_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(TESTDIR)/test_autocomplete: $(TESTDIR)/test_autocomplete.o $(TESTDIR)/test.o \
        $(TESTDIR)/stubs.o $(TESTDIR)/stubs_extra.o \
        $(OBJDIR)/autocomplete.o $(TEST_SRCS_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(TESTDIR)/test_word: $(TESTDIR)/test_word.o $(TESTDIR)/test.o \
        $(TESTDIR)/stubs.o $(TESTDIR)/stubs_extra.o \
        $(OBJDIR)/word.o $(TEST_SRCS_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(TESTDIR)/test_basic: $(TESTDIR)/test_basic.o $(TESTDIR)/test.o \
        $(TESTDIR)/stubs.o $(OBJDIR)/basic.o $(TEST_SRCS_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(TESTDIR)/test_region: $(TESTDIR)/test_region.o $(TESTDIR)/test.o \
        $(TESTDIR)/stubs_noyank.o $(OBJDIR)/yank.o $(TEST_SRCS_OBJS)
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
	install -m 755 $(TARGET) $(DESTDIR)$(bindir)/$(PROG)
	install -d $(DESTDIR)$(man1dir)
	install -m 644 $(MAN1) $(DESTDIR)$(man1dir)/$(PROG).1

uninstall:
	rm -f $(DESTDIR)$(bindir)/$(PROG)
	rm -f $(DESTDIR)$(man1dir)/$(PROG).1

.PHONY: all clean distclean check deb release install uninstall
