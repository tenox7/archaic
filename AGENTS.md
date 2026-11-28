# Archaic: notes for agents

The details: how Archaic works with WRP, building, tests, porting.
The facts of each old system are in `ports/`, a file for each system (see The systems).

README.md is for people. Keep it short: what Archaic is, screenshots, platforms, quick start, build.
Show each system once in README.md: its emulator's window, at the top.
Put new details here or in `ports/`, not in README.md.
Keep this file under 150,000 characters (the limit of Claude Code): the facts of one system go in its file in `ports/`.

Test on a copy of a system's disk, never on the user's own disk.

## What Archaic shows

| What Archaic shows | What it skips |
|---|---|
| text fields, password fields | JavaScript, CSS, HTTPS (WRP does all that) |
| buttons, drop-down menus (SELECT) | check boxes, radio buttons, TEXTAREA |
| links, page title, line breaks | tables, fonts, colors |
| GIF, ABM and AB8 pictures, clickable (ISMAP) | PNG, JPEG (Archaic asks WRP for GIF, ABM or AB8) |

## Files

| File | What it is |
|---|---|
| `src/archaic.h` | everything shared, and the C rules for old compilers |
| `src/main.c` | the page on the screen: layout, drawing, mouse, keys, forms |
| `src/html.c` | HTML reader: forms, menus, links, images, text |
| `src/http.c` | URLs and HTTP/1.0, with a 16 KB read buffer |
| `src/gif.c` | GIF decoder: the colors, or black and white with a 4x4 dither. WRP's black and white GIFs go a fast way, and its pictures in colors the 8-bit way (see Pictures: GIF, ABM, ABM3 and AB8) |
| `src/pic.c` | pictures: looks at what came (GIF, ABM or AB8); the ABM, ABM3 and AB8 decoders |
| `src/update.c` | updates: asks WRP for a newer Archaic, and puts it in the place of the program (see Updates) |
| `src/x10.c` | display for X10 (Idris) |
| `src/nidris.c` | network for Idris: TCP through netd |
| `src/x11.c` | display for X11 (Unix, HP-UX, DG/UX, AIX, Ultrix, macOS with XQuartz) |
| `src/cocoa.m` | display for macOS: a Cocoa window |
| `src/sdl.c` | display for SDL 2 (macOS, Linux, Windows ...) |
| `src/headless.c` | display with no window, for tests and automation: commands on standard input, screenshots |
| `src/fb.c`, `fb.h` | a screen in memory, for `cocoa.m`, `sdl.c`, `gem.c` and `headless.c`; writes PNG screenshots |
| `src/font6x13.h` | the X11 6x13 font (public domain), for `fb.c` |
| `src/candle.h` | Archaic's logo, the candle: 9 frames (no flame, then 8 lit), 1 bit a pixel. Made by `tools/candle.c` |
| `src/nsock.c` | network for BSD sockets (Unix, macOS, FreeMiNT, A/UX). On A/UX it waits in the Toolbox's `WaitNextEvent` (see `ports/aux3.md`) |
| `src/fetch`, `mkinc`, `mk`, `lka` | Idris scripts: get the sources, headers, compile, link |
| `src/heap.c` | Idris tool: sets how much memory a program file asks for. `lka` links with a copy of the linker that asks for more |
| `src/unixpc.c` | display for the AT&T UNIX PC (3B1) window system |
| `src/ntwg.c` | network for the 3B1: the BSD sockets of WIN/TCP (TWG). Also for the 3B2: WIN/3B |
| `src/mkunixpc` | 3B1 script: compile and link with gcc |
| `src/dmd.c` | display for the AT&T DMD 5620 terminal (the Blit) in layers, from a 3B2: commands to Archaic's part in the terminal. It keeps a copy of the screen, so that only what changes goes down the line (see `ports/3b2.md`) |
| `src/dmdterm.c` | Archaic's part in the DMD 5620: it draws, it keeps and decodes the page's slices (ABM, ABM3), it scrolls. Built with the DMD's compiler |
| `src/mk3b2`, `bin2c.c` | 3B2 script: compile and link with the 3B2's cc and `dmdcc`. `bin2c` puts the terminal's part into `archaic` as a C table (`dmdm.h`) |
| `src/sunview.c` | display for SunView (SunOS 3 and 4) |
| `src/mksunview` | SunOS 3 and 4 script: compile and link with Sun's cc |
| `src/mkhpux` | HP-UX 9 script: compile and link with HP's cc, for X11 (`x11.c`, `nsock.c`) |
| `src/mkdgux` | DG/UX R4.20 script: compile and link with DG/UX's cc, for X11 (`x11.c`, `nsock.c`) |
| `src/mkaix` | AIX 3.2 script: compile and link with IBM's XL C (or gcc), for X11 (`x11.c`, `nsock.c`) |
| `src/mkaixps2` | AIX PS/2 1.3 script: compile and link with MetaWare High C (or the kit's hcc), for X11 (`x11.c`, `nsock.c`) |
| `src/mkultrix` | Ultrix 4 script: compile and link with Ultrix's cc (or gcc), for X11 (`x11.c`, `nsock.c`) |
| `src/mac.c` | display for the classic Mac OS (System 6, Mac OS 7.5) on a 68k Mac: a QuickDraw window with the Mac's own scroll bar, and the Mac's `main()`. It shares the CPU with the other programs (see `ports/mac.md`). Also the display for A/UX 3, through the A/UX Toolbox (`-DAUX`) |
| `src/nmactcp.c` | network for the classic Mac OS: MacTCP's TCP driver (on Mac OS 7.5: the MacTCP part of Open Transport) |
| `src/dnr.c`, `dnr.h` | the classic Mac OS: MacTCP's name resolver, for host names (from HTTPHead, a MacTCP sample for Snow). C99: only the Mac build compiles it |
| `src/mkmac`, `CMakeLists.txt`, `archaic.r` | the classic Mac OS: build with Retro68 in Docker, on your computer (`make mac`). `archaic.r`: the SIZE resource, with the multitasking flags |
| `src/gem.c` | display for GEM on the Atari (FreeMiNT with XaAES, as on ARAnyM): a GEM window with GEM's own scroll bar. It shows `fb.c`'s screen in memory through the VDI: in colors on a screen of 15 to 32 bits, else in black and white |
| `src/mkmint` | FreeMiNT: build with the m68k-atari-mint gcc in Docker, on your computer (`make mint`): `archaic-mint.prg` |
| `src/tos.c` | display for GEM on TOS: the ST, STE, TT and Falcon, and graphics cards. A GEM window with GEM's own scroll bar. It keeps the window's pixels in the screen's own format (1 bit, the Atari's bit planes, or packed pixels) and draws them itself, so a redraw only copies (see `ports/tos.md`). Also TOS's `main()`: the options from `ARCHAIC.INF` |
| `src/nsting.c` | network for TOS: STinG's TCP (a NetUSBee or an EtherNEC card), found through the cookie `STiK` |
| `src/mktos` | TOS: build with the m68k-atari-mint gcc in Docker, on your computer (`make tos`): `archaic-tos.prg`, `ARCHAIC.PRG` on the Atari |
| `src/ntwin.c` | display for Windows NT (3.1 and newer, on x86, MIPS, Alpha and PowerPC): a Win32 window drawn with GDI, with NT's own scroll bar, a palette of its own on a screen of 256 colors (see `ports/nt.md`) |
| `src/nwin.c` | network for Windows NT: Winsock 1.1 (WSOCK32.DLL) |
| `src/mknt`, `archaic.rc`, `archaic.ico` | Windows NT: build with Microsoft's own SDKs in Docker (the image `tenox7/ntsdk`), on your computer (`make nt`): `archaic-ntx86.exe`, `archaic-ntmips.exe`, `archaic-ntalpha.exe`, `archaic-ntppc.exe`; ARM (PINT) with the image `tenox7/pint35sdk`: `archaic-ntarm.exe`. `archaic.rc`: the icon |
| `src/mkaux`, `auxrsrc.r` | A/UX 3 script: compile and link with A/UX's cc and the A/UX Toolbox (libmac), for `mac.c` and `nsock.c`. `auxrsrc.r`: the SIZE resource, which `rez` puts in `%archaic`, the resource file next to the program |
| `tools/idris.py` | host tool: run commands in Idris over telnet, serve files |
| `tools/hget.c` | Idris tool: download one file over HTTP (through netd) |
| `tools/slowline.py` | host tool: a TCP proxy that is a slow line (bytes a second), to try Archaic on a slow line with WRP |
| `tools/ntico.py` | host tool: draws Archaic's icon for Windows NT (`archaic.ico`): the candle in 3D, 32 x 32 and 16 x 16 in 16 colors, 32 x 32 in black and white |
| `tools/pcf2c.py` | host tool: an X11 PCF font as a C table (it made `font6x13.h`) |
| `tools/candle.c` | host tool: draws Archaic's logo, the candle, as a C table: `cc -o candle tools/candle.c -lm`, `./candle > src/candle.h`. `-p prefix` also writes each frame as PBM. `-s n`: a picture of n x n (default 20; the design is 32) |
| `tools/dmdlog.py` | host tool for the DMD 5620 port: decodes a log of the bytes that Archaic sent to the terminal (`ARCHAICLOG`), a command a line, and draws the screen they make |
| `test/run.sh` | tests: GIF decoder, HTML, forms, WRP, the headless display |
| `test/xshot.sh` | test: run the X11 build and take a screenshot |
| `test/data/` | GIFs saved from WRP, for the tests; two of them also as ABM and ABM3 (`.ab3`), two as AB8. `full.gif`, `runs.gif` and `ilace.gif` are made for the 8-bit way: a full code table with no clear, long runs, interlaced. `dith` is noise in 8 colors: smaller as a GIF than as AB8. WRP makes the `.ab3` and `.ab8` files from the GIFs: `WRP_SAMPLES=dir go test -run TestSamples` |
| `test/cfacts.c`, `cfacts2.c` | probe for a new port: what its C compiler and linker do (name length, case, struct copies) |
| `screenshots/` | the pictures in README.md: the emulators' windows, the scroll bar, and WRP's buttons and labels in `icons/` (drawn from the pictures in `main.c`, as `drawbutton()` draws them, at their real size). Also each system's own screen, for this file (see each system's file in `ports/`) |

