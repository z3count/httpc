PROGNAME=httpc

CC?=gcc

DESTDIR=/usr/local
OBJDIR=objs
SRCDIR=src
INCDIR=include
BINDIR=bin
MANDIR=man
TESTDIR=tests

MANFILE=$(MANDIR)/${PROGNAME}.1
PROGFILE=$(BINDIR)/$(PROGNAME)

DESTMANDIR=$(DESTDIR)/man/man1
DESTBINDIR=$(DESTDIR)/bin

SRC=$(wildcard $(SRCDIR)/*.c)
OBJS=$(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(notdir $(SRC))))

COMMON_CFLAGS=-D_GNU_SOURCE -I$(INCDIR) -I/usr/local/include -Wall -Wextra -Werror -std=c99
COMMON_LDFLAGS=

CFLAGS=-g -ggdb -O0 $(COMMON_CFLAGS)
LDFLAGS=$(COMMON_LDFLAGS) -lssl -lcrypto


compile: $(PROGNAME)

all: compile test

test: $(PROGNAME)
	$(BINDIR)/$(PROGNAME) -t

$(PROGNAME): $(OBJS)
	$(CC) -o $(BINDIR)/$(PROGNAME) $(CFLAGS) $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/%.h
	$(CC) $(CFLAGS) -o $@ -c $<

install:
	install -m755 $(BINDIR)/$(PROGNAME) $(DESTBINDIR)
	install -m644 $(MANFILE) $(DESTMANDIR)

uninstall:
	rm -f $(DESTBINDIR)/$(PROGNAME)
	rm -f $(DESTMANDIR)/$(MANFILE)

clean:
	rm -f $(OBJDIR)/*.o $(BINDIR)/$(PROGNAME)
	find . -name \*~ -delete
