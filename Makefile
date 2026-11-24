# SPDX-License-Identifier: MIT

CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
CPPFLAGS ?=
LDFLAGS ?=

PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin
MANDIR  ?= $(PREFIX)/share/man

SRC := unizip.c detect.c extract.c util.c
OBJ := $(SRC:.c=.o)
BIN := uz
STATIC_BIN := uz.static

.PHONY: all clean install uninstall static test

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ)

test: $(BIN)
	./tests/run.sh

static: $(STATIC_BIN)

$(STATIC_BIN): $(OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -static -o $@ $(OBJ)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

unizip.o: unizip.c unizip.h detect.h extract.h util.h
detect.o: detect.c detect.h unizip.h util.h
extract.o: extract.c extract.h unizip.h util.h
util.o: util.c util.h

install: $(BIN)
	install -d $(DESTDIR)$(BINDIR)
	install -m755 $(BIN) $(DESTDIR)$(BINDIR)/$(BIN)
	install -d $(DESTDIR)$(MANDIR)/man1
	install -m644 uz.1 $(DESTDIR)$(MANDIR)/man1/uz.1

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(BIN)
	rm -f $(DESTDIR)$(MANDIR)/man1/uz.1

clean:
	rm -f $(OBJ) $(BIN) $(STATIC_BIN)