## Options

```
archaic [=WxH+X+Y] [-a] [-c n] [-f type] [-t] [-v] [-o file] [-s script] [-r n] [-u] [-n] [--version] [--help] [url]
```

| Option | Meaning |
|---|---|
| `url` | WRP's address, like `10.0.2.2:8080`. Default: `$WRP`, else port 8080 on the host (Idris, the 3B1, the 3B2, SunView, HP-UX, DG/UX, AIX, FreeMiNT, TOS and Windows NT: `10.0.2.2`, A/UX: `192.168.1.10`, other Unix and the classic Mac OS: `127.0.0.1`). Ultrix in gxemul, Mac OS 7.5 in Snow and TOS in Hatari need WRP's address: see `ports/ultrix4.md`, `ports/mac.md` and `ports/tos.md` |
| `=WxH+X+Y` | window size and place (also `-g WxH+X+Y`). Default on Idris and SunView: about 9/10 of the screen, in the middle. On the 3B1: 686 x 248 inside a frame; a larger size gets no frame, up to 720 x 288 (`-g 720x288`). On the classic Mac OS and A/UX: the screen under the menu bar (on System 7 and A/UX, less the column of disk icons at the right). On the DMD 5620: the layer as it is (Reshape in the layers menu changes it) |
| `-a` | also show WRP's M, T and C menus, and send them as they are set |
| `-c n` | start with pictures of n colors (2 to 256), not as many as the display shows (see Colors). With `-t`: as if the display showed n colors |
| `-f type` | ask WRP for one type of pictures, with no choice. `gif`: GIFs only. `abm`: ABM, and GIFs in colors (as Archaic 0.11). `abm3`: ABM3. `ab8`: AB8, and ABM. `small`: for each picture the smallest type. Without `-f`, Archaic times its pictures and asks for the kind that is faster here (see Pictures: GIF, ABM, ABM3 and AB8) |
| `-t` | test mode: no window. Prints the page items, and for each picture what it came as (`gif`, `abm`, `abm3`, `ab8`) and a checksum |
| `-v` | status messages also go to stderr, and the times of each trial of the picture types |
| `-o file` | at the end, save the first picture: PBM, or PPM if it has colors |
| `-s script` | play a script, then quit (see Scripts, in `ports/idris.md`) |
| `-r n` | typing into the page: WRP's picture again after n seconds with no keys. Default 3, 0 never. Not on the 3B1, Idris and the DMD 5620 (no clock). A new picture that takes longer than n seconds turns it off for the run (see Typing into the page) |
| `-u` | update: ask WRP for a newer Archaic and install it, with no window, then quit (see Updates) |
| `-n` | no updates. Without it, Archaic asks WRP once a run (not with `-t` or `-s`) |
| `--version` | print the version, then quit: `Archaic v0.9`. A release build also names its system (see Updates): `Archaic v0.9 for aix3` |
| `--help` | print all the options, a line each, then quit |

The classic Mac OS has no command line: Archaic reads its options from the file `Archaic Options`
(see `ports/mac.md`).

## WRP's form

**WRP's settings are set by Archaic, and not shown:**

| WRP box | Archaic sends |
|---|---|
| W, H | the window's size: the picture fills the window. Always, also with `-a` |
| M (mode) | ISMAP |
| T (picture type) | GIF. WRP sends its pictures as ABM, ABM3 or AB8 in its place, as Archaic asks (see Pictures: GIF, ABM, ABM3 and AB8) |
| C (colors) | a display with colors: the menu shows (see Colors). Black and white: WRP's one menu shows its lines of 2 (see below); with an older WRP, or with `-c n`, Archaic sends 2 (or n), and C is not shown |
| SL (slices) | 1: the whole page, in slices (see Scrolling). An older WRP ignores it |

So WRP's form is one line: the address box, **Go**, WRP's buttons, **Z** (zoom),
**C** (colors and dither) and **K** (keys).
The labels Z, C and K are pictures: a magnifying glass, a palette and a keyboard.
With a WRP that has `kb` in its form, K, ⌫ and ↵ are gone: the keys go straight to the page
(see Typing into the page).
On a black and white display, C has only the dither, and its label is a dither pattern (a checkerboard).

The address box is as wide as the line allows: WRP's buttons and menus are at the right end of the line.
Right of them, in the top right corner, is Archaic's logo: a candle, 13 x 19 pixels, black and white
(`src/candle.h`, which `tools/candle.c` draws). It is as tall as WRP's buttons.
While the spinner turns (see The status line), the candle is lit, and its flame moves left and right:
a frame for each step of the spinner. Then the flame goes out.
On the DMD 5620 the flame is lit but does not move: each frame would go down the slow line.
A line with no room for the logo (21 pixels with its margins) has none, and the box gets the room that is left.

**C** is one menu for the colors and the dither (WRP 4.12.0 or newer).
Each line is a number of colors and a letter:

| Letter | The picture |
|---|---|
| d | dithered (Floyd-Steinberg): all of the page. Photos and shades keep their tones. In 2 colors, text in a color, gray text and text on a color become dots |
| n | no dither: each pixel gets the nearest color. Photos get bands of color. In 2 colors, the smallest pictures |
| p | only in 2 colors, for reading: only photos are dithered. Text and lines are sharp, and the pictures are about 20% smaller than 2d |

The lines are 216n, 216d, 256d, 256n, 128d, 128n, 16d, 16n, 2p, 2d and 2n
(WRP 4.14.2 and older: 256d and 256n first).
128 is for SunView: its colormap segment has 128 places. (WRP 4.13.0 and older also have 64d and 64n.)
The first line of each number is WRP's default for it: 216 is sharp (the web colors, see 216 colors: GIP),
2 dithers only photos, and the other numbers are dithered.

- On a display with colors, C shows all the lines. On a black and white display, only 2p, 2d and 2n.
- Choose another line, and Archaic sends WRP's form at once with St: WRP makes the picture again.
  The next pages keep the line.
- An older WRP has only numbers in C, and dithers no colors. Its **D** menu has the dither of 2 colors:
  All (2d), Pics (2p) and None (2n). Archaic shows D on a black and white display, and hides C there.
- Archaic 0.7 and older keep only the number: on the next page, C is at the first line of that number again.

The address box has a clear button (a white × in a black circle) at its right end,
while it has text. Click it: the box is empty, and ready for a new address.

**WRP's buttons are pictures**, like in a modern browser.
(WRP names them with 2 letters, because old browsers can only put text on a button.)

| Picture | WRP's name | Does |
|---|---|---|
| ← arrow | Bk | back |
| stop sign | St | stop loading |
| ↻ circle arrow | Re | reload |
| ↑ and ↓ arrows | Up, Dn | scroll the web page one screen up or down. Only an older WRP shows them: see Scrolling |
| ⌫ | Bs | Backspace, for the web page. With text in K: the text, then Backspace. Only an older WRP (no `kb`) |
| ↵ | Rt | Return (Enter), for the web page. With text in K: the text, then Return. Only an older WRP (no `kb`) |

With an older WRP (no `kb` in its form), to type into a box on the web page:
click the box in the picture, then type in **K**.

- Click ↵: WRP types the text into the page, then presses Return (to search, for example).
- Or press Return: WRP types only the text. Use this to fill in more boxes before you send the page's form.

(WRP alone does only the button's key when it gets both a button and keys, so Archaic sends
the text and the button's key together as keys: `k=text%0D`. This works with every WRP.)

**The window's title** is the page's title, like `Hacker News`. WRP's page title is `WRP` and the page's address:
there the window says the address, like `https://en.wikipedia.org/`. Before the first page, it is `Archaic`.
Archaic's version is at the right end of the status line, like `Archaic v0.10` (see The status line).
With `-t`, the `title` line is still the page's own title.

## Colors

Archaic asks the display how many colors it shows.
The first status line says what it found, like `Archaic 0.10, millions of colors`.

| Display | Colors |
|---|---|
| 3B1, Idris (X10), DMD 5620 | black and white (they are) |
| SunView | a color screen (8 bits, as the cg3): 128, in a colormap segment of Archaic's own. A black and white screen (bw2, as the Sun-3's bwtwo): black and white |
| X11 | the default visual. Depth 1: black and white. TrueColor: millions. A colormap (8-bit PseudoColor, as on old workstations): its size, like 256 |
| Cocoa, SDL | the screen's depth: millions |
| Mac OS 6 and 7.5 | the main screen's depth, as set in Monitors: 2, 4, 16 or 256; millions with 16 or 32 bits. A Mac with no Color QuickDraw: black and white |
| GEM on TOS (`tos.c`) | 1 plane (ST high, TT high) and ST medium's 4 colors: black and white. 16 and 256 colors (ST low, TT, Falcon, a card in 8 bits): 16 or 256. 15 to 32 bits (the Falcon's true color, cards): millions |
| Windows NT (`ntwin.c`) | the screen's bits for a pixel: 2, 16, 256 (a palette: see `ports/nt.md`), 65536, millions |
| `archaic-headless` | millions, like SDL (`-c 2` for black and white). `ARCHAIC_COLORS=N`: as a display of N colors (for the tests) |

On a display with colors:

- WRP's **C** menu shows. Millions of colors: C stays at WRP's line, as WRP's `-g` says
  (216n by default: the fastest), and the start page asks with no `c`.
  Fewer colors: C is set to the display's colors: the first line of the nearest of WRP's
  numbers (256, 216, 128, 16, 2) that is not more, or of `-c n`.
  A display of 256 colors (an 8-bit colormap) gets 216n: the web colors, as the old browsers had
  (see 216 colors: GIP). Choose 256d in C for the page's own colors.
