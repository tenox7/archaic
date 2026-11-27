# Archaic on Idris (Atari ST, X10)

The parts that all systems share (Colors, Pictures, Updates, Porting ...) are in [AGENTS.md](../AGENTS.md).

## Running on Idris

- Kit: `atarist-idris314-hatari` (Hatari), on osarchive in `os/idris/virt/`.
  Newer kits have Archaic in `/usr/X/bin`.
- Release binary: `archaic-idris`. Copy it into Idris's `/usr/X/bin` as `archaic` (kermit works),
  then `chmod /usr/X/bin/archaic`. With no mode, Idris's chmod makes a file executable.
- Log in as `root`, start X: `/usr/X/bin/xstart`. In an xterm, type `archaic`.
  It opens a little smaller than the screen, in the middle (576 x 360), so the desktop shows around it.
  xstart puts `/usr/X/bin` on the search path. From a telnet login, type `/usr/X/bin/archaic`.
- **Ctrl-Q** quits.
- In the kit, your computer is `10.0.2.2` for Idris. So Archaic on Idris looks for WRP at `http://10.0.2.2:8080/`.
- `screenshots/idris.png`: the ST's own screen, 640 x 400, black and white:
  `uname -a` in the lower xterm, and Archaic under it (`=638x290+0+108`) with
  Wikipedia's Idris article.

## Building in Idris

### 1. Move the files in

A tiny HTTP client, **hget**, brings files from your computer through netd.
You type hget in only once.

Serve this folder from your computer (leave it running):

```
python3 tools/idris.py 0 serve . 8077
```

