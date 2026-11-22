# Archaic for Unix with X11 and BSD sockets (on macOS: XQuartz in /opt/X11).
# make sdl: with SDL 2 instead of X11 (macOS, Linux ...): archaic-sdl.
# make cocoa: a Cocoa window on macOS, nothing else needed: archaic-cocoa.
# make headless: no window, for tests and automation: archaic-headless.
# The Idris (Atari ST) build is src/mk, run inside Idris.
# The AT&T UNIX PC (3B1) build is src/mkunixpc, run on the 3B1.
# The SunOS 3 and 4 build (SunView) is src/mksunview, run on SunOS.
# The HP-UX 9 build (X11, PA-RISC) is src/mkhpux, run on HP-UX.

CC = cc
X11 = /opt/X11
WARN = -Wall -Wextra -Wno-unused-parameter -Wno-deprecated-non-prototype -Wno-strict-prototypes -Wno-deprecated-declarations
CFLAGS = -std=c89 -pedantic $(WARN) -O2 -I$(X11)/include
LIBS = -L$(X11)/lib -lX11

SDLCONFIG = sdl2-config

CORE = src/main.c src/html.c src/http.c src/gif.c src/pic.c
HDR = src/archaic.h
FB = src/fb.c src/fb.h src/font6x13.h

archaic: $(CORE) src/x11.c src/nsock.c $(HDR)
	$(CC) $(CFLAGS) -o $@ $(CORE) src/x11.c src/nsock.c $(LIBS)

# SDL 2 and Cocoa draw with fb.c, a screen in memory
archaic-sdl: $(CORE) $(FB) src/sdl.c src/nsock.c $(HDR)
	$(CC) -std=c99 $(WARN) -O2 `$(SDLCONFIG) --cflags` -c -o sdl.o src/sdl.c
	$(CC) $(CFLAGS) -o $@ $(CORE) src/fb.c src/nsock.c sdl.o `$(SDLCONFIG) --libs`
	rm -f sdl.o

archaic-cocoa: $(CORE) $(FB) src/cocoa.m src/nsock.c $(HDR)
	$(CC) $(WARN) -O2 -c -o cocoa.o src/cocoa.m
	$(CC) $(CFLAGS) -o $@ $(CORE) src/fb.c src/nsock.c cocoa.o \
		-framework Cocoa -framework QuartzCore
	rm -f cocoa.o

# no window: commands on standard input, screenshots (see src/headless.c)
archaic-headless: $(CORE) $(FB) src/headless.c src/nsock.c $(HDR)
	$(CC) $(CFLAGS) -o $@ $(CORE) src/fb.c src/headless.c src/nsock.c

sdl: archaic-sdl
cocoa: archaic-cocoa
headless: archaic-headless

# the same, with the address and undefined behaviour sanitizers
archaic-asan: $(CORE) src/x11.c src/nsock.c $(HDR)
	$(CC) $(CFLAGS) -g -O1 -fsanitize=address,undefined -o $@ $(CORE) src/x11.c src/nsock.c $(LIBS)

# K&R mode: no prototypes at all, as an old compiler would see it
knr: $(CORE) src/x11.c src/nsock.c $(HDR)
	$(CC) $(CFLAGS) -DNOPROTO -fsyntax-only $(CORE)

test: archaic-asan archaic-headless
	sh test/run.sh

clean:
	rm -rf archaic archaic-asan archaic-asan.dSYM archaic-sdl archaic-cocoa archaic-headless

.PHONY: sdl cocoa headless knr test clean
