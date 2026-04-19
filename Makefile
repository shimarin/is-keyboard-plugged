PREFIX  ?= /usr/local
DESTDIR ?=

CFLAGS  += -Wall -Wextra -O2

.PHONY: all install clean

all: is-keyboard-plugged

is-keyboard-plugged: is-keyboard-plugged.c

install: is-keyboard-plugged
	install -D -m 755 is-keyboard-plugged $(DESTDIR)$(PREFIX)/bin/is-keyboard-plugged

clean:
	rm -f is-keyboard-plugged