Once: type hget's source into Idris over telnet, then build it.
(2323 is the kit's telnet port.)

```
python3 tools/idris.py 2323 put /src/archaic tools/hget.c
# then in Idris:
cd /src/archaic
cp /src/net/netc.h .
c hget.c
sh /src/net/lk hget
cp hget /usr/bin/hget
```

Then, in Idris, in an empty folder:

```
hget 10.0.2.2 8077 /src/fetch fetch
sh fetch
```

All 13 files (197 KB) take about 2 minutes.

### 2. Build

```
sh mkinc     # once per folder: the X10 headers
sh mk        # compile and link: archaic, about 160 KB
```

With Hatari's fast-forward on, `sh mk` takes about 4 minutes.
To relink only: `sh lka`.

> **Fast-forward**: turn it on only while waiting (compiles, X start).
> Never type with it on: every key repeats.

### 3. Test

**Test mode, no X needed** (with `tools/idris.py ... serve` running):

```
./archaic -t http://10.0.2.2:8077/test/data/hn2.gif
```

The last line ends with `sum ae2b58f3`: the same as on Unix.
That checks HTTP, netd and the GIF decoder in one go.
With `hn2.abm` or `hn2.ab3` in the place of `hn2.gif`, it checks the ABM and ABM3 decoders: the same sum.

**With X:** start X, then run `./archaic` in an xterm (or from a telnet login).

**Scripts** (`-s`) act as a user, then quit:

| In the script | Means |
|---|---|
| a letter | that key |
| `~` | Return |
| `@x,y;` | left click at x, y in the window |
| `%` | wait 2 seconds |

Example: load a page, then keep the window 40 s for a screenshot:

```
./archaic -s '@50,10;http://en.wikipedia.org/~%%%%%%%%%%%%%%%%%%%%'
```

**Event trace:** `setenv ARCHAICDBG 1` prints every X event to stderr.

**Measured on the emulated Mega ST:**

| What | Time |
|---|---|
| WRP start page | 3 s |
| Wikipedia page with its picture (8 KB as ABM) | 10 s from Return (Archaic 0.3: 30 s) |
| decode check (`-t`, `hn2.abm`, 9 KB) | 9 s |
| decode check (`-t`, `hn2.gif`, 11 KB: a GIF with 256 colors in its table, so the slow way) | 27 s |
| update with `-u` (a program of 132 KB, about 1.9 KB/s) | 1 min 24 s |
| the same update in the window, after WRP's first page (about 1.25 KB/s) | 2 min 10 s |

Before an update's download, Archaic reads its own file for its CRC (8 s on the ST):
if another Archaic has installed the new one already, there is nothing to get.

## Idris on the Atari ST: what to know

Test on a copy of your Idris disk.

### The system

| Thing | Idris |
|---|---|
| CPU, memory | 68000 at 8 MHz, 4 MB, no MMU |
| Screen | 640 x 400, black and white |
| Window system | **X10** (not X11): `Xlib.o`, `XText`, `XPixSet`, `XBitmapBitsPut` |
| Network | netd: SLIP on the serial port. AF_UNIX socket `/tmp/netd` |
| Addresses (emulator kit) | Idris 10.0.2.15. **Your computer = 10.0.2.2** (its 127.0.0.1) |
| Speed | about 1.6 KB/s to Idris, 0.7 KB/s back |
| File names | 14 characters at most |

### The C compiler (Whitesmiths C 3.2)

- `int` is 32 bits. Plain `char` is **unsigned**.
- Prototypes work, but `__STDC__` is not defined. `_WSLAS` is.
- **Bug**: `a[n - 1]` on a **local** array reads the wrong memory. Make arrays `static`.
- `putc()` on stderr crashes. `fprintf(stderr, ...)` is fine.
- `c file.c` compiles, then tries to link and prints `undefined: ...`. That is normal.
- `as.68k` reaches a function in the same file with a 16-bit offset: 32 KB at most.
  `main.c` has more code than that, so `mk` runs the passes of `c` by hand for it, with `as.68k -z`:
  then every call gets a 32-bit address. `c` has no flag for `-z`. `as.68k -z` then says
  `16-bit value exceeded` and `word pc range` for about 13 branches in `main()` back to a label
  past 32 KB. These messages are false: the code is right, and Archaic works.
- `<X/Xlib.h>` is not on the include path, and `Xlib.h` wants a
  `<sys/param.h>` that Idris does not have. `sh mkinc` fixes both,
  in the build folder.
- The default link line allows only a small BSS. `lka` adds `-m0xffffffff` to `+bss`.
- The default link also gives a program only about 26 KB for `malloc`
  (one slice of a long page). `lka` adds `-b0x30000`: 192 KB for the heap
  and the stack. The 4-byte field at offset 16 of the program file holds
  this size.
- The linker itself (`/etc/bin/link`) asks for 128 KB. That is too little to link Archaic
  with the ABM3 and AB8 decoders: it says `no memory`, and writes a broken program. So `lka` links with a copy of
  the linker that asks for 512 KB: `heap` (from `heap.c`, which `mk` builds) sets the field in the copy.
- There is no `times()`: the only clock is `time()`, in seconds.
- `XCloseDisplay` pulls in `sigsetmask`, which no library has. Archaic does not call it.

### The shell

- No `for`, no `if`, no `2>&1`. Variables are one letter. Use `setenv NAME value`.
- **`$` is expanded even inside single quotes.** So `-s` scripts have no `$`.
- `#` is **not** a comment. `:` with plain words is.
  Even then, no quotes, `;` or brackets in it.
- `cp /dev/null file` copies the device. Use `cat /dev/null > file`.
- `rm` has no `-f`. There is no `tail`, no `which` and no `rmdir`:
  `rm -d DIR` removes an empty directory.
- `chmod 755 file` does not work: chmod takes flags, not numbers.
  `chmod file` (no mode) makes a file executable.
- `$PATH` is `$P` followed by `ATH`. The command search path is `$X`.
- In the environment, `PATH` is like `|/etc/|/bin/|/etc/bin/|/usr/bin/`: `|` before each folder,
  a `/` at the end of each, and the empty first one is the current folder. `set X ...` changes it too.
  A program started by name gets only its name as `argv[0]`.

### X10 on this ST

- The screen size reads as 0 x 0. Archaic then uses 640 x 400.
- The keyboard goes to the window under the mouse pointer.
  Archaic moves the pointer into its window when it opens.
- Bitmaps: rows of 16-bit shorts, the **low** bit is the **left** pixel.
- The **left** mouse button comes as `MiddleButton` (1), the right one as `RightButton` (0).
- All four arrow keys come as the same key. Archaic does not use them.
- No scroll bars: Archaic draws its own. Archaic asks for no motion events:
  the elevator moves when the button is let go, as in GEM.
- **The root window's X cursor is drawn 7 pixels up and left of where it clicks.**
  Archaic sets its own arrow cursor, with the hot spot at its tip (0,0).
- Cursor colors are ignored: cursor bit 1 is drawn white, the rest of the mask black.
- Start X: log in on the console, run `/usr/X/bin/xstart`. It takes about 40 s.

### netd

- Send `tcp HOST PORT\n`. The answer line is `ok ...` or `err why`.
- Then the data. From netd, byte `0377` comes twice, and `0377 0` means the end.
- The program closes the socket, never netd.
- Sockets ignore FIONBIO: a read waits. Read only what `FIONREAD` says is there.
- `select()` takes one `unsigned long` as the mask, and whole seconds.
- netd treats a short write as "the program has gone". So read soon.
  Archaic keeps a 16 KB buffer and fills it while it decodes the picture.