- Choose another line there, and Archaic sends WRP's form at once with St (stop).
  WRP takes the picture again in those colors, and does not load the page again.
  The frame stays where it was.
- The pictures keep their colors: a byte for each pixel, and the GIF's colors.
- SunView: Archaic's colormap segment has 128 places: white, black and 126 for the picture.
  A segment of 256 would take all of the screen's colors, and the other windows would lose theirs.
  When a picture has more colors than places (C at 256, or 128 with no pure black),
  the color nearest to another one gives up its place, until they fit:
  so a small red line keeps its red. The others take the nearest color.
- X11 with a colormap: Archaic takes cells in the default colormap only for the colors
  that a picture uses, when it draws it. They are shared with the other programs.
  When the colormap is full, it takes the nearest color the colormap has.
  A new palette frees the last one's cells.

On a black and white display, C shows only its lines of 2 (an older WRP: C is hidden).
Archaic asks WRP for 2 colors (or for `-c n`: then C is hidden).
The pictures are 1 bit a pixel; one with more colors gets a 4x4 dither.

Archaic's own start page request asks WRP for the same settings as the next ones
(`/?m=ismap&t=gif&c=256&sl=1`, with C's colors), so that WRP's form does not change after the first page.

### 216 colors: GIP

With C at 216n, WRP makes **GIP** pictures: the 216 web colors (a 6 x 6 x 6 color cube), with no dither.
216d dithers the page to the same 216 colors.
Archaic still asks for GIF (`t=gif`), not `t=gip`: WRP shows the C menu only for GIF.
(WRP before November 2026 makes 216 colors of the page itself, with median cut, as for 256.)

The web colors are the same on every page. On a display with a colormap (8 bits) this matters:

- Archaic takes cells only for the colors a picture uses: a page of text takes 20 to 90 cells.
  The cells stay from page to page, and the other programs keep the rest.
- Other programs that show the web colors (as the old browsers) share the same cells.
- WRP's own 216 colors (median cut) are new on each page, and a slice uses almost all of them.

Measured on 7 text pages (Wikipedia, Hacker News, BBC News, CNN Lite, the Go docs, a man page, GitHub),
slices of 632 x 351, the same screenshots:

| | Median cut, 216 | GIP |
|---|---|---|
| All 7 pages | 9.9 MB | 5.4 MB |
| One slice, decoded on the 3B1 (3 of the pages) | 2.2 to 3.2 s | 1.8 to 2.2 s |
| WRP's work for a page | 7 to 67 ms more (the page's palette) | none |
| Colors in the first 2 slices | 79 to 215 | 16 to 87 |
| Text (macOS OCR, and by eye at 2x) | the same | the same |
| Light tints: Hacker News's beige, gray code boxes | kept | white |
| Photos | good | bands of color |

On an 8-bit X display (Xvfb) with 183 free cells, 4 Archaic windows with 4 of these pages
took 72 cells with GIP, and all showed the right colors. Before Archaic took cells only for
used colors, one window took all 183, with GIP too. With median cut, the windows after the first
had no free cells, and their photos lost their colors.

## Pictures: GIF, ABM, ABM3 and AB8

A GIF has a code for every few pixels. An old CPU needs a long time for that:
a 10 MHz 68010 needs about 8 seconds for one picture of 551 x 310 pixels.
That was most of the time of a page, also on a slow line.

WRP has three picture types of its own for Archaic:

| Type | Pictures | What it is | Size | To decode |
|---|---|---|---|---|
| **ABM** | black and white | the display's own bytes (1 bit a pixel), packed with a byte LZ | 10 to 20% less than the GIF | very fast: no work for each pixel |
| **ABM3** | black and white | the same pixels, from an arithmetic coder that learns the page | about 39% less than ABM | slow: work for each coded byte and pixel |
| **AB8** | colors | a byte for each pixel, packed with a byte LZ | most often less than the GIF. With a dither to the 216 web colors: about 20% more | fast: 3 times as fast as the GIF |

Seconds to decode one slice of 632 x 351 pixels (CPU time, no network; mean sizes of 152 slices from 15 web pages):

| Type | Size | 3B1 (10 MHz 68010) | Atari ST (8 MHz 68000) | Mac IIcx (16 MHz 68030) |
|---|---|---|---|---|
| GIF, 2 colors (the fast way) | 8.2 KB | 1.55 | 2.97 | 0.53 |
| ABM | 6.7 KB | 0.20 | 0.30 | 0.05 |
| ABM3 | 4.1 KB | 2.75 | 4.52 | 1.02 |
| GIF, 256 colors (the 8-bit way) | 40 KB | | | 1.30 |
| AB8, 256 colors | 36 KB | | | 0.43 |

- Archaic's requests name the types it takes: `Accept: text/html, image/x-abm, image/x-abm3;q=0.5, image/x-ab8, image/gif;q=0.5`.
  A type with `q=0.5` is one that Archaic takes, but does not want now.
- WRP (with `picfmt.go`) makes the picture in each type that Archaic likes best, and sends the smallest one.
  For a black and white picture (2 colors) the types are ABM and ABM3; for more colors, AB8 and the GIF.
- When WRP chose between two types, its answer has `X-Fast-Length`: the picture's size in the type
  that decodes fastest (ABM, AB8).
- An older WRP knows only `image/x-abm`: it sends ABM for black and white, and GIFs for colors. A WRP before ABM sends only GIFs.
- On a display with colors, an ABM picture shows as it is: black and white (C set to 2).
  Archaic asks for AB8 only on a display with colors.
- `-f` asks for one type, with no choice (see Options). Use it to compare the types on your computer:
  `archaic -t -f gif ...`, then `archaic -t -f abm ...`. The last status line has the time.
- WRP's black and white GIF (2 colors in its color table) goes a fast way in the GIF decoder too:
  about 5 times faster than before.
