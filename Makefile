# Archaic for Unix with X11 and BSD sockets (on macOS: XQuartz in /opt/X11).
# The Idris (Atari ST) build is src/mk, run inside Idris.
# The AT&T UNIX PC (3B1) build is src/mkunixpc, run on the 3B1.

CC = cc
X11 = /opt/X11
WARN = -Wall -Wextra -Wno-unused-parameter -Wno-deprecated-non-prototype -Wno-strict-prototypes -Wno-deprecated-declarations
CFLAGS = -std=c89 -pedantic $(WARN) -O2 -I$(X11)/include
LIBS = -L$(X11)/lib -lX11

CORE = src/main.c src/html.c src/http.c src/gif.c
HDR = src/archaic.h

archaic: $(CORE) src/x11.c src/nsock.c $(HDR)
	$(CC) $(CFLAGS) -o $@ $(CORE) src/x11.c src/nsock.c $(LIBS)

# the same, with the address and undefined behaviour sanitizers
archaic-asan: $(CORE) src/x11.c src/nsock.c $(HDR)
	$(CC) $(CFLAGS) -g -O1 -fsanitize=address,undefined -o $@ $(CORE) src/x11.c src/nsock.c $(LIBS)

# K&R mode: no prototypes at all, as an old compiler would see it
knr: $(CORE) src/x11.c src/nsock.c $(HDR)
	$(CC) $(CFLAGS) -DNOPROTO -fsyntax-only $(CORE)

test: archaic-asan
	sh test/run.sh

clean:
	rm -rf archaic archaic-asan archaic-asan.dSYM

.PHONY: knr test clean
