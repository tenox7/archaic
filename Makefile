# Archaic for Unix with X11 and BSD sockets (on macOS: XQuartz in /opt/X11).
# make sdl: with SDL 2 instead of X11 (macOS, Linux ...): archaic-sdl.
# make cocoa: a Cocoa window on macOS, nothing else needed: archaic-cocoa.
# make headless: no window, for tests and automation: archaic-headless.
# make mac: Mac OS 6 and 7.5 on a 68k Mac, with Retro68 in
# Docker: src/build-mac/Archaic.dsk, a floppy image (see src/mkmac).
# The old systems have a build script of their own, in src. The Idris
# (Atari ST) build is src/mk, run inside Idris; the AT&T UNIX PC (3B1)
# build is src/mkunixpc. On AIX 3, AIX PS/2, HP-UX, DG/UX, Ultrix and
# SunOS, make runs the system's script (mkaix, mkaixps2, mkhpux, mkdgux,
# mkultrix, mksunview) and moves its archaic here: their compilers do not
# know the flags below. SunOS 3 has its uname in /usr/5bin: put it on the
# PATH. CC is the compiler, also for the scripts that take one (mkaix,
# mkaixps2, mkultrix): CC=gcc gmake.

X11 = /opt/X11
WARN = -Wall -Wextra -Wno-unused-parameter -Wno-deprecated-non-prototype -Wno-strict-prototypes -Wno-deprecated-declarations
CFLAGS = -std=c89 -pedantic $(WARN) -O2 -I$(X11)/include
LIBS = -L$(X11)/lib -lX11

SDLCONFIG = sdl2-config

CORE = src/main.c src/html.c src/http.c src/gif.c src/pic.c src/update.c
HDR = src/archaic.h
FB = src/fb.c src/fb.h src/font6x13.h

archaic: $(CORE) src/x11.c src/nsock.c $(HDR)
	@m=; case "`uname -s` `uname -r` `uname -m`" in \
	AIX*i386) m=mkaixps2 ;; \
	AIX*) m=mkaix ;; \
	HP-UX*) m=mkhpux ;; \
	dgux*) m=mkdgux ;; \
	ULTRIX*) m=mkultrix ;; \
	SunOS?[34]*) m=mksunview ;; \
	esac; \
	if [ -n "$$m" ]; then \
		echo "cd src && CC=$(CC) sh $$m && mv archaic .."; \
		cd src && CC="$(CC)" sh $$m && mv archaic ..; \
	else \
		echo "$(CC) $(CFLAGS) -o $@ $(CORE) src/x11.c src/nsock.c $(LIBS)"; \
		$(CC) $(CFLAGS) -o $@ $(CORE) src/x11.c src/nsock.c $(LIBS); \
	fi

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

mac:
	sh src/mkmac

# the same, with the address and undefined behaviour sanitizers
archaic-asan: $(CORE) src/x11.c src/nsock.c $(HDR)
	$(CC) $(CFLAGS) -g -O1 -fsanitize=address,undefined -o $@ $(CORE) src/x11.c src/nsock.c $(LIBS)

# K&R mode: no prototypes at all, as an old compiler would see it
knr: $(CORE) src/x11.c src/nsock.c $(HDR)
	$(CC) $(CFLAGS) -DNOPROTO -fsyntax-only $(CORE)

test: archaic-asan archaic-headless
	sh test/run.sh

clean:
	rm -rf archaic archaic-asan archaic-asan.dSYM archaic-sdl archaic-cocoa archaic-headless src/build-mac

.PHONY: sdl cocoa headless mac knr test clean