- A GIF in colors, on a display with colors, goes the 8-bit way (`lzw8`), if it fills its screen
  and is not interlaced (WRP's GIFs do). Other GIFs are decoded a pixel at a time.

The file formats are in `src/pic.c`, and in WRP's `abm.go`, `abm3.go` and `ab8.go`.

On the DMD 5620, the terminal decodes ABM and ABM3 itself (`dmdterm.c`): the 3B2 passes WRP's file on
as it comes. A decoded picture would be 6 times as many bytes on a line of 450 bytes a second.

### The automatic choice

Which type is on the screen first depends on the CPU and on the line.
On a fast line the CPU is the slow part, and the type that is fast to decode is best.
On a slow line the bytes are the slow part, and the small type is best.
Seconds for a page of 6 typical slices in black and white (`-t`, at the computer's real speed):

| Computer | Line | GIF | ABM | ABM3 |
|---|---|---|---|---|
| Mac IIcx | 9600 baud | 52 | 42 | 26 |
| | 19200 baud | 27 | 22 | 14 |
| | 38400 baud | 14 | 11 | 11 |
| | Ethernet | 7.7 | 4.5 | 10 |
| 3B1 | 9600 baud | 54 | 44 | 29 |
| | 19200 baud | 28 | 23 | 23 |
| | 38400 baud | 17 | 13 | 24 |
| | Ethernet | 14 | 4.7 | 21 |
| Atari ST (SLIP, netd) | 19200 baud | 45 | 25 | 42 |

Archaic does not know its CPU's speed (an emulator can run it 10 times as fast) or its line's.
So it measures. First with a test:

- **The picture test.** Once a run, when WRP's first page is here and Archaic has nothing to do,
  it gets a test picture from WRP: once in the fast kind, once in the small kind, and times the two.
  The picture is a part of a sample page, as large as the frame, in the colors in use (`/archaic-test`).
- The status line shows the test: `Picture test 1 of 2 (ABM): 3 of 8 KB` with the progress bar,
  `Picture test 2 of 2 (ABM3): 2 of 5 KB`, then the result, which stays:
  `Picture test: ABM3 is used (2.5 s, ABM 4.3 s)`. The small kind is used if it took 10% less.
- WRP's log has the test too: `Archaic: picture test starts: ABM, then ABM3 (615x349, 2)`,
  a `Picture test:` line for each picture, and `Archaic: picture test: ABM3 is used (2.5 s, ABM 4.3 s)`.
- A click or a key stops the test: it comes again later (3 times at most). Esc stops it for the run.
  No test with `-f`, `-t` or `-s`, or with a WRP that has none (4.14.1 and older).

Then with the pictures of the pages, so that the choice can change later:

- It times each picture, from the answer's header until the last row is there. The time before
  the header (the connect, the request, WRP's work) is the same for both types, and is not counted:
  for a small picture it would be most of the time.
- Only pictures that are not drawn while they come are counted: of a page's slices, the ones that
  come ahead. Drawing takes time that the others do not have. (With `-t`, all are counted.)
- There are two kinds: **fast** (ABM; in colors AB8) and **small** (the smaller one of ABM3 and ABM;
  in colors of the GIF and AB8). Archaic asks for a kind through the `q` of the types (see above).
- After the test, Archaic goes on with the kind that the test chose. With no test (an older WRP, `-s`, `-t`),
  the first 2 pictures that count are of the fast kind, and the next 2 of the small kind: the first trial.
- A trial: the other kind is tried for 2 pictures, and takes over if its time for a byte is 10% less.
  Pictures of the two kinds are not the same pictures: the time counts for a byte of the fast type,
  which `X-Fast-Length` gives for a small picture.
- The next trial comes after 8 pictures, and after twice as many each time, up to 64.
  If the other kind took 2 times as long, the next trial comes after 64 pictures at once.
- Older pictures count less: at 16 pictures of a kind, their sums are halved.
- A kind whose pictures take less than 1/10 s stays: there is nothing to win.
- Black and white pictures and pictures in colors have a choice each.
- The clock is the display's (`g_ms`). The 3B1's is `times()` (1/60 s), Idris's the seconds of `time()`
  (`nowms()` in `http.c`). A clock that coarse can't tell after 2 pictures: then the trial goes on,
  for 6 pictures at most.
- The status line shows each slice that came, with its type, size, time and speed:
  `Picture part 3 of 9: ABM3, 5 KB in 2.7 s, 1.9 KB/s`. So a new type shows there. The line of a picture
  that comes names its type too (`Picture part 3 of 9 (ABM3): 4 of 7 KB`), and so does the page's last line
  (`2 s, 45 KB, ABM3  http://...`).
- A trial is not on the status line: it is in WRP's log, and with `-v` on stderr.
- With `-v`, each trial's sums also go to stderr:
  `archaic: pictures in black and white: fast 5798 ms for 10 KB, small 5206 ms for 15 KB`.
- WRP's log gets each trial too: `Archaic: pictures in black and white: trial of ABM3 starts (ABM is in use)`,
  then `Archaic: pictures in black and white: now ABM3 (2.1 s for 10 KB, ABM 3.4 s); fast ...` (or `still ABM`).
  The times are each kind's time for 10 KB of the fast type; in colors the small kind is `GIF or AB8`.
  Archaic sends these notes in the request header `X-Archaic-Note`.

To try a slow line on your computer: `python3 tools/slowline.py 8098 8080 1920` is a line of 19200 baud
to the WRP on port 8080. Start Archaic with `-v` and port 8098 (in an emulator kit: `archaic -v 10.0.2.2:8098`).

Measured with WRP and a long page (Wikipedia's Unix article, 18 slices, a scroll every 2 s), in Archaic's
window on the 3B1 at its real speed (10 MHz), started with `-s` (so with no test at the start: the choice
came from the first trial). Seconds from the first slice to the last one:

| Line | Archaic chose | With the choice | `-f abm` | `-f abm3` |
|---|---|---|---|---|
| 9600 baud | ABM3 | 101 | 128 | 98 |
| 19200 baud | ABM | 85 | | |
| 38400 baud | ABM | 70 | | |
| Ethernet | ABM | 65 | | |

On the Atari ST in X (Idris, SLIP) Archaic chose ABM: a page and 10 scrolls took 95 s with `-f abm`
and 122 s with `-f abm3`. On the Mac IIcx in 216 colors on Ethernet it chose AB8.

### Rows on the screen

A call that draws has a fixed cost. On a 68030 Mac it is about 15 ms (CopyBits), and the status line
takes 7 ms more: 44 calls for a slice (8 rows each) took longer than its pixels.
So new rows wait, and go to the screen at most 5 times a second, with the progress (`img_rows` in `main.c`).
On Idris (a clock in seconds) they go once a second.

| Measured | Before | Now |
|---|---|---|
| Mac IIcx, 216 colors: the slice on the screen (609 x 384) | 2.53 s (GIF, 13 KB) | 1.18 s (AB8, 12 KB) |
| Mac IIcx, 216 colors: the next slice, not on the screen | 1.76 s (GIF, 20 KB) | 0.79 s (AB8, 19 KB) |
| 3B1 in its window, Ethernet: a page and 10 scrolls, ABM | 28 s | 23 s |

### ABM against GIF, and other formats

Measured with `-t`: one page with 6 pictures of 551 x 310 pixels (42 KB as GIF, 37 KB as ABM):

| Computer and line | Archaic 0.3 (GIF) | GIF, the fast way | ABM |
|---|---|---|---|
| 3B1 (10 MHz 68010), Ethernet | 49 s | 11 s | 4 s |
| 3B1, a line of 4000 bytes a second | 50 s | 14 s | 12 s |
| 3B1, a line of 1600 bytes a second | 52 s | 29 s | 26 s |
| Atari ST (8 MHz 68000), SLIP | 100 s | 40 s | 25 s |

With ABM, the line is the slow part, not the CPU.

Other formats were measured too. The sizes are the mean of 171 pictures of 551 x 310 pixels
from 12 web pages. The times are for one picture on the 3B1, with no network:

| Format | Size | Time to decode |
|---|---|---|
| GIF, as Archaic 0.3 decoded it | 6.9 KB | 7.5 s |
| GIF, the fast way | 6.9 KB | 1.3 s |
| **ABM** | 6.2 KB | 0.15 s |
| deflate (as in PNG) | 5.3 KB | 0.6 s |
| PackBits | 7.5 KB | 0.15 s |
| the bits as they are | 21 KB | 0.15 s |
| XBM | 108 KB | 5 s |

- Deflate is smaller than ABM, but slower to decode. On the 3B1 it was 12% faster on the line of
  1600 bytes a second, and 60% slower on Ethernet. On the ST with SLIP it was 12% slower:
  there netd uses the CPU too. So ABM has the simple packing.
- The bits as they are, and XBM, are never faster: they are 3 and 15 times larger than ABM.
- VNC (RFB) has no 1-bit pixels. Its simple encodings carry 8 bits or more a pixel
  (Raw: 24 times the GIF, Hextile: 2.7 times). Its small ones (Zlib, ZRLE, Tight) are deflate: see above.

**The 8-bit way** uses the picture as its own dictionary, as Larry Bank's
[AnimatedGIF](https://github.com/bitbank2/AnimatedGIF) does in its Turbo mode.
A code's pixels are already in the picture, where they came the first time.
So for each code Archaic keeps that place and the number of pixels (16 KB from malloc, once),
and a code is one copy of earlier pixels. Each pixel is written once.
The way before put each pixel on a stack, then in a row, then in the picture.
The picture is also filled with its lightest color first, now 4 bytes at a time (not 1).

Measured on the 3B1 (10 MHz 68010, CPU time for one 632 x 351 slice from WRP, no network):

| Slice | Before | The 8-bit way |
|---|---|---|
| Wikipedia, GIF 216 colors (25 KB) | 9.1 s | 2.2 s |
| BBC News, GIF 216 colors (45 KB) | 10.5 s | 3.2 s |
| Hacker News, GIF 216 colors (41 KB) | 10.2 s | 2.9 s |
| Hacker News, GIP (20 KB) | 8.8 s | 1.9 s |

On the Mac it is 2 to 3 times faster than before. Wuffs, the fastest GIF decoder found,
keeps up to 8 pixels for each code; it is C99, and Archaic's compilers are older.

## Mouse and keys

**Mouse**

- Left button: click links, buttons, fields, menus, and the picture.
- Right button: scroll a window. Top half of the window = up, bottom half = down.
- The scroll bar at the right: see The scroll bar.

**Keys**

| Key | Does |
|---|---|
| Tab | next text field. Typing into the page: the page's next box (in the address box: out of it, to the page) |
| Return | send the form. Typing into the page: Return for the page, and its new picture |
| Backspace / Delete | delete the last character (typing into the page: on the page) |
| Ctrl-U | clear the field |
| Esc | stop loading (or leave the field) |
| Space / b | scroll a window down / up (when no field has the keyboard; typing into the page: and no box on the page) |
| Ctrl-Q | quit |

See Typing into the page for the keys that go to the web page.

**On the 3B1**, its own keys and the window's frame work too:

| Key or frame part | Does |
|---|---|
| Exit, Cancl, the close box | quit |
| Page, Next / Shift+Page, Prev | scroll a window down / up |
| ↓ / ↑, the scroll arrows (also FreeBee's mouse wheel) | scroll a quarter of the window down / up. The frame's scroll arrows are the window's own scroll bar: Archaic draws none on the 3B1 |
| Rfrsh, Clear | draw the window again |
| the resize box | a new window size. A new width: WRP makes the picture again, as wide (see A new window size) |

**On the DMD 5620**, in a window of layers:

| Key or button | Does |
|---|---|
| button 1 | click |
| button 2 | as the right button: scroll a window. Top half of the window = up, bottom half = down |
| button 3 | the menu of layers: New, Reshape, Move, Top, Bottom, Current, Delete, Exit |
| ↑ / ↓ | scroll a quarter of the window up / down |
| Space / b | scroll a window down / up (when no box has the keyboard) |
| the scroll bar at the right | Archaic's own (see The scroll bar). The elevator follows the pointer while button 1 is down |
| Reshape in the layers menu | a new window size (see A new window size) |
| Ctrl-Q | quit: the layer is a terminal again |

**In SunView**, the Sun keyboard's keys and SunView's frame work too:

| Key or frame part | Does |
|---|---|
| PgDn (R15), PgUp (R9) | scroll a window down / up |
| ↓ / ↑ | scroll a quarter of the window down / up |
| Home (R7), End (R13) | go to the top / the end of the page |
| the scroll bar at the left | SunView's own: left button down, right button up, middle button to that place. Archaic draws none in SunView |
| the frame's menu (right button on the frame): Quit | quit |
| the frame's menu: Resize (Zoom, FullScreen ...) | a new window size (see A new window size) |

The Stop key (L1) does nothing: SunView sends no event for it. Esc stops a load.

**On macOS (Cocoa) and with SDL:** Page Up and Page Down scroll a window.
The up and down arrows and each step of the mouse wheel scroll a quarter of it.
A trackpad scrolls smoothly, by pixels. Ctrl+click is the right button.
Cmd-Q and the window's close button quit.
Cocoa has the Mac's own scroll bar; SDL has Archaic's (see Scrolling).

**With X11** (HP-UX, DG/UX, AIX, Ultrix, Linux, XQuartz ...): Page Up and Page Down scroll a window
(on an HP keyboard: Prev and Next; on DEC's LK201: Prev Screen and Next Screen). The up and down arrows and each step of the mouse wheel
scroll a quarter of it. Close in the window manager's menu quits. The scroll bar is Archaic's.

**On Windows NT:** Page Up and Page Down scroll a window. The up and down arrows and each step of the
mouse wheel (NT 4.0) scroll a quarter of it; Home and End go to the top and the end. The scroll bar is NT's own:
its arrows and gray parts step while the button is down, and its box can be dragged. The close box quits.

**On Mac OS 6 and 7.5, and on A/UX**, its own keys, menus and window parts work too:

| Key or part | Does |
|---|---|
| Cmd-Q, File > Quit, the close box | quit |
| Cmd-. | as Esc: stop loading |
| Cmd-click | as the right button: scroll a window |
| Page Up, Page Down / ↑ ↓ / Home, End (Extended Keyboard) | scroll a window / a quarter of it / go to the top, the end |
| Enter | as Return |
| Edit > Paste (Cmd-V) | type the clipboard's text |
| the scroll bar | the Mac's own: its arrows and gray parts step while the button is down, its box can be dragged |
| the grow box, the zoom box | a new window size (see A new window size) |
| the Apple menu | About Archaic, and the desk accessories |
| the Application menu (System 7), a click in another program's window | another program is in front. Archaic goes on loading the page behind it |
| the Finder's Shut Down or Restart | quit (the Quit Apple event, System 7; not on A/UX: its Toolbox has no Apple events) |

Control and a letter is its control code, as on Unix: Ctrl-U, Ctrl-Q. A keyboard with no Control key
(the Mac Plus): Cmd-Q quits.

On the 3B1, the help line at the bottom of the screen lists the main keys.

## Typing into the page

With a WRP that has `kb` in its form (`keys.go`, WRP 4.14.0 or newer), the keys go straight
to the web page as you type them. K, ⌫ and ↵ are gone.

- Many pages have a box ready (a search box, a login name). Else click the box in the picture.
- Letters, Backspace and Tab go to WRP's `/keys`, with no new picture: the page has them at once.
  Keys typed while a request is out wait, and go in one group after it.
- The status line shows the end of the box's text, as WRP says it is, and the keys typed since:
  `Box: archaic browser_`. A password is stars: `Password: ******_`.
  Keys with no box (shortcuts): `Keys to the page: k_`. After Tab, stars until WRP says which box it is.
- Return goes with WRP's form, with the keys not sent yet (`k=ab%0D`): WRP presses Return,
  and the new picture comes.
- Space and b scroll Archaic's frame while no box on the page has the keyboard
  and no keys wait. In a box, they are letters.
- The arrows, Page Up, Page Down, the wheel, Esc and Ctrl-Q stay in Archaic.
  Ctrl-U, Home, End and the other control keys do not go to the page.
- The address box is Archaic's: click it to type an address, Return loads the page.
  Tab, Esc or a click elsewhere: out of it. After a page loads, the keys go to the page.
- A key that starts a new page (a shortcut, a link): WRP's answer says so, and Archaic gets its picture (St).
- A click, or a button, sends the keys that wait first.
- After 3 s with no keys (`-r n`: n s, `-r 0`: never), WRP makes the picture again (St, the page is not
  loaded again). It shows the typed text, and what the page did with it, like a list of suggestions
  that you can click. The status line keeps the box's text. On the Mac with Google's page, WRP made
  it in less than 1 s (a slice of 20 KB in 256 colors).
- While Archaic waits, a bar beside the box's text on the status line has a block for each second left
  (10 at most). They go out one at a time, and at 0 the picture comes. Each key fills the bar again.
- Not in a password box (the picture would show only dots). Not on the 3B1, Idris and the DMD 5620:
  Archaic has no clock there (`g_ms`), and they are the slowest systems.
- A new picture that takes longer than the pause (`-r`) turns them off for the run:
  `The picture took 7 s: no new pictures while you type`. So a slow computer or line gets none.
- Keys typed while it comes wait, and go after it.
- Letters that are not ASCII go as UTF-8 (WRP reads them so; K sends them as Latin-1, and WRP gets `?`).
- WRP logs how many keys came, not the keys: they may be a password.

Measured on the Mac with WRP (the time of each request, as WRP's log shows it):
keys with no new picture take about 0.25 s, most of it WRP's wait for a new page that may come
(as for a click). The same keys with a new picture (as K and ↵ send them) took 0.6 to 1.1 s on Google's page,
and the picture was 7 to 14 KB in black and white.

**/keys**, the question and the answer (plain text, a line each):

```
GET /keys?k=ab

focus text
value archaic ab
```

| Line | Means |
|---|---|
| `focus` | what has the keyboard on the page now: `text` (a box for text), `password` or `none` |
| `value` | the box's text, its last 200 letters, in Latin-1 (`?` for the other letters). A password as stars. Not with `none` |
| `page new` | the keys started a new page: get its picture |

- `k` is the keys in UTF-8, as WRP's `k`: `%08` Backspace, `%09` Tab.
- WRP's form has `<INPUT TYPE="HIDDEN" NAME="kb" VALUE="text">`: what had the keyboard when WRP
  made the picture. Its being there says that this WRP takes keys at `/keys`.
  Other browsers send it back with the form: WRP does not use it.
- A frame of another site can't be looked into: when it has the keyboard, WRP says `text`.

## The status line

**The status line** at the bottom of the window says what Archaic does, as it does it.
While Archaic loads what you asked for, a **spinner** turns at its left end: 8 dots in a circle.
Archaic's logo, the candle in the top right corner, burns meanwhile (see WRP's form).
It stops when the page is done (or stopped). It does not turn for the slices that Archaic gets
ahead, while you read.
Archaic's version is at the right end of the status line, like `Archaic v0.10`. The progress bar is left of it.
In a window less than 4 times as wide as the version, the version is not shown.

| Status line | Means |
|---|---|
| `Page: connecting to 127.0.0.1 port 8080` | Archaic connects to WRP |
| `Page: waiting for 127.0.0.1, 3 s` | the request went out, and WRP makes the page. The seconds count up |
| `Page: 2 KB` | the page comes |
| `Picture 1 of 2 (GIF): 12 of 31 KB` | a picture comes, and its type (GIF, ABM, ABM3, AB8). The bar at the right shows how much has come |
| `Picture part 3 of 9 (ABM3): 4 of 7 KB` | a part (a slice) of the page's picture comes, for the frame |
| `Picture part 4 of 9 (ahead, ABM3): 2 of 7 KB` | the next part comes, before you scroll to it |
| `Picture test 1 of 2 (ABM): 3 of 8 KB` | the picture test at the start: a test picture comes, first in the fast type, then in the small one (see The automatic choice) |
| `Picture test: ABM3 is used (2.5 s, ABM 4.3 s)` | the test's result: the type from now on, and the time of each one |
| `Picture part 3 of 9: ABM3, 5 KB in 2.7 s, 1.9 KB/s` | a slice has come: its type, its size, the time from the answer's header to its last row, and the speed that makes. It stays until the next slice or page. (While you type into the page, the box's line stays.) |
| `2 s, 45 KB, AB8  http://...` | the page is done: the time it took, how much came, and the type of its pictures (GIF, ABM, ABM3, AB8) |
| `Box: archaic browser_` and a bar of blocks | keys typed into the page: the box's text, and a block for each second until WRP's new picture (see Typing into the page) |

The bar shows when the server sends the size (WRP does for its pictures).
The spinner turns 8 steps a second by the display's clock (`g_ms`).
The 3B1 and Idris displays have no clock: there it steps each time the network waits
(1/10 s on the 3B1, up to 1 s on Idris).
On the DMD 5620 the terminal turns the spinner and draws the bar itself, with its own clock:
they cost no bytes on the line. The status line there has no count of the bytes that came.
With `-v`, each status line also goes to standard error (`status:` lines to standard output with `-t`).

## When WRP has an error

WRP answers with a page that has its form. When WRP answers with no form, it is an error:
Archaic keeps WRP's page and shows the error on the status line, like
`WRP: Unable to find map /map/....map`. WRP's buttons (← back, ↻ reload) still work.

- "Unable to find map": WRP made a new picture since this one, and forgot the old one's map.
  Archaic then gets WRP's new picture at once (St, which does not load the page again)
  and says `The picture was old: here is WRP's new one. Click again.`
- While WRP makes the next page, a click on the picture does nothing: the picture is old,
  and its map is about to go. Wait for the spinner to stop, or press Esc (or click St) to stop.
- A picture or a file from WRP is not shown: the status line says what it was.
- No page at all (WRP did not answer at the start): a box in the middle of the window says
  `Unable to connect to WRP`, with WRP's address and the error, like `connection refused` or
  `no answer`. The status line says it too, and so does the terminal (standard error; not on the 3B1):

  ```
  archaic: unable to connect to WRP at 10.0.2.2:8080: no answer
  archaic: ensure WRP is up and running. To use another address: archaic HOST:PORT
  ```

  Start WRP, then press Return: Archaic tries again. Or start Archaic with WRP's address.
- While Archaic connects to WRP with no page yet, the box says `Connecting to WRP` and the address
  (then `Waiting for WRP`, while WRP makes the page). Esc stops.
- With BSD sockets (`nsock.c`: Unix, macOS, SunOS, HP-UX, DG/UX, AIX, Ultrix, A/UX) the connect does not hold up the window:
  it is drawn, Esc stops, and after 20 s with no answer Archaic gives up (an address that
  does not exist on this network, like 10.0.2.2 outside an emulator kit).
  MacTCP (`nmactcp.c`, the classic Mac OS) does the same: Cmd-. stops. So does Winsock (`nwin.c`, Windows NT).
  On the 3B1 (`ntwg.c`) the connect waits as long as the system does.
- With a page, a failed connect is on the status line:
  `Unable to connect to WRP at 10.0.2.2:8080: connection refused. Ensure WRP is up and running.`

## A new window size

Make the window wider or narrower: once its size has stayed the same for 0.4 s,
Archaic asks WRP for the page again, as wide as the window. It sends WRP's form with St,
so WRP does not load the page again: the browser lays it out at the new width, and WRP
takes a new picture. The frame keeps its place on the page, in proportion
(a narrower page is longer). Text you typed in a box stays.

A new height alone needs no new picture: the frame gets taller or shorter.
On the 3B1, Idris and the DMD 5620 (no clock), the new picture is asked for at once.

## Scrolling: the whole page in slices

With a WRP that knows slices (`sl=1`), WRP makes a picture of the whole page once, and keeps it.

- Archaic shows the picture in a frame under WRP's form. Only the frame scrolls: the form stays at the top.
- The picture comes in **slices** as high as the frame. Archaic keeps 4 of them:
  the ones on the screen, and the next one above and below.
  It gets the next ones while you read, so most scrolls do not wait for the network.
- A click on the picture goes to WRP as a place on the whole page.
- After a click or keys on the same page, the frame stays where it was. A new page starts at its top.
- WRP leaves out its Up and Dn buttons.
- A slice in 2 colors is about 5 to 9 KB for 632 x 351 pixels as GIF, and about 10% less as ABM.
  In 256 colors it is bigger: about 20 to 100 KB.
- WRP 4.14.1 and newer make the colors and the dither of a slice when Archaic asks for it,
  and the rest of the page in the background. So the first slice of a long page comes at once:
  for a page of 79,499 rows, in 4 ms (2d) to 81 ms (256d) on an M1 Ultra.

| Input | Scrolls the frame |
|---|---|
| a step of the wheel, ↑ ↓, the 3B1's scroll arrows and Roll keys | a quarter of the frame |
| Page Up, Page Down, Space, b, the right button, the 3B1's Page, Next, Prev | the frame, less 2 text lines |
| a trackpad (Cocoa, SDL) | by pixels |

A scroll does not flicker: no pixel is made white first. Where the display can (`g_copy`:
X11, and the screen in memory of Cocoa, SDL and headless), the pixels on the screen move,
and Archaic draws only the rows that come in. X11 moves them with XCopyArea; when a part of the
window is covered (the server then sends GraphicsExpose), Archaic draws all of the frame again.
The 3B1, Idris (X10) and SunView draw all of the frame again for each scroll.
On the DMD 5620 the terminal has the slices: it moves its own pixels and draws the new rows itself.
A scroll is about 40 bytes on the line.

An older WRP ignores `sl=1`: then the picture fills the window, and WRP's Up and Dn buttons scroll the page.

## The scroll bar

A system that has scroll bars uses its own. Where it has none, Archaic draws one.

| Display | Scroll bar |
|---|---|
| Cocoa (macOS) | the Mac's own (NSScroller), always shown. A click in its track scrolls as Page Down or Page Up, or jumps to that place, as set in System Settings (with Option, the other one) |
| the 3B1 | the window frame's scroll arrows |
| Mac OS 6 and 7.5 | the Mac's own (a scroll bar control): see Mouse and keys |
| Windows NT | NT's own (a scroll bar control): see Mouse and keys |
| SDL, X11, X10 (the Atari ST), the DMD 5620, headless | Archaic's own, in the style of the Atari ST's GEM |

Archaic's own scroll bar is at the right of the window, 17 pixels wide (`screenshots/scrollbar.png`):

| Part | Does |
|---|---|
| ⇧ and ⇩ arrow boxes | scroll a quarter of the frame. Hold the button down: again every 1/10 s, after 4/10 s |
| the dotted shaft, above or below the elevator | scroll as Page Up or Page Down. Held down, it goes on until the elevator gets to the pointer |
| the white elevator | drag it. Its length shows how much of the page is in the window, its place where |

On X10 the elevator moves when the button is let go (as in GEM), and held parts do not repeat:
X10 has no events for the mouse moving with a button down, and can't wait for a time.

On the DMD 5620 the bar costs few bytes: the dotted shaft goes down the line as a pattern (4 rows),
and only the part of the bar that changed is drawn again. While a part of the bar is held down,
Archaic gets no new slice (one takes 10 s and more there): it comes when the button is let go.

## Updates

Archaic updates itself, through WRP. So a kit with Archaic in it needs no new disk for a new Archaic.
The old systems have no HTTPS: WRP gets the file from GitHub for them.

```
Archaic --HTTP--> WRP, /archaic-update --HTTPS--> GitHub: the newest release of tenox7/archaic
```

- Once a run, when WRP's first page is here, Archaic asks WRP for the newest Archaic for its system.
  The question is quiet: when there is no newer Archaic, the status line does not change.
- A newer one, and Archaic can write the folder of its program (root can): Archaic installs it.
  The status line says `Archaic upgraded to 0.9. Start archaic again to use it: /usr/local/bin/archaic`.
  The program that runs stays as it is, until you quit it.
- Archaic can't write the folder: `Archaic 0.9 is out. To upgrade, start archaic as root`.
- These lines also go to the terminal (standard error).
- A click while Archaic asks: the question stops, and comes again later (3 times at most).
  Esc stops it for this run.
- `archaic -u`: update now, with no window, then quit. Exit status 0: installed, or this is the newest.
- `archaic -n`: no updates. With `-t` or `-s`, Archaic asks for none.
- Versions compare as numbers, part by part: 0.10 is newer than 0.9. Archaic installs only a newer one.

**How Archaic installs it** (`update.c`):

1. It finds its own file: the name it was started with (if it has a `/`), else the first `archaic` on `$PATH`,
   else `/usr/local/bin/archaic` (on Idris `/usr/X/bin/archaic`).
2. If that file already is the new version (another Archaic installed it), Archaic says so, and gets nothing.
   Else it writes the new file into the same folder, as `archaic.PID` (13 letters at most: old file systems take 14).
3. It reads the file again, and checks its size and CRC-32 against WRP's answer.
4. It makes the file executable (755). The old program stays as `archaic.old`.
5. It puts the new file in the place of the old one, with `rename()`. Idris, the 3B1 and the 3B2 have no `rename()`
   (`NORENAME` in `archaic.h`): there it is `unlink()`, then `link()`, and only if `archaic.old` was made.
   If `link()` fails, `archaic.old` goes back.

A broken file, a full disk or Esc: the old program stays, and no `archaic.PID` stays.

**WRP's side** (`archaic.go` in WRP 4.13.0 or newer):

- `wrp -archaic URL`: the release, as a GitHub API address. Default:
  `https://api.github.com/repos/tenox7/archaic/releases/latest`. `-archaic ""`: no updates.
- WRP keeps the release's list for 15 minutes (after an error, 2 minutes), and each file it got,
  until the release changes that file. A file that is not good: WRP keeps why for 2 minutes.
- WRP checks each file: its size and SHA-256 must be the release's (GitHub's `digest`),
  and its marks (see below) must name the system and a version.

The question and the answer are plain text, a line each:

```
GET /archaic-update?p=3b1&v=0.8

archaic-update 1
version 0.9
file archaic-3b1
size 67027
crc 1f2e3d4c
get archaic-update?p=3b1&f=0.9
```

- The `get` line gives the file (`application/octet-stream`). A version that is not the newest gets 404.
- With no file for the system, WRP sends `note` and why, with no `version`:
  `note no archaic-dgux in release v0.9`. Archaic then says `WRP has no update: no archaic-dgux in release v0.9`.
- An older WRP (before 4.13.0) sends its page. Archaic then says `This WRP has no updates for Archaic: it is an older WRP`.

**The marks.** Each program carries 2 strings, for WRP. `what` and `strings` show them too,
and `archaic --version` says them as `Archaic v0.9 for 3b1`:

| Mark | Set in `archaic.h` by | Like |
|---|---|---|
| `@(#)Archaic VERSION` | `IDENT` | `@(#)Archaic 0.9` |
| `@(#)Archaic for SYSTEM` | `SYSID` | `@(#)Archaic for 3b1` |

WRP reads the marks in the file, not the release's tag:

- A release also has older files, for the systems not built again. WRP answers with the version in the file's marks,
  and Archaic installs the file only if that version is newer than its own. So an old file in a new release
  can't start an update loop.
- WRP does not offer a file with the mark of another system (a file put in the wrong place in the release).
- WRP does not offer a file with no marks (Archaic 0.8 and older).

**System names.** `archaic.h` sets `SYSID` from the names that the compiler defines.
The release's file is `archaic-` and the system name; on the Atari (TOS, FreeMiNT) also `.prg`:
`archaic-tos.prg`, `archaic-mint.prg`.

| System | Compiler defines | Name |
|---|---|---|
| Idris (Atari ST) | `_WSLAS` | `idris` |
| AT&T UNIX PC (3B1) | `unixpc` | `3b1` (Archaic 0.14 and older: `unixpc`, which WRP still takes) |
| AT&T 3B2 (with a DMD 5620) | `u3b2` | `3b2` |
| SunOS 4 (Sun-4) | `SUNVIEW` (mksunview), `sparc` | `sun4` |
| SunOS 3.5 (Sun-3) | `SUNVIEW` (mksunview), `mc68020` | `sun3` |
| HP-UX 9 | `__hpux` | `hpux9` |
| DG/UX | `__DGUX__` | `dgux` |
| AIX 3.2 | `_AIX`, `_IBMR2` | `aix3` |
| AIX PS/2 1.3 | `_AIX`, `ps2` | `aixps2` |
| Ultrix 4 | `ultrix` | `ultrix4` |
| A/UX 3 | `AUX` (mkaux) | `aux3` |
| FreeMiNT (Atari, ARAnyM) | `__MINT__` (m68k-atari-mint gcc) | `mint` |
| TOS (Atari ST, STE, TT, Falcon) | `ATARITOS` (mktos) | `tos` |
| Windows NT on x86, MIPS, Alpha, PowerPC | `_WIN32`, and `_M_IX86`, `_M_MRX000`, `_M_ALPHA`, `_M_PPC` | `ntx86`, `ntmips`, `ntalpha`, `ntppc` |

A build with no `SYSID` gets no updates: macOS, Linux, the tests, the classic Mac OS (for now). For a new port:
`cc -DSYSID='"@(#)Archaic for NAME"'`, or a line in `archaic.h`.

**A first install, with curl or wget.** WRP also gives the release's files as they are
(older builds too, with no marks): `/archaic-download/archaic-NAME` (or `/archaic-download/NAME`).
`/archaic-download` alone lists the files, with their sizes.

```
curl -O http://HOST:8080/archaic-download/archaic-hpux9
chmod 755 archaic-hpux9
mv archaic-hpux9 /usr/local/bin/archaic
```

WRP checks each file's size and SHA-256, and does not give a file with the mark of another system.
An error is a short text with HTTP 404: `curl -f` does not save it.

**For a release:**

- Change `IDENT` in `archaic.h`. Then build the systems again, or keep their old files:
  WRP offers only a file that is newer.
- Keep the files' names: `archaic-NAME`, on the Atari `archaic-NAME.prg`, on Windows NT `archaic-NAME.exe` (`archaic-ntx86.exe` ...).
- The first version with updates must go into each kit once. After that, a kit's Archaic updates itself.

**Trust.** Archaic and WRP talk plain HTTP, as for the pages. WRP gets the file from GitHub over HTTPS,
and checks it against the release's SHA-256. The CRC-32 finds a broken download, not an attack.
Use a WRP that you trust, on your own network.

## Building and testing on Unix or macOS

```
make              # ./archaic, X11 + BSD sockets
make cocoa        # ./archaic-cocoa, a macOS window
make sdl          # ./archaic-sdl, SDL 2
make headless     # ./archaic-headless, no window: for tests and automation
make archaic-asan # X11 with address and undefined-behaviour checks
make knr          # syntax check as a pre-ANSI compiler sees it (-DNOPROTO), also main.c with -DREMOTE
make test         # sh test/run.sh
```

- `make` needs X11 (on macOS: XQuartz in `/opt/X11`).
- On AIX 3, AIX PS/2, HP-UX, DG/UX, Ultrix, SunOS and A/UX, `make` (or `gmake`) runs the system's script in `src`
  (see each system's file in `ports/`), and moves `archaic` up (on A/UX also `%archaic`). `CC` goes to the scripts that take it
  (`mkaix`, `mkaixps2`, `mkultrix`): `CC=gcc gmake`. SunOS 3: put `/usr/5bin` (its `uname`) on the PATH.
- `make cocoa` needs only the Xcode command line tools.
- `make sdl` needs SDL 2: `sdl2-config` on the PATH (`brew install sdl2`).
- `make headless` needs only a C compiler.
- The tests need ImageMagick.

Cocoa, SDL and headless draw the same pixels, with the X11 6x13 font.
On a Retina screen, Cocoa and SDL show each pixel as 2x2.

### No window: archaic-headless

`archaic-headless` reads commands from standard input, one per line.
It reads the next command only when Archaic has nothing else to do,
so a command that loads a page ends before the next one starts.
At the end of the input, Archaic quits.

| Command | Does |
|---|---|
| `click X Y [B]` | press mouse button B (1 left, 2 middle, 3 right) at X, Y, and let it go |
| `down X Y [B]`, `up X Y [B]` | press the button and hold it down; let it go |
| `move X Y` | move the mouse, with the button down (to drag the scroll bar's elevator) |
| `key K` | a character, or `return`, `tab`, `esc`, `backspace`, `delete`, `space`, `pgup`, `pgdn`, `up`, `down`, `ctrl-X` |
| `type TEXT` | the rest of the line, as keys |
| `scroll up`, `scroll down` | a window, as Page Up and Page Down |
| `wheel N` | N steps of the mouse wheel (a quarter of the window each); down if N > 0, up if N < 0 |
| `pixels N` | N pixels, as a trackpad; down if N > 0, up if N < 0 |
| `size W H` | the window is now W x H |
| `shot FILE` | write the screen to FILE: PNG (in colors if the screen has them), PBM if the name ends in `.pbm`, PPM if `.ppm` |
| `sleep MS` | wait MS milliseconds. While a part of the scroll bar is held down, the time is counted in steps of 1/10 s and not waited, so a held arrow repeats the same way in every run |
| `quit` | quit |

Empty lines and lines that start with `#` do nothing. Standard output gets
`title TEXT` for each page, and `shot FILE WxH` for each screenshot.
With `ARCHAIC_COLORS=N` set, the display says it shows N colors (256: as an 8-bit colormap).

```
printf '%s\n' 'click 20 12' 'type https://en.wikipedia.org/wiki/AT%26T_UNIX_PC' \
    'key return' 'shot page.png' 'scroll down' 'shot down.png' |
    ./archaic-headless -v -g 800x600 127.0.0.1:8080
```

`test/run.sh` checks `--help` first: a line for each option of the usage line, at most 79 characters each. Then:

1. GIF decoding gives **exactly** the same pixels as ImageMagick.
   ABM and ABM3 pictures give exactly the pixels of the GIF. A broken ABM is refused,
   and an ABM3 that was cut shows the rows that came.
   What Archaic asks for (its `Accept`): the fast types, and the others with `q=0.5`; AB8 only with colors;
   one type with `-f`.
   The automatic choice (1d): pages of 8 pictures from the test server, which sends the pictures of one kind
   some time after their header. The slower kind is tried for 2 pictures, and again after 8 pictures;
   the faster kind takes over, also in colors. The status line says how a trial ended, and names the type
   of each picture that comes. None with `-f`.
   The picture test (7a, with the headless display): both kinds come and show, the faster one is used,
   and WRP gets the start and the result as notes. None with `-f` or an older WRP.
2. Gray and color GIFs decode at the right size.
   With colors (`-c 256`), the colors are **exactly** ImageMagick's. Also in the hard cases
   of the 8-bit way: a full code table with no clear, long runs, and an interlaced GIF (the way before).
   AB8 pictures give exactly the colors of the GIF. Not on a black and white display;
   a broken one is refused.
3. HTML: title, entities, fields, menus, hidden fields, buttons, links, comments.
4. A button click sends the form with the right query.
5. WRP's form: W, H, M, T and C are hidden, and Archaic sends its own values (and with `-a`, the menus). WRP's 7 buttons and the labels Z, C, D and K are pictures, Go stays text.
   The url field fills its line, up to Archaic's logo: in `-t` the line ends at 615. A new choice in WRP's D menu sends the form at once.
   With colors: C shows, set to the colors (for 100 colors: 64); another choice sends St at once;
   the start page request has WRP's settings.
   WRP's one menu (5c): with colors, C is set to the first line of the nearest number (256d, 216n,
   16d for 100 colors); with millions (headless), C stays at WRP's line (216n, or 256d);
   a new line sends St at once, and the next page keeps it.
   In black and white, C has only 2p, 2d and 2n, is set to 2p, and its label is the dither pattern.
   Text in K, then ↵ or ⌫: the text and the key go as keys (`k=ab%0D`), with no button.
   The address box's clear button empties it.
   With `kb` in WRP's form: K, its label, Bs and Rt are gone; each key goes to `/keys`, Return with
   the form (`k=%0D`); the status line shows the box, a password as stars; Space and b scroll with
   no box; `page new` gets St at once; Latin-1 goes as UTF-8; Tab takes the keys out of the address box.
   WRP's error (an answer with no form, after a click on the picture): WRP's page stays, the error
   is on the status line, and St goes at once. With no page (no server), Return tries again,
   and the terminal says that WRP is not there.
6. With WRP running (default `127.0.0.1:8089`): the start page, then a page with a window-wide picture.
7. `archaic-headless`: PNG and PBM screenshots (the same pixels), typing into a form and sending it, a new size.
   A color picture on the screen is exactly the GIF's, and the PNG is in colors.
   Millions of colors ask WRP for its own (no `c`). A display of 256 colors (`ARCHAIC_COLORS=256`)
   asks WRP for 216, and one of 128 (as SunView) for 128.
   A new width: one St with the new width, once the size stays (none for a new height); typed text stays.
   No WRP: the box in the middle of the window. The window's title: `Archaic`, then the page's title;
   on WRP's page, the address, not WRP. Archaic's version at the right end of the status line:
   the same pixels at 640 x 400 and 500 x 300. Archaic's logo in the top right corner: the candle with no flame,
   the same pixels as frame 0 of `src/candle.h`.
   Typing into the page with `-r 3`, then 3 s with no keys: one St, and the status line shows the box again;
   none after 2 s, with `-r 0` or in a password box. A new picture slower than `-r`: no more of them.
   The bar beside the box's text: 3 blocks at 0.5 s, 1 at 2.5 s, none after the new picture.
8. The tall picture, with no WRP: the test server cuts a random 600 x 1500 GIF into slices.
   After a wheel step, a window and a scroll to the end, the frame must show exactly the right rows.
   Also: the next slice comes before it is needed, and a click sends the place on the whole page.
9. Archaic's scroll bar, on the same picture: the arrows, a click in the shaft, the elevator
   dragged (back to where it was, and past the end), an arrow held down for 1 s.
   In 10 shots, the frame must show the right rows, and the elevator must be at the right place.
   Clicks on the bar must not go to the page.
10. The status line: connecting, waiting (counted), the page in bytes, a part in KB, time and size.
11. Updates, with the test server as WRP: a build with the system name `test` (and the headless display).
    `--version` names the version, and the system of the `test` build.
    `-u` asks `archaic-update?p=test&v=VERSION`, installs 99.0 and keeps `archaic.old`; the new file runs.
    The file installed already (by another Archaic): no download. Started by its name alone,
    the search path finds it (`dir:dir`, and Idris's `|dir/|dir/`).
    The same version, a wrong CRC, a short file, WRP's note, an older WRP, a folder it can't write
    (not as root): nothing changes, and no file is left.
    In the window: installed once WRP's page is here; none with `-n` or `-t`.

Run WRP for the tests: `wrp -l 127.0.0.1:8089`.

To see the real window without a desktop, use Xvfb:

```
Xvfb :7 -screen 0 660x420x24 -listen tcp -nolisten local -nolisten unix &
PATH=/opt/X11/bin:$PATH sh test/xshot.sh 127.0.0.1:7 shot.png 1 '%%' 127.0.0.1:8089
```

`-screen 0 660x420x8` gives an 8-bit colormap (PseudoColor), as on old workstations.

## The systems

Each old system has a file in `ports/`: how to run Archaic there, how to build it,
and what to know (the compiler, the window system, the network, the tests).
Read the system's file before work on its port. Put the facts of one system in its file.

| System | File | Emulator |
|---|---|---|
| Idris on the Atari ST (X10) | [ports/idris.md](ports/idris.md) | Hatari |
| AT&T UNIX PC (3B1) | [ports/unixpc.md](ports/unixpc.md) | FreeBee |
| AT&T 3B2 with a DMD 5620 | [ports/3b2.md](ports/3b2.md) | Open SIMH, dmd5620 |
| SunOS 4 with SunView (Sun-4) | [ports/sun4.md](ports/sun4.md) | QEMU |
| SunOS 3.5 with SunView (Sun-3) | [ports/sun3.md](ports/sun3.md) | TME |
| HP-UX 9 (X11) | [ports/hpux9.md](ports/hpux9.md) | QEMU (LEMU) |
| DG/UX R4.20 (X11) | [ports/dgux.md](ports/dgux.md) | 86Box |
| AIX 3.2 (X11) | [ports/aix3.md](ports/aix3.md) | QEMU (LEMU) |
| AIX PS/2 1.3 (X11) | [ports/aixps2.md](ports/aixps2.md) | QEMU (LEMU) |
| Ultrix 4 (X11) | [ports/ultrix4.md](ports/ultrix4.md) | gxemul |
| Mac OS 6 and 7.5 (68k Mac) | [ports/mac.md](ports/mac.md) | Snow |
| FreeMiNT with GEM (Atari) | [ports/mint.md](ports/mint.md) | ARAnyM |
| TOS with GEM (ST, STE, TT, Falcon) | [ports/tos.md](ports/tos.md) | Hatari |
| A/UX 3 | [ports/aux3.md](ports/aux3.md) | QEMU (LEMU) |
| Windows NT 3.1 and newer (x86, MIPS, Alpha, PowerPC) | [ports/nt.md](ports/nt.md) | QEMU (LEMU) |

The next ports, for old systems with no web browser: [ports/roadmap.md](ports/roadmap.md).

## Porting to DOS, OS/2, Win16 and others

The core (`main.c`, `html.c`, `http.c`, `gif.c`, `pic.c`, `update.c`) does not change.
Give the port a system name for the updates (`SYSID`, see Updates).
A port is **one display file** and **one network file**.

**Display (`g_` functions)**

| Function | Does |
|---|---|
| `g_open(geom, &w, &h)` | open a window, give its size |
| `g_close()`, `g_title(s)`, `g_flush()` | |
| `g_fill(x, y, w, h, c)` | rectangle, c: 0 white, 1 black |
| `g_invert(x, y, w, h)` | swap black and white |
| `g_text(x, y, s, n, inv)` | n characters, x, y = top left |
| `g_textw(s, n)`, `g_font(&height, &base)` | font size |
| `g_bits(x, y, w, h, bits, bpl)` | 1-bit rows, 1 = black, high bit left |
| `g_colors()` | how many colors the display shows (a long): 2 for black and white, up to `1L << 24` |
| `g_pix(x, y, w, h, pix, bpl, pal)` | 8-bit rows, a color number for each pixel; pal: 256 colors, 3 bytes each (red, green, blue). Archaic calls it only if `g_colors()` is more than 2: a black and white port leaves it empty |
| `g_copy(x, y, w, h, dy)` | move x, y, w, h of the window dy rows down (up if dy < 0), for a scroll: Archaic then draws only the new rows. Returns 0 if it did not (a part was covered, or the port can't): then Archaic draws all of it. A port can always return 0 |
| `g_event(&e, wait)` | next key, click (`E_DOWN`, `E_UP`, and `E_MOVE` with a button down), expose, resize, quit, scroll (`E_SCROLL`: pixels, quarter windows, windows, or to a place). wait 0: don't wait; 1: wait for an event; 2: wait at most 1/10 s (or as 1) |
| `g_fd()` | the display's file descriptor, or -1 |
| `g_ms()` | a clock in milliseconds, from any start (for the spinner); -1 if the system has none |
| `g_bar` | a variable: 0 if the system has scroll bars (Archaic then calls `g_sbar`), 1 if Archaic must draw its own |
| `g_sbar(pos, max, vis)` | show the page's place in the system's scroll bar: pos, 0 up to max; vis pixels show. Empty where `g_bar` is 1. The scroll bar sends `E_SCROLL` (`S_TO` for its knob) |

**A display on a slow line** (`REMOTE`, as `dmd.c`: a terminal with a CPU of its own) keeps the page's
slices itself. It has these functions too, and `main.c` then keeps no pixels of the page:

| Function | Does |
|---|---|
| `g_slice(s, y0, im)` | pass the picture that comes now on to the display, for its slot s: the slice at page row y0. Fills `im`'s size and kind. 0: the display has it; -1 stopped or short; -2 not a type it takes; -3 it has no memory |
| `g_view(x, y, w, h, oy, a, b)` | the frame is at x, y, w x h, and its top row is page row oy: the display draws its rows a up to b - 1 again |
| `g_drop(s)` | slot s is free |
| `g_spin(on, x, y)` | the display turns the spinner at x, y itself, or takes it away |
| `g_gauge(x, y, w, h)` | for the next `g_slice`: the display shows how much has come, in a bar there |
| `g_hole(x, y, w, h)` | an open list is there: the display shows no rows of a slice that comes in that part. w 0: no hole |

**Network (`n_` functions)**

| Function | Does |
|---|---|
| `n_open(host, port, err)` | connect; on error, the reason in `err` (`connection refused`, `no answer` ...: Archaic adds the address). It may call `ui_poll()` while it waits, and stop when that returns 1 (Esc) |
| `n_read(h, buf, n, wait)` | bytes read; -2 nothing yet, 0 end, -1 error. wait 1: a short time (`nsock.c` 1/20 s), less when the display has input |
| `n_write(h, buf, n)`, `n_close(h)` | |

**C rules** (they are in `archaic.h` too):

- K&R function definitions. Prototypes only through `P()`. `-DNOPROTO` turns them off.
- No `const`, `enum`, `void *`, struct assignment, structs by value.
- Parameters only `int`, `long` or pointers.
- No arrays in local variables (Whitesmiths bug). Make them static.
- `unsigned char` for bytes. `long` for byte counts and pixel offsets (int may be 16 bits).
- Few library calls: malloc, free, sprintf, printf, fprintf, getenv, time.
  `update.c` also needs open, creat, read, write, close, access, chmod, link, unlink, getpid, getuid,
  and rename (or `NORENAME`).

**Memory note for 16-bit systems:** the static data is about 165 KB
(page 30 KB, items 50 KB, read buffer 16 KB, GIF tables 32 KB, ABM3's contexts and tables 19 KB).
On a display with colors, the GIF decoder also takes 16 KB from malloc, once.
On DOS or Win16, use the large memory model, or make these smaller in `archaic.h`.

## Known limits

- One page at a time. No history of its own: use WRP's back button (← arrow).
- Typing into the page: the picture shows the new text only after Return, a click, or a pause with no keys
  (`-r`; not on the 3B1, Idris and the DMD 5620). The status line shows it meanwhile, from the end of the box's text
  (the place of the caret is not known).
- A page over 30 KB of HTML is cut (WRP's pages are about 3 KB).
- On the 3B1 and the DMD 5620, the font has only ASCII: other letters in WRP's form show as `?`.
- On the DMD 5620 the line is slow (about 450 bytes a second): a screen of a page takes 10 to 30 s,
  and Archaic's part goes into the terminal at each start (25 s). A small window is faster.
- On the DMD 5620, WRP's page picture must be ABM or ABM3 (WRP 4.11.0 or newer).
  The terminal has no GIF decoder: with an older WRP there is no picture.
  Other pictures of a page (not WRP's) go as bits, 1 for each pixel: slow.
- On the DMD 5620 there is no new picture of the page while you type into it.
- In SunView, a second window with many colors (another Archaic) can change the colors of the first one,
  until the pointer is in it (see `ports/sun4.md`).
- In SunView, C at 216 shows some wrong colors, like green dots in gray text. The 216 web colors do not fit
  in its 128 places, and Archaic drops colors of the table, also ones that the picture uses
  (x11.c takes cells only for the colors that a picture uses). 128d, the default there, and 256d are right.
- With a WRP before November 2026, C at 216 is WRP's median cut: new colors on each page,
  and on an 8-bit X display almost all of the colormap (see 216 colors: GIP).
- WRP before commit 3126bc0 (November 2026) could hang for good on back (← arrow),
  when Chrome took the page from its back-forward cache. Use a newer WRP.
- With a WRP before 4.14.1, a very long page with the dither (like reddit) could time out:
  WRP made the colors of the whole page before it answered. Use a newer WRP.
- With slices, a link to a place on the same page (`#name`) does not scroll the frame there.
- With a WRP that has no `picfmt.go` (4.14.1 and older), there is no ABM3 and no AB8: black and white
  pictures come as ABM, and pictures in colors as GIFs. A 16 MHz 68030 needs 1.3 s for one 632 x 351
  slice as a GIF, and 0.4 s as AB8 (see Pictures: GIF, ABM, ABM3 and AB8).
- The picture test takes time at each start: two pictures as large as the frame. On a line of 19200 baud
  that is about 7 s in black and white. A key or a click stops it.
- Later, 2 pictures from time to time are of the kind that is slower here (a trial). The clock on Idris counts
  seconds: there a trial takes up to 6 pictures.
