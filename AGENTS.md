# Archaic: notes for agents

The details: how Archaic works with WRP, the facts of each old system, building, tests, porting.

README.md is for people. Keep it short: what Archaic is, screenshots, platforms, quick start, build.
Show each system once in README.md: its emulator's window, at the top.
Put new details here, not in README.md.

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
| `src/nsock.c` | network for BSD sockets (Unix, macOS, FreeMiNT, A/UX). On A/UX it waits in the Toolbox's `WaitNextEvent` (see A/UX 3: what to know) |
| `src/fetch`, `mkinc`, `mk`, `lka` | Idris scripts: get the sources, headers, compile, link |
| `src/heap.c` | Idris tool: sets how much memory a program file asks for. `lka` links with a copy of the linker that asks for more |
| `src/unixpc.c` | display for the AT&T UNIX PC (3B1) window system |
| `src/ntwg.c` | network for the 3B1: the BSD sockets of WIN/TCP (TWG). Also for the 3B2: WIN/3B |
| `src/mkunixpc` | 3B1 script: compile and link with gcc |
| `src/dmd.c` | display for the AT&T DMD 5620 terminal (the Blit) in layers, from a 3B2: commands to Archaic's part in the terminal. It keeps a copy of the screen, so that only what changes goes down the line (see The 3B2 and the DMD 5620: what to know) |
| `src/dmdterm.c` | Archaic's part in the DMD 5620: it draws, it keeps and decodes the page's slices (ABM, ABM3), it scrolls. Built with the DMD's compiler |
| `src/mk3b2`, `bin2c.c` | 3B2 script: compile and link with the 3B2's cc and `dmdcc`. `bin2c` puts the terminal's part into `archaic` as a C table (`dmdm.h`) |
| `src/sunview.c` | display for SunView (SunOS 3 and 4) |
| `src/mksunview` | SunOS 3 and 4 script: compile and link with Sun's cc |
| `src/mkhpux` | HP-UX 9 script: compile and link with HP's cc, for X11 (`x11.c`, `nsock.c`) |
| `src/mkdgux` | DG/UX R4.20 script: compile and link with DG/UX's cc, for X11 (`x11.c`, `nsock.c`) |
| `src/mkaix` | AIX 3.2 script: compile and link with IBM's XL C (or gcc), for X11 (`x11.c`, `nsock.c`) |
| `src/mkaixps2` | AIX PS/2 1.3 script: compile and link with MetaWare High C (or the kit's hcc), for X11 (`x11.c`, `nsock.c`) |
| `src/mkultrix` | Ultrix 4 script: compile and link with Ultrix's cc (or gcc), for X11 (`x11.c`, `nsock.c`) |
| `src/mac.c` | display for the classic Mac OS (System 6, Mac OS 7.5) on a 68k Mac: a QuickDraw window with the Mac's own scroll bar, and the Mac's `main()`. It shares the CPU with the other programs (see Multitasking on the classic Mac OS). Also the display for A/UX 3, through the A/UX Toolbox (`-DAUX`) |
| `src/nmactcp.c` | network for the classic Mac OS: MacTCP's TCP driver (on Mac OS 7.5: the MacTCP part of Open Transport) |
| `src/dnr.c`, `dnr.h` | the classic Mac OS: MacTCP's name resolver, for host names (from HTTPHead, a MacTCP sample for Snow). C99: only the Mac build compiles it |
| `src/mkmac`, `CMakeLists.txt`, `archaic.r` | the classic Mac OS: build with Retro68 in Docker, on your computer (`make mac`). `archaic.r`: the SIZE resource, with the multitasking flags |
| `src/gem.c` | display for GEM on the Atari (FreeMiNT with XaAES, as on ARAnyM): a GEM window with GEM's own scroll bar. It shows `fb.c`'s screen in memory through the VDI: in colors on a screen of 15 to 32 bits, else in black and white |
| `src/mkmint` | FreeMiNT: build with the m68k-atari-mint gcc in Docker, on your computer (`make mint`): `archaic-mint` |
| `src/mkaux`, `auxrsrc.r` | A/UX 3 script: compile and link with A/UX's cc and the A/UX Toolbox (libmac), for `mac.c` and `nsock.c`. `auxrsrc.r`: the SIZE resource, which `rez` puts in `%archaic`, the resource file next to the program |
| `tools/idris.py` | host tool: run commands in Idris over telnet, serve files |
| `tools/hget.c` | Idris tool: download one file over HTTP (through netd) |
| `tools/slowline.py` | host tool: a TCP proxy that is a slow line (bytes a second), to try Archaic on a slow line with WRP |
| `tools/pcf2c.py` | host tool: an X11 PCF font as a C table (it made `font6x13.h`) |
| `tools/dmdlog.py` | host tool for the DMD 5620 port: decodes a log of the bytes that Archaic sent to the terminal (`ARCHAICLOG`), a command a line, and draws the screen they make |
| `test/run.sh` | tests: GIF decoder, HTML, forms, WRP, the headless display |
| `test/xshot.sh` | test: run the X11 build and take a screenshot |
| `test/data/` | GIFs saved from WRP, for the tests; two of them also as ABM and ABM3 (`.ab3`), two as AB8. `full.gif`, `runs.gif` and `ilace.gif` are made for the 8-bit way: a full code table with no clear, long runs, interlaced. `dith` is noise in 8 colors: smaller as a GIF than as AB8. WRP makes the `.ab3` and `.ab8` files from the GIFs: `WRP_SAMPLES=dir go test -run TestSamples` |
| `test/cfacts.c`, `cfacts2.c` | probe for a new port: what its C compiler and linker do (name length, case, struct copies) |
| `screenshots/` | the pictures in README.md: the emulators' windows, the scroll bar, and WRP's buttons and labels in `icons/` (drawn from the pictures in `main.c`, as `drawbutton()` draws them, at their real size). Also each system's own screen, for this file (see each Running section) |

## Options

```
archaic [=WxH+X+Y] [-a] [-c n] [-f type] [-t] [-v] [-o file] [-s script] [-r n] [-u] [-n] [--version] [--help] [url]
```

| Option | Meaning |
|---|---|
| `url` | WRP's address, like `10.0.2.2:8080`. Default: `$WRP`, else port 8080 on the host (Idris, the 3B1, the 3B2, SunView, HP-UX, DG/UX, AIX and FreeMiNT: `10.0.2.2`, A/UX: `192.168.1.10`, other Unix and the classic Mac OS: `127.0.0.1`). Ultrix in gxemul and Mac OS 7.5 in Snow need WRP's address: see Running on Ultrix 4 and Running on Mac OS 7.5 |
| `=WxH+X+Y` | window size and place (also `-g WxH+X+Y`). Default on Idris and SunView: about 9/10 of the screen, in the middle. On the 3B1: 686 x 248 inside a frame; a larger size gets no frame, up to 720 x 288 (`-g 720x288`). On the classic Mac OS and A/UX: the screen under the menu bar (on System 7 and A/UX, less the column of disk icons at the right). On the DMD 5620: the layer as it is (Reshape in the layers menu changes it) |
| `-a` | also show WRP's M, T and C menus, and send them as they are set |
| `-c n` | start with pictures of n colors (2 to 256), not as many as the display shows (see Colors). With `-t`: as if the display showed n colors |
| `-f type` | ask WRP for one type of pictures, with no choice. `gif`: GIFs only. `abm`: ABM, and GIFs in colors (as Archaic 0.11). `abm3`: ABM3. `ab8`: AB8, and ABM. `small`: for each picture the smallest type. Without `-f`, Archaic times its pictures and asks for the kind that is faster here (see Pictures: GIF, ABM, ABM3 and AB8) |
| `-t` | test mode: no window. Prints the page items, and for each picture what it came as (`gif`, `abm`, `abm3`, `ab8`) and a checksum |
| `-v` | status messages also go to stderr, and the times of each trial of the picture types |
| `-o file` | at the end, save the first picture: PBM, or PPM if it has colors |
| `-s script` | play a script, then quit (see Scripts, in Building in Idris) |
| `-r n` | typing into the page: WRP's picture again after n seconds with no keys. Default 3, 0 never. Not on the 3B1, Idris and the DMD 5620 (no clock). A new picture that takes longer than n seconds turns it off for the run (see Typing into the page) |
| `-u` | update: ask WRP for a newer Archaic and install it, with no window, then quit (see Updates) |
| `-n` | no updates. Without it, Archaic asks WRP once a run (not with `-t` or `-s`) |
| `--version` | print the version, then quit: `Archaic v0.9`. A release build also names its system (see Updates): `Archaic v0.9 for aix3` |
| `--help` | print all the options, a line each, then quit |

The classic Mac OS has no command line: Archaic reads its options from the file `Archaic Options`
(see Running on Mac OS 6).

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
Right of them, in the top right corner, is Archaic's logo: for now a white A in a black box, 19 x 19 pixels.
A line with no room for the logo (27 pixels with its margins) has none, and the box gets the room that is left.

**C** is one menu for the colors and the dither (WRP 4.12.0 or newer).
Each line is a number of colors and a letter:

| Letter | The picture |
|---|---|
| d | dithered (Floyd-Steinberg): all of the page. Photos and shades keep their tones. In 2 colors, text in a color, gray text and text on a color become dots |
| n | no dither: each pixel gets the nearest color. Photos get bands of color. In 2 colors, the smallest pictures |
| p | only in 2 colors, for reading: only photos are dithered. Text and lines are sharp, and the pictures are about 20% smaller than 2d |

The lines are 256d, 256n, 216n, 216d, 128d, 128n, 16d, 16n, 2p, 2d and 2n.
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
| `archaic-headless` | millions, like SDL (`-c 2` for black and white). `ARCHAIC_COLORS=N`: as a display of N colors (for the tests) |

On a display with colors:

- WRP's **C** menu shows. It is set to the display's colors: the first line of the nearest of WRP's
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
  MacTCP (`nmactcp.c`, the classic Mac OS) does the same: Cmd-. stops.
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
GET /archaic-update?p=unixpc&v=0.8

archaic-update 1
version 0.9
file archaic-unixpc
size 67027
crc 1f2e3d4c
get archaic-update?p=unixpc&f=0.9
```

- The `get` line gives the file (`application/octet-stream`). A version that is not the newest gets 404.
- With no file for the system, WRP sends `note` and why, with no `version`:
  `note no archaic-dgux in release v0.9`. Archaic then says `WRP has no update: no archaic-dgux in release v0.9`.
- An older WRP (before 4.13.0) sends its page. Archaic then says `This WRP has no updates for Archaic: it is an older WRP`.

**The marks.** Each program carries 2 strings, for WRP. `what` and `strings` show them too,
and `archaic --version` says them as `Archaic v0.9 for unixpc`:

| Mark | Set in `archaic.h` by | Like |
|---|---|---|
| `@(#)Archaic VERSION` | `IDENT` | `@(#)Archaic 0.9` |
| `@(#)Archaic for SYSTEM` | `SYSID` | `@(#)Archaic for unixpc` |

WRP reads the marks in the file, not the release's tag:

- A release also has older files, for the systems not built again. WRP answers with the version in the file's marks,
  and Archaic installs the file only if that version is newer than its own. So an old file in a new release
  can't start an update loop.
- WRP does not offer a file with the mark of another system (a file put in the wrong place in the release).
- WRP does not offer a file with no marks (Archaic 0.8 and older).

**System names.** `archaic.h` sets `SYSID` from the names that the compiler defines.
The release's file is `archaic-` and the system name.

| System | Compiler defines | Name |
|---|---|---|
| Idris (Atari ST) | `_WSLAS` | `idris` |
| AT&T UNIX PC (3B1) | `unixpc` | `unixpc` |
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
- Keep the files' names (`archaic-NAME`).
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
  (see each Building section), and moves `archaic` up (on A/UX also `%archaic`). `CC` goes to the scripts that take it
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
   The url field fills its line, up to Archaic's logo: in `-t` the line ends at 609. A new choice in WRP's D menu sends the form at once.
   With colors: C shows, set to the colors (for 100 colors: 64); another choice sends St at once;
   the start page request has WRP's settings.
   WRP's one menu (5c): with colors, C is set to the first line of the nearest number (256d, 216n,
   16d for 100 colors); a new line sends St at once, and the next page keeps it.
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
   A display of 256 colors (`ARCHAIC_COLORS=256`) asks WRP for 216, and one of 128 (as SunView) for 128.
   A new width: one St with the new width, once the size stays (none for a new height); typed text stays.
   No WRP: the box in the middle of the window. The window's title: `Archaic`, then the page's title;
   on WRP's page, the address, not WRP. Archaic's version at the right end of the status line:
   the same pixels at 640 x 400 and 500 x 300. Archaic's logo in the top right corner: the 300 black pixels
   of its picture.
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

## Running on the AT&T UNIX PC (3B1)

- Needs the Ethernet board and **WIN/TCP** (TWG's TCP/IP, in `/usr/ethernet`).
  Kit: `att3b1-unix351-freebee` (FreeBee), on osarchive in `os/att/3b1/virt/`.
- Release binary: `archaic-unixpc`. Copy it to the 3B1 as `/usr/local/bin/archaic`,
  then `chmod 755 /usr/local/bin/archaic`.
- Open a shell window (Office > UNIX System) and type `archaic`.
  It opens a window of its own, as large as the screen allows.
  A telnet or rsh login works too: the window opens on the 3B1's screen.
- Type addresses with `https://`.
- **Exit** (or the close box in the window's frame) quits.
- In the FreeBee kit, your computer is `10.0.2.2` for the 3B1, as for Idris.
  So Archaic on the 3B1 looks for WRP at `http://10.0.2.2:8080/`.
- The 3B1 has no DNS: give WRP's address as numbers, or put the name in `/etc/hosts`.
- `screenshots/unixpc.png`: the 3B1's screen in FreeBee (green on black), 720 x 348:
  `uname -a` in the console, and Archaic in a smaller window (`-g 686x212+0+48`) with
  Wikipedia's AT&T UNIX PC article.

## Building on the AT&T UNIX PC (3B1)

Needs **gcc** (2.5.8 in `/usr/local/bin`: the FreeBee kit has it) and WIN/TCP.

1. Copy these 10 files (197 KB) into a folder on the 3B1, with ftp, rcp or a floppy:
   `src/archaic.h`, `main.c`, `html.c`, `http.c`, `gif.c`, `pic.c`, `update.c`, `unixpc.c`, `ntwg.c` and `mkunixpc`.
   The FreeBee kit forwards the 3B1's ftp to port 35121 on your computer, and rsh to 35514.
2. Build:

   ```
   sh mkunixpc     # compile and link: archaic, about 88 KB
   ```

   It takes about 1 minute in FreeBee at turbo 10.
3. Test. **Test mode, no window needed** (serve this folder from your computer:
   `python3 tools/idris.py 0 serve . 8077`):

   ```
   ./archaic -t http://10.0.2.2:8077/test/data/hn2.gif
   ```

   The last line ends with `sum ae2b58f3`: the same as on Unix and on Idris.
   With `hn2.abm` or `hn2.ab3` in the place of `hn2.gif`, it checks the ABM and ABM3 decoders: the same sum.
   The server must send them as `image/x-abm` and `image/x-abm3`, as WRP and `idris.py serve` do.
   `python3 -m http.server` sends `application/octet-stream`, and Archaic does not show it.

   **With the window:** `./archaic` in a shell window.

**Measured in FreeBee** at 10 MHz, as a real 3B1 (from Return until the picture is complete).
At turbo 10, as in the kit, the CPU's part is 10 times shorter.

| What | Time |
|---|---|
| WRP start page (`-t`) | 1 s |
| decode check (`-t`, `hn2.abm`, 9 KB) | 1 s |
| decode check (`-t`, `hn2.gif`, 11 KB: a GIF with 256 colors in its table, so the slow way) | 9 s |
| Wikipedia page with its picture (`-t`) | 4 s (Archaic 0.3: 13 s) |
| the same in the window, with the address typed by a script | 14 s (Archaic 0.3: 25 s) |

## The AT&T UNIX PC (3B1): what to know

Test on a copy of your 3B1 disk.

### The system

| Thing | 3B1 |
|---|---|
| CPU, memory | 68010 at 10 MHz, up to 4 MB, paged MMU |
| Screen | 720 x 348, black and white (FreeBee can show it green) |
| Window system | in the kernel: `/dev/window`, and **tam**, the window library (in the shared library) |
| Network | Ethernet board, **WIN/TCP** of The Wollongong Group: BSD sockets in `/usr/lib/libnet.a` |
| Addresses (FreeBee kit) | the 3B1 10.0.2.15. **Your computer = 10.0.2.2** (its 127.0.0.1) |
| File names | 14 characters at most |

### The C compiler

- gcc 2.5.8 (`/usr/local/bin/gcc`, not on a stock 3B1). It defines `unixpc`:
  Archaic uses that for the default address.
- `-shlib` links the shared library: libc and tam.
- `<time.h>` has no `time_t`: it is in `<sys/types.h>`.
- `times()` gives the time since the start of UNIX in 1/60 s: Archaic's clock for the pictures' times
  and for the rows on the screen (`nowms()` in `http.c`). The display still has no clock (`g_ms`).
- `/bin/cc`, the 1985 compiler, stops at the `#sccs` line of each header on the kit's disk. Archaic does not use it.
- SysV cannot replace a program that runs: quit Archaic before you link it again.

### The window system

- A window of its own: close 0, 1 and 2, open `/dev/window`, then `winit()` and `wcreate()`.
  `WIOCSETD` then sets the size in pixels.
- Windows live between y 12 and y 300. The frame adds 6 pixels at the left,
  28 at the right (with the scroll arrows), and 20 at the top and at the bottom.
  So the largest inside is 686 x 248, or 720 x 288 without a frame.
- `wrastop()` draws. **1 is a lit pixel**, and the **low** bit of each 16-bit word is the **left** pixel.
  Archaic makes white lit, so pictures are not negatives.
- Text: the font file `/usr/lib/wfont/system.8.ft` (9 x 12, ASCII only), drawn with `wrastop()`.
  Every letter is 9 pixels wide.
- With this font, WRP's form (WRP 4.12.0) only just fits on one line: it ends at x 680, and 682 is the limit
  (686 less the margin). One more letter or item puts the last button on a second line.
  So there is no room for Archaic's logo. With a WRP that has `kb` (no K, ⌫ and ↵), the logo fits.
  So the Z and D pictures are narrower than the others (11 and 9 pixels), and the D menu's words have 4 letters.
  WRP's one menu (C) has only 2p, 2d and 2n there, with D's picture: the form still ends at x 680.
  To check a change: in a copy of `main.c`, make `tw()` return `n * 9` and the `-t` window 686 wide,
  then run `archaic -t -c 2` with WRP. This gives the same places as the real 3B1.
- `keypad(wn, 1)`: the 3B1's keys come as one code each (`kcodes.h`): Exit 0350, Cancl 0370, Page 0371.
  The window is a tty: with VMIN 0 and VTIME 1, a read waits at most 0.1 s.
- Mouse: `wsetmouse()` with MSDOWN. A click comes as the Mouse key (0375),
  then `wreadmouse()` gives x, y in the window and the buttons: 4 left, 2 middle, 1 right.
- The frame's scroll arrows send Roll Up and Roll Down (Shift+↑, Shift+↓). FreeBee's wheel does too.
- **SIGWIND** (the window has a new size) goes only to the window's process group.
  Archaic calls `setpgrp()` before it opens the window, then `ioctl(wn, WIOCPGRP, 0)`.
- `wprompt()` text shows on the screen's prompt line: 72 characters fit.
- Archaic's status line holds about 60 characters (Archaic's version is at its right end): put the main point of a message first.
- Standard error goes to `/dev/null` while the window is open. As 0 and 1, fd 2 was the window,
  and a line on it drew over the page. Archaic's messages are on its status line.

### WIN/TCP

- Headers in `/usr/ethernet/include`, the library `/usr/lib/libnet.a`.
  Compile with `-DSYSTEM5 -DSHORT_IDENT` (TWG's READ_ME).
- `select()` takes bit masks (a `long`), not `fd_set`, and waits as long as asked: 0.1 s works.
  Archaic waits on the network in 0.1 s steps and reads the window's keys between them.
- No DNS: `gethostbyname()` reads only `/etc/hosts`. Archaic tries `inet_addr()` first.

## Running on the AT&T 3B2 with a DMD 5620

- **UNIX System V Release 3.2** on an AT&T 3B2/400, with a **DMD 5620** terminal (the Blit) in **layers**.
  Kit: `att3b2-400-svr32-simh` (Open SIMH, with the dmd5620 emulator), on osarchive in `os/att/3b2/`.
  Archaic is in the kit: `/usr/local/bin/archaic`, and its source in `/usr/local/src/archaic`.
- It needs WIN/3B (the 3B2's TCP/IP) and the DMD software (`/usr/dmd`: layers and `32ld`). The kit has both.
- Release binary: `archaic-3b2` (the kit's). On another 3B2, copy it as `/usr/local/bin/archaic`,
  then `chmod 755 /usr/local/bin/archaic`. It is one file: Archaic's part for the terminal is inside it.
- At the DMD, log in as `root` and type `layers`. Make a window: button 3, New, then drag a rectangle with button 3.
  In the window, type `archaic`.
- Archaic first loads its part into the terminal: the pointer is a coffee cup, for about 25 s.
  **Do not type or click while it loads.** Then WRP's form comes, and a short picture test (10 s).
- The window is the layer. Reshape in the layers menu (button 3) gives it another size.
  A small window is faster: its pictures are smaller.
- **Ctrl-Q** quits: the layer is a terminal again.
- In the kit, your computer is `10.0.2.2` for the 3B2. So Archaic looks for WRP at `http://10.0.2.2:8080/`.
  On another network: `archaic HOST:8080`, or set `WRP` to `HOST:8080`.
  The kit has a name server, so HOST can be a name.
- The screen is black and white. White is a lit pixel, so a page is a bright window.
- `screenshots/dmd5620.png`: the terminal's screen, 800 x 1024: `uname -a` in a layer,
  and Archaic in a second layer with Wikipedia's Blit article.

## Building on the AT&T 3B2

Needs the 3B2's C compiler, `cc` (C Issue 4.2 in the kit), WIN/3B, and the DMD software with `dmdcc`.

1. Copy these 12 files (259 KB) into a folder on the 3B2, with ftp or rsh:
   `src/archaic.h`, `main.c`, `html.c`, `http.c`, `gif.c`, `pic.c`, `update.c`, `dmd.c`, `dmdterm.c`, `ntwg.c`,
   `bin2c.c` and `mk3b2`.
   The kit lets root in with rsh, on port 3514 of your computer (rcp does not work there).
   The kit's own `tools/rshpipe.py` copies a file: `python3 tools/rshpipe.py FILE 'cat > /usr/tmp/an/FILE'`.
2. Build:

   ```
   sh mk3b2        # compile and link: archaic, about 408 KB (357 KB after strip)
   ```

   It takes about 2 minutes in the kit. It makes the terminal's part first (`archaic.m`, with `dmdcc`),
   then puts it into `archaic` (`bin2c` makes `dmdm.h` of it).
3. Test. **Test mode, no terminal needed** (serve this folder from your computer:
   `python3 tools/idris.py 0 serve . 8077`):

   ```
   ./archaic -t http://10.0.2.2:8077/test/data/hn2.gif
   ```

   The last line ends with `sum ae2b58f3`: the same as on the other systems.
   With `hn2.abm` or `hn2.ab3` in the place of `hn2.gif`, it checks the ABM and ABM3 decoders: the same sum.
   (These are the 3B2's decoders. The terminal has its own: see below.)

   **With the window:** `./archaic` in a window of layers.

**Measured in the kit**, with WRP on the same computer. SIMH runs the 3B2 as fast as the host can,
and the DMD is an emulator: on real machines the times are longer. The line (19200 baud, layers) is the slow part.

| What | Time |
|---|---|
| Archaic's part goes into the terminal (11 KB) | about 25 s |
| WRP's form on the screen, the first time (2 KB of commands) | 5 s |
| the picture test: two pictures of a quarter of the frame | 10 s |
| a slice of 608 x 400 as ABM3 (5 KB) | 14 s |
| a slice of 480 x 668 as ABM3 (6 KB) | 15 s |
| the same kind of slice as ABM (10 KB) | 27 s |
| a slice of 768 x 860 as ABM3 (9 KB) | 22 s |
| a scroll to rows that the terminal has | at once (40 bytes, and about 60 for the scroll bar) |
| a letter typed into the address box | 32 bytes |

The frame on the terminal's screen was compared with the GIF of the same rows from WRP:
no pixel differs, for ABM3 and for ABM, also after many scrolls.

## The AT&T 3B2 and the DMD 5620: what to know

Test on a copy of your 3B2 disk.

### The system

| Thing | The kit |
|---|---|
| Computer | AT&T 3B2/400 in Open SIMH: WE32100, 4 MB. SIMH runs it as fast as the host can |
| Terminal | DMD 5620 (the dmd5620 emulator): a WE32100 of its own, 1 MB, 800 x 1024, black and white (green) |
| Line | the 3B2's second console port (`contty`), 19200 baud. In layers (the xt protocol) a program gets about 450 bytes a second |
| Window system | layers: each window (layer) is a channel on the line. `32ld` loads a program into a layer's terminal |
| Network | NI Ethernet, WIN/3B: BSD sockets in `/usr/lib/libnet.a`. SIMH NAT: the 3B2 10.0.2.15, **your computer 10.0.2.2**, name server 10.0.2.3 |
| File names | 14 characters at most. No symbolic links |

### Two parts

The line is the slow part: 450 bytes a second. A decoded slice of 608 x 400 is 30 KB: more than a minute.
So Archaic is two programs.

- **On the 3B2**: Archaic's core, with `dmd.c` as its display. It has the network, the HTML and WRP's form.
- **In the terminal**: `dmdterm.c`, which `dmd.c` loads with `32ld`. It draws, and it has the page's picture.
- **The picture stays packed on the line.** The 3B2 passes WRP's ABM or ABM3 file on as it comes
  (`g_slice`), and the terminal decodes it into its own memory, and shows the rows as they come.
  The terminal keeps the 4 slices; to scroll, the 3B2 says only which page row is at the frame's top (`g_view`).
- **The picture's width is a multiple of 32** (`picsize()` in `main.c`): the decoded rows are then the
  terminal's own bitmap, as they are. So up to 31 pixels at the right of the window stay white.
- **Only what changes goes down the line.** `dmd.c` draws into a copy of the screen in the 3B2's memory, and keeps
  a list of what was drawn. `flush()` compares the copy with the screen as the terminal has it, and sends the
  list again only for the parts that are not the same: the terminal clips to such a part.
  A new page with the same form costs a few bytes.
- The copy does not have the terminal's pixels: the 3B2 does not have the terminal's font. A letter is a pattern
  of its own in its 9 x 14 cell, and a row of the frame is a number made from its page row.
  It only has to be different where the screen is.
- **While the line is busy, nothing new waits in its queue.** What Archaic draws meanwhile goes as one change
  when there is room. Type fast, and only the last state of the address box goes.
- The spinner and the bar (how much of a slice has come) are the terminal's own: they cost no bytes.
- The scroll bar is Archaic's own, drawn through the same list. A picture whose rows repeat (the shaft's dots)
  goes as a pattern: its first 1, 2 or 4 rows.
- While a button is down, the terminal says where the pointer is when it moves (10 times a second at most),
  and only the last place waits in Archaic's queue: the elevator follows the pointer.
- In `main.c`, this is `REMOTE` (`mk3b2` sets it): the slices are not in Archaic's memory, `drawtall()` only
  calls `g_view`, a picture that fits in the window is a slice too, the picture test uses a quarter of the frame,
  and a long text in a field moves 8 letters at a time (so most keys change one letter on the screen).
- The commands of the two parts are in `dmd.c`, at its top.

### The C compilers

- `cc` is AT&T's C Issue 4.2: K&R C, with `void`. It defines `u3b2`: `archaic.h` uses that
  for the WRP address and the system name. `mk3b2` gives it `-DNOPROTO -DREMOTE`.
- `dmdcc` is the same compiler for the terminal, with the DMD's headers and library.
  `dmd.h` makes macros of many common names (`min`, `max`, `sw`, `add`, `point`, `string`):
  `dmdterm.c` keeps away from them. `dmdcc -E` shows what a line becomes.
- In a program for the terminal, a table with values is in its data, and goes down the line at each start.
  Tables with no values are in bss: they cost nothing. `dmdterm.c` is 11 KB on the line.
- `Rect()`, `Pt()`, `rectf()`, `bitblt()` and `string()` are calls into the terminal's ROM, through a table.
  `string()` draws into a bitmap from `balloc()` too: `dmdterm.c` draws a text there first, then clips it.
- libc has no `atexit`, `memmove`, `strcasecmp` or `bcopy`. It has `memcpy`, `memset` and `memcmp`.
- `/usr/gnu/bin/gcc` (2.7.2.3) is in the kit. Archaic does not need it (not tested with it).
- `mk3b2` gives `cc` the option `-K fpe`: the program then needs no math unit (MAU, WE32106).
  Without it, `cc` on a 3B2 that has a MAU (as the kit's) makes a program that runs only with one
  (`file archaic` says `mau hardware required`). Archaic has no floating point of its own.

### WIN/3B

- BSD sockets, as on the 3B1: `ntwg.c` is for both. On the 3B2 `struct sockaddr_in` is in `<sys/in.h>`.
- Link with `-lnet -lnsl_s`: `libnet.a` uses the TLI functions (`t_open` ...) of the shared `libnsl_s`.
- Host names: `gethostbyname()` also asks the name server of `/etc/resolv.conf` (in the kit: SIMH's, 10.0.2.3).
  So `archaic -t http://example.com/` works there. The 3B1's reads only `/etc/hosts`.
- `times()` counts 1/100 s: Archaic's clock for the pictures' times (`nowms()` in `http.c`).
  The display has no clock (`g_ms`).

### The layer's line

- `ioctl(0, JMPX, 0)` says if this is a window of layers. If not, Archaic says so and stops.
- The tty must be raw before `32ld`, with 8 bits and no output processing: else bytes of the program change.
  `VMIN` 0 and `VTIME` 0: a read does not wait. `poll()` with no files waits (30 ms steps).
- On an xt channel, `TCSETAW` can wait for ever. To put the modes back: `TCFLSH`, then `TCSETA`.
- **Flow.** The xt driver of this UNIX stops for good when a program writes more than about 130 bytes
  that the terminal has not read. So the terminal sends `a` for each 64 bytes it has read,
  and `dmd.c` keeps at most 128 bytes on the way.
- **Standard error is the layer too.** A message on it would be commands for the terminal.
  `dmd.c` sends it to `/dev/null` while the window is open. With `-v`, give it a file: `archaic -v 2>log`.
- Keys or clicks while `32ld` loads the program break the download (the emulator stopped with a CPU error).
- If Archaic is killed, the layer stays raw. Type `stty sane` and Ctrl-J. A shell that reads from a tty
  with `VMIN` 0 can end at once: then make a new window.
- A layer keeps its pixels when another one covers it, and when it moves. After Reshape it is empty:
  the terminal says its new size, and Archaic draws all of it again.

### The terminal

- A lit pixel is 1. Archaic makes white lit, so pictures are not negatives.
  The decoders in `dmdterm.c` store the pictures' bytes inverted (in ABM, 1 is black).
- Memory: 4 slices of the page, and one for the picture test. A slice of 608 x 400 is 30 KB;
  a window of the whole screen needs 4 x 90 KB. With no memory for a slice, Archaic says
  `Picture: the terminal has no memory for it. Make the window smaller`.
- The font is the terminal's own, 9 x 14, ASCII only. WRP's form needs a window about 690 pixels wide
  to be one line. In a narrower window it is two lines.
- The arrow keys come as `ESC [ A` to `D`, all at once: the terminal's part makes one code of them.
- Buttons 1 and 2 are the program's. Button 3 is the menu of layers.

### For tests

- `ARCHAICLOG=file archaic ...` writes all the bytes for the terminal to a file.
  On your computer, `python3 tools/dmdlog.py file W H` prints its commands and draws the screen they make.
- `ARCHAICM=file.m archaic ...` loads another program into the terminal (a `dmdterm.c` that you changed),
  not the one inside `archaic`.
- The 3B2's shell tools are old: no `head`, no `strings`; `grep` has no `\|` (use `egrep`);
  `od -b` adds a 0 byte to a file with an odd size.
- With the dmd5620 emulator's MCP tools (`dmd5620 --headless --name NAME`), a test can type, click
  and take the terminal's screen. Wait for the program to load before the first key.

## Running on SunOS 4 with SunView

- **SunOS 4.1** with SunView, on a Sun-4 (SPARC). Kit: `sunos414-lemu` (QEMU),
  on osarchive in `os/sunos/Virtual/qemu/SunOS414/`. It has Sun's C compiler.
  Archaic is in the kit: `/usr/local/bin/archaic`, and its source in `/usr/local/src/archaic`.
- Release binary: `archaic-sun4` (the kit's). Copy it to SunOS as `/usr/local/bin/archaic`,
  then `chmod 755 /usr/local/bin/archaic`. Or build it there: see Building on SunOS 4. It takes about 6 s.
- Log in as `root` at the console and type `sunview`.
  In the shelltool, type `archaic`.
  It opens a frame a little smaller than the screen, in the middle.
- **Ctrl-Q** quits, or Quit in the frame's menu.
- Archaic looks for WRP at `http://10.0.2.2:8080/`: your computer, in QEMU's default user network.
  The kit has another network: there, root's `.cshrc` sets `WRP` to your computer's port 8080 in it.
  On another network: `archaic HOST:8080`, or set `WRP` to `HOST:8080`.
  SunOS has no name server in the kit: give addresses as numbers.
- On a color screen (8 bits, as the cg3) the pictures have up to 128 colors (see Colors).
- `screenshots/sunview.png`: the SPARCstation's screen (cg3), 1152 x 900:
  `uname -a` in the console, and Wikipedia's SunView article in 128 colors, in 1 s.

## Building on SunOS 4 (SunView)

Needs Sun's C compiler, `cc` (the kit has it), and SunView's libraries in `/usr/lib`:
`libsuntool`, `libsunwindow`, `libpixrect` (SunOS has them with SunView).

1. Copy these 10 files (206 KB) into a folder on SunOS, with ftp, rcp or a CD
   (the kit: `CD=file.iso ./run.sh`, then `mount -rt hsfs /dev/sr0 /cdrom`):
   `src/archaic.h`, `main.c`, `html.c`, `http.c`, `gif.c`, `pic.c`, `update.c`, `nsock.c`, `sunview.c` and `mksunview`.
   The kit lets root in with rsh, on port 41414 of your computer.
2. Build:

   ```
   sh mksunview    # compile and link: archaic, about 120 KB
   ```

   It takes about 6 s in the kit.
3. Test. **Test mode, no window needed** (serve this folder from your computer:
   `python3 -m http.server 8077`. HOST: see Running on SunOS 4 with SunView):

   ```
   ./archaic -t http://HOST:8077/test/data/hn2.gif
   ```

   The last line ends with `sum ae2b58f3`: the same as on Unix, Idris and the 3B1.

   **With the window:** `./archaic` in a shelltool.

**Measured in the kit** (QEMU, SPARCstation 5), from Return until the picture is complete:

| What | Time |
|---|---|
| decode check (`-t`, `hn2.gif`, 11 KB, 256 colors) | less than 0.1 s |
| Wikipedia's SunView article in the window, 128 colors (91 KB) | 1 s |
| Wikipedia's Sun-4 article in the window, 128 colors (50 KB) | 1 s |
| Wikipedia's SunView article, black and white (`-c 2`: ABM, 21 KB) | 2 s |

## SunOS 4 and SunView: what to know

Test on a copy of your SunOS disk.

### The system

| Thing | SunOS 4.1.4 kit |
|---|---|
| Computer | a SPARCstation 5 in QEMU: microSPARC II, 256 MB (SunOS gets 240) |
| Screen | cg3, 8 bits: 256 colors in one colormap. 1280 x 1024 (`RES=1152x900 ./run.sh` for another size) |
| Window system | SunView: `sunview` at the console |
| Network | Lance Ethernet, BSD sockets. QEMU user networking: **your computer** (its 127.0.0.1) is the network's host address, `10.0.2.2` by default |
| Names | no name server: give addresses as numbers |

### The C compiler

- Sun's `cc` is K&R C: `mksunview` gives it `-DNOPROTO`. (gcc 3.0.4 is in `/usr/local/bin` in the kit;
  Archaic does not need it.)
- Link with `-lsuntool -lsunwindow -lpixrect`: the toolkit, the windows and the notifier, the drawing.
- SunView's headers have an `Event` and an `Image`, as `archaic.h` has, and a `max` macro.
  `sunview.c` renames SunView's two while it includes them.
- `gethostbyname()` knows only names. `nsock.c` tries `inet_addr()` first.

### SunView

- A SunView program started from rsh or telnet needs to know the screen:
  `WINDOW_PARENT=/dev/win0` and `WMGR_ENV_PLACEHOLDER=/dev/win1` (a shelltool has them).
- Archaic has its own loop: `notify_dispatch()` runs SunView's handlers, and `select()` waits on the
  frame's and the canvas's files. Each `notify_dispatch()` reads about one event.
- The window: a frame with a canvas, drawn through the canvas's pixwin.
  Rectangles with `pw_writebackground()`, text with `pw_text()` in the default font
  (`$DEFAULT_FONT` chooses another one), pictures with `pw_rop()` from a pixrect in memory
  (its rows are a whole number of 16-bit words).
- The canvas is not retained. A retained canvas (SunView keeps a copy) lost its copy after a resize,
  and drew those parts white. So SunView calls Archaic's repaint function, and Archaic draws those parts again.
- The scroll bar is SunView's. An interposed function takes its scroll requests from the canvas.
  The bar has already set its new view start: that is the new place on the page.
- Colors: `pw_setcmsname()` and `pw_putcolormap()` make a colormap segment of 128: 0 white, 127 black.
  The segments of all windows share the screen's 256 colors: SunView loads the colors of the window
  under the pointer. So with two Archaic windows (or another program with many colors),
  the other window can show wrong colors until the pointer is in it.
- Keys: the arrows come as `ESC [ A` to `ESC [ D`, one character for each `notify_dispatch()`.
  So after an ESC, Archaic waits 1/20 s for each next character. A lone ESC is the Esc key.
  R7, R9, R13 and R15 come as function keys: Home, PgUp, End, PgDn.
  The Stop key (L1) sends nothing: it is the shift key of Stop-A.
- The frame menu's Quit: an interposed destroy function says no to it (`notify_veto_destroy()`).
  Archaic quits, then closes the frame.

## Running on SunOS 3.5 on a Sun-3

- **SunOS 3.5** with SunView, on a Sun-3 (68020), in TME (The Machine Emulator):
  a Sun-3/160 with the bwtwo screen, and slirp for the network.
  Kit: `sun3-sunos35-tme`, on osarchive in `os/sunos/Virtual/tme/`.
  Archaic is in the kit: `/usr/local/bin/archaic`, and its source in `/usr/local/src/archaic`.
- Release binary: `archaic-sun3` (the kit's). Copy it to SunOS as `/usr/local/bin/archaic`,
  then `chmod 755 /usr/local/bin/archaic`. Or build it there: see Building on SunOS 3.5.
- Log in as `root` at the console and type `suntools` (SunOS 3's name for SunView).
  In a shelltool, type `archaic`.
- **Ctrl-Q** quits, or Quit in the frame's menu.
- With TME's slirp, your computer is `10.0.2.2` for SunOS (SunOS is `10.0.2.15`).
  So Archaic on a Sun-3 looks for WRP at `http://10.0.2.2:8080/`.
- `screenshots/sun3.png`: the Sun-3's screen (bwtwo), 1152 x 900, black and white:
  `/usr/5bin/uname -a` and the release line of `/etc/motd` in a shelltool,
  and Wikipedia's Sun-3 article, in 2 s.

## Building on SunOS 3.5

The same 10 files and the same script as on SunOS 4 (see Building on SunOS 4), with Sun's `cc`:

```
sh mksunview    # compile and link: archaic, about 580 KB
```

It takes about 30 s in TME. The program is large: SunOS 3.5 has no shared libraries.
Test mode: `./archaic -t http://10.0.2.2:8077/test/data/hn2.gif` ends with `sum ae2b58f3`.

**Measured in TME** (on an Apple Silicon Mac), from Return until the picture is complete:

| What | Time |
|---|---|
| decode check (`-t`, `hn2.gif`, 11 KB, 256 colors) | 0.6 s |
| decode check (`-t`, `hn2.abm`, 9 KB) | less than 0.1 s |
| Wikipedia's Sun-3 article in the window (ABM, 26 KB) | 2 s |

## SunOS 3.5: what to know

Test on a copy of your SunOS disk.

| Thing | SunOS 3.5 in TME |
|---|---|
| Computer | Sun-3/160: 68020 and 68881, 32 MB |
| Screen | bwtwo: 1152 x 900, black and white |
| Window system | SunView 1: `suntools` at the console |
| Network | Intel Ethernet (`ie0`), BSD sockets (4.2BSD). TME's slirp: SunOS 10.0.2.15, **your computer 10.0.2.2** (its 127.0.0.1) |

- Sun's `cc` defines `sun`, `mc68000` and `mc68020`, but not `sun3`.
- gcc 1.42 (if `/usr/local/bin` has it) stops in `<sundev/vuid_event.h>`. Use Sun's `cc`.
- `<suntool/sunview.h>` guards itself with the name `SUNVIEW` (`#ifndef SUNVIEW.H`).
  The build's `-DSUNVIEW` made it empty, so `sunview.c` undefines `SUNVIEW` first.
- `<pixrect/memvar.h>` declares `mem_ops` only in its `mpr_static` macro: `sunview.c` declares it.
- `<sys/types.h>` has `fd_set` (one `int`), but no `FD_SET`, `FD_ZERO` or `FD_ISSET`:
  `nsock.c` and `sunview.c` define them.
- `time_t` is only in `<sys/types.h>`, not in `<time.h>`.
- `stderr` is buffered when it goes to a file: Archaic flushes each `-v` line.
- For Archaic, SunView is the same as on SunOS 4: the same toolkit calls, and the same
  variables for a program started from rsh (`WINDOW_PARENT=/dev/win0`, `WMGR_ENV_PLACEHOLDER=/dev/win1`).
- The shell's tools are old: no `printf`, and `grep` has no `\|` (use `egrep`).
  `uname` is only System V's, `/usr/5bin/uname`: it is not on root's path, and it does not
  name the release. `head -1 /etc/motd` does.

## Running on HP-UX 9

- **HP-UX 9.05** with X11 and HP VUE, on an HP 9000/715 (PA-RISC). Kit: `hpux905-715-lemu` (QEMU),
  on osarchive in `os/hpux/Qemu/`. It has HP's C compiler.
  Archaic is in the kit: `/usr/local/bin/archaic`, and its source in `/usr/local/src/archaic`.
- Release binary: `archaic-hpux9` (the kit's). Copy it to HP-UX as `/usr/local/bin/archaic`,
  then `chmod 755 /usr/local/bin/archaic`. Or build it there: see Building on HP-UX 9.
- Log in as `root` at the VUE login (the kit has no password). In a terminal (hpterm), type `archaic`.
  The window is 640 x 400: `archaic -g 860x660+360+120` gives a larger one.
- **Ctrl-Q** quits, or Close in the window's menu.
- Archaic looks for WRP at `http://10.0.2.2:8080/`: your computer, in the kit's network (QEMU NAT).
  On another network (a bridged VM, a real HP): `archaic HOST:8080`, or set `WRP` to `HOST:8080`.
  HP-UX has no name server in the kit: give addresses as numbers.
- The screen has 8 bits, so the pictures have the 216 web colors (see 216 colors: GIP).
- `screenshots/hpux.png`: the 715's screen, 1280 x 1024, in HP VUE:
  `uname -a` in an hpterm, and Wikipedia's HP-UX article in 216 colors, in 7 s.

## Building on HP-UX 9

Needs HP's C compiler, `cc` (the kit has HP C A.09.73), and X11R5 (HP-UX 9 has it).

1. Copy these 10 files (200 KB) into a folder on HP-UX, with ftp, rcp or NFS:
   `src/archaic.h`, `main.c`, `html.c`, `http.c`, `gif.c`, `pic.c`, `update.c`, `nsock.c`, `x11.c` and `mkhpux`.
2. Build:

   ```
   sh mkhpux       # compile and link: archaic, about 132 KB
   ```

   It takes about 3 s in the kit.
3. Test. **Test mode, no window needed** (serve this folder from your computer:
   `python3 -m http.server 8077`):

   ```
   ./archaic -t http://10.0.2.2:8077/test/data/hn2.gif
   ```

   The last line ends with `sum ae2b58f3`: the same as on the other systems.
   All the test pictures of `test/run.sh` (1, 1b, 2 and 2b) give exactly the bytes they give on the Mac.

   **With the window:** `./archaic` in an hpterm.

**Measured in the kit** (QEMU on an Apple Silicon Mac), from Return until the picture is complete.
Archaic decodes a slice in less than 0.1 s here: the rest of the time is WRP's.

| What | Time |
|---|---|
| decode check (`-t`, `hn2.gif`, 11 KB, 256 colors) | less than 0.1 s |
| Wikipedia's HP-UX article in the window, 216 colors (37 KB) | 7 s |
| Wikipedia's PA-RISC article, after a click on its link (33 KB) | 6 s |
| the same page, C set to 256 (44 KB) | 6 s |

## HP-UX 9: what to know

Test on a copy of your HP-UX disk.

| Thing | HP-UX 9.05 kit |
|---|---|
| Computer | an HP 9000/715 in QEMU (LEMU): one PA-7100LC, 128 MB |
| Screen | Artist graphics: 1280 x 1024, 8 bits: 256 colors in one colormap (PseudoColor) |
| Window system | X11R5 with HP VUE 3.0 (window manager: vuewm) |
| Network | LASI Ethernet (82596), BSD sockets. QEMU NAT: HP-UX 10.0.2.15, **your computer 10.0.2.2** (its 127.0.0.1) |
| Names | no name server: give addresses as numbers |

- HP's `cc` (HP C A.09.73 in the kit) is K&R C by default, and ANSI C with `-Aa -D_HPUX_SOURCE`.
  `mkhpux` uses K&R (`-DNOPROTO`), as on SunOS: in ANSI C, HP-UX's `select()` takes `int *`,
  not `fd_set *`, and each call gets warnings. The `cc` that comes with HP-UX alone (K&R only) is not tested.
- `+DA1.0`: PA-RISC 1.0 code, for every PA-RISC computer (on a 715 the default is 1.1).
  `file archaic` then says `s800 shared executable`.
- X11R5 is not on the default paths: `-I/usr/include/X11R5`, `-L/usr/lib/X11R5`.
  Archaic uses the shared `/usr/lib/X11R5/libX11.sl` (`chatr archaic` lists it).
- gcc 3.3.6 is in `/usr/local/bin` in the kit. Archaic does not need it.
- Two builds of the same source differ in 15 time stamps (45 bytes): `cmp` alone does not show that they are the same.
- From an rsh or telnet login, `DISPLAY=:0` reaches the screen (as root, with root logged in to VUE).
- Page Up and Page Down come as the keys Prior and Next (Prev and Next on an HP keyboard).
- The colormap: VUE takes most of it. In the kit, 160 to 180 cells were free.
  A page of text took 30 to 55 cells (C at 216), and 63 with C at 256 (with no dither, as 256n now).
  When Archaic quits, the X server frees them all.
- `mount`, `shutdown` and other system programs are in `/etc`: rsh's PATH does not have it.
  `/etc/mount HOST:/dir /mnt` mounts an NFS folder.
- `tr` does not know `\n`: write `\012`.

## Running on DG/UX

- **DG/UX R4.20MU07** with X11, on a Pentium PC in 86Box. Kit: `DGUX-R4.20MU07-86Box.tar.lz`,
  on osarchive in `os/dgux/Virtual/`. Archaic is in the kit: `/usr/local/bin/archaic`,
  and its source in `/usr/local/src/archaic`.
- Release binary: `archaic-dgux` (the kit's). On another DG/UX R4.20 with X11, copy it as `/usr/local/bin/archaic`,
  then `chmod 755 /usr/local/bin/archaic`. Or build it there: see Building on DG/UX.
- Log in as `root` at the X login (the kit has no password). An xterm opens: type `archaic`.
- **Ctrl-Q** quits.
- Archaic looks for WRP at `http://10.0.2.2:8080/`: your computer, in 86Box's network (SLiRP).
  On another network: `archaic HOST:8080`, or set `WRP` to `HOST:8080`.
  The kit has no name server (`/etc/resolv.conf` is empty): give addresses as numbers.
- The screen has 8 bits, so the pictures have the 216 web colors (see 216 colors: GIP).
- `screenshots/dgux.png`: the screen, 1280 x 1024, with mwm: `uname -a` in an xterm,
  and Wikipedia's DG/UX article in 216 colors, in 7 s.

## Building on DG/UX

Needs DG/UX's C compiler, `cc` (in the SDK, `/usr/opt/sdk`), and X11 (DG/UX has X11R6).
The kit has both.

1. Copy these 10 files (200 KB) into a folder on DG/UX, with ftp, rcp or NFS:
   `src/archaic.h`, `main.c`, `html.c`, `http.c`, `gif.c`, `pic.c`, `update.c`, `nsock.c`, `x11.c` and `mkdgux`.
2. Build:

   ```
   sh mkdgux       # compile and link: archaic, about 95 KB
   ```

   It takes about 5 s in the kit.
3. Test. **Test mode, no window needed** (serve this folder from your computer:
   `python3 -m http.server 8077`):

   ```
   ./archaic -t http://10.0.2.2:8077/test/data/hn2.gif
   ```

   The last line ends with `sum ae2b58f3`: the same as on the other systems.
   All 9 pictures in `test/data` (7 GIF, 2 ABM) give the same sums as on the Mac.

   **With the window:** `./archaic` in an xterm.

**Measured in the kit** (86Box on an Apple Silicon Mac), from Return until the picture is complete.
The decode check takes less than 0.1 s of CPU (`/usr/bin/time`). Most of a page's time is WRP's:
its log shows 6 s and 7 s from the request until the picture was ready.

| What | Time |
|---|---|
| decode check (`-t`, `hn2.gif`, 11 KB, 256 colors), with the download | 0.4 s |
| Wikipedia's DG/UX article in the window, 216 colors (35 KB) | 7 s |
| Wikipedia's Data General article, after a click on its link (50 KB) | 8 s |

## DG/UX: what to know

Test on a copy of your DG/UX disk.

| Thing | DG/UX R4.20MU07 kit |
|---|---|
| Computer | a PC in 86Box: ASUS P/I-P55TP4XE (Intel 430FX), Pentium 200 MHz, 128 MB |
| Screen | ATI Mach32 (PCI): 1280 x 1024, 8 bits: 256 colors in one colormap (PseudoColor) |
| Window system | X11R6 from Data General (window manager: mwm, Motif) |
| Network | DECchip 21040 Tulip, BSD sockets. SLiRP: DG/UX 10.0.2.15, **your computer 10.0.2.2** |
| Names | no name server: give addresses as numbers |

- `cc` is DG/UX's C compiler from the SDK, with GCC 2.7.2 inside (`cc -v` shows its cpp and cc1).
  It defines `__DGUX__` and `DGUX`. `archaic.h` uses `__DGUX__` for the WRP address.
  The kit also has gcc 3.0.4 in `/usr/local/bin`: Archaic does not need it (not tested with it).
- BSD sockets are in libsocket and libnsl, as on System V Release 4.
  `ldd archaic` lists libX11.so.2, libsocket.so.1, libnsl.so.1, libc.so.1 and libresolv.so.1.
- From an rsh or telnet login as root, `DISPLAY=:0` reaches the screen.
- Programs built on other systems (SCO, UnixWare, Solaris x86) reach DG/UX's X server only over TCP:
  `DISPLAY=localhost:0`, after `xhost +localhost`. Archaic is built on DG/UX: `:0` works.
- The shell is an old Bourne shell: no `$(...)`. `grep` has no `-A` or `-B` (the kit has GNU grep as `ggrep`).
- `mount` and `umount` are in `/etc` and `/sbin`: rsh's PATH does not have them.
- `shutdown` from rsh does nothing. `/sbin/init 0` stops DG/UX, then the PC restarts:
  its boot loader boots DG/UX again after 15 s.

## Running on AIX 3.2

- **AIX 3.2.5** with X11R5 (AIXwindows), on an IBM RS/6000 7006-41T (PowerPC 601).
  Kit: `aix325rs6000-lemu` (QEMU), on osarchive in `os/aix/Virtual/`. It has IBM's C compiler.
  Archaic is in the kit: `/usr/local/bin/archaic`, and its source in `/usr/local/src/archaic`.
- Release binary: `archaic-aix3` (the kit's). On another AIX 3.2 with X11, copy it as `/usr/local/bin/archaic`,
  then `chmod 755 /usr/local/bin/archaic`. Or build it there: see Building on AIX 3.2.
- Log in as `root` at the xdm login (the kit has no password). An aixterm opens: type `archaic`.
- **Ctrl-Q** quits, or Close in the window's menu.
- Archaic looks for WRP at `http://10.0.2.2:8080/`: your computer, in the kit's network (QEMU NAT).
  On another network: `archaic HOST:8080`, or set `WRP` to `HOST:8080`.
  The kit has a name server, so HOST can be a name.
- The screen has 8 bits, so the pictures have the 216 web colors (see 216 colors: GIP).
- `screenshots/aix.png`: the screen, 1280 x 1024, with mwm: `uname -a` in an aixterm,
  and Wikipedia's IBM AIX article in 216 colors, in 7 s.

## Building on AIX 3.2

Needs IBM's C compiler, `cc` (XL C 1.3 in the kit), and X11R5 (AIX 3.2 has it in AIXwindows).
The kit has both.

1. Copy these 10 files (200 KB) into a folder on AIX, with ftp, rcp or NFS:
   `src/archaic.h`, `main.c`, `html.c`, `http.c`, `gif.c`, `pic.c`, `update.c`, `nsock.c`, `x11.c` and `mkaix`.
2. Build:

   ```
   sh mkaix        # compile and link: archaic, about 146 KB
   ```

   It takes about 25 s in the kit. gcc builds it too (2.7.2.2 in `/usr/local/bin` in the kit):
   `CC=gcc sh mkaix` takes 4 s, and the program is about 180 KB.
   In a copy of the whole repository, `make` or `gmake` runs `mkaix` in `src`: `CC=gcc gmake` for gcc.
3. Test. **Test mode, no window needed** (serve this folder from your computer:
   `python3 tools/idris.py 0 serve . 8077`):

   ```
   ./archaic -t http://10.0.2.2:8077/test/data/hn2.gif
   ```

   The last line ends with `sum ae2b58f3`: the same as on the other systems.
   With `hn2.abm` in the place of `hn2.gif`, it checks the ABM decoder: the same sum.
   The pictures of `test/run.sh` (1, 1b, 1c, 2 and 2b) give exactly the bytes they give on the Mac.

   **With the window:** `./archaic` in an aixterm.

**Measured in the kit** (QEMU on an Apple Silicon Mac), from Return until the picture is complete.
Archaic decodes a picture in about 0.1 s here: most of a page's time is WRP's.

| What | Time |
|---|---|
| decode check (`-t`, `hn2.gif`, 11 KB, 256 colors), with the download | 0.2 s |
| decode check (`-t`, `hn2.abm`, 9 KB) | 0.1 s |
| Wikipedia's IBM AIX article in the window, 216 colors (34 KB) | 7 s |
| Wikipedia's IBM article, after a click on its link (41 KB) | 7 s |
| the same page, C set to 256 (81 KB) | 8 s |
| Wikipedia's RS/6000 article (40 KB) | 2 s |
| the same page, after the window was made narrower (12 KB) | 1 s |

## AIX 3.2: what to know

Test on a copy of your AIX disk.

| Thing | AIX 3.2.5 kit |
|---|---|
| Computer | an IBM RS/6000 7006-41T in QEMU (LEMU): PowerPC 601, 256 MB |
| Screen | GXT150L: 1280 x 1024, 8 bits: 256 colors in one colormap (PseudoColor) |
| Window system | X11R5 (AIXwindows), with xdm and mwm (Motif) |
| Network | Ethernet, BSD sockets. QEMU NAT: AIX 10.0.2.15, **your computer 10.0.2.2** (its 127.0.0.1) |
| Names | QEMU's name server, 10.0.2.3 |

- `cc` is IBM's XL C 1.3, in its extended mode. It defines `_AIX`, `_AIX32` and `_IBMR2`.
  `archaic.h` uses `_AIX` for the WRP address. Plain `char` is unsigned.
- XL C's code runs on every RS/6000: POWER, POWER2 and PowerPC (its default is `-qarch=com`).
- gcc 2.7.2.2 is in `/usr/local/bin` in the kit (from the UCLA AIX PDS archive).
- `fd_set` is in `<sys/select.h>`: `<sys/types.h>` has it only with `_BSD`.
  `x11.c` and `nsock.c` include it on AIX.
- BSD sockets are in libc. X11R5 is on the default paths. `dump -H archaic` lists
  the shared libraries: `libc.a` (`shr.o`) and `libX11.a` (`shr4.o`).
- From an rsh login as root, `DISPLAY=:0` reaches the screen (with root logged in at the xdm login).
- rsh's PATH has no `/usr/sbin`: `/usr/sbin/mount HOST:/dir /mnt` mounts an NFS folder
  (it works through QEMU NAT).
- `grep` has no `\|`: use `egrep`. `tr` does not know `\n`: write `\012`.
- Stop AIX with `shutdown -F`. Wait for `Halt completed` on the screen, then quit QEMU.

## Running on AIX PS/2 1.3

- **AIX PS/2 1.3** (PTF 0024) with X11 and AIXwindows (Motif), on a 486 PC.
  Kit: `aix13ps2-lemu` (QEMU), on osarchive in `os/aix/Virtual/`. It has the MetaWare C compiler.
  Archaic is in the kit: `/usr/local/bin/archaic`, and its source in `/usr/local/src/archaic`.
- Release binary: `archaic-aixps2` (the kit's). On another AIX PS/2 1.3 with X11, copy it as `/usr/local/bin/archaic`,
  then `chmod 755 /usr/local/bin/archaic`. Or build it there: see Building on AIX PS/2 1.3.
- Log in as `root` at the xdm login (the kit has no password: Enter at `Password:`).
  The AIX Shell (an aixterm) opens: type `archaic`.
- **Ctrl-Q** quits, or Close in the window's menu.
- Archaic looks for WRP at `http://10.0.2.2:8080/`: your computer, in the kit's network (QEMU NAT).
  On another network: `archaic HOST:8080`, or set `WRP` to `HOST:8080`.
  The kit has a name server, so HOST can be a name.
- The screen has 8 bits, so the pictures have the 216 web colors (see 216 colors: GIP).
- `screenshots/aixps2.png`: the screen, 1024 x 768, with mwm: `uname -a` in the AIX Shell,
  and Wikipedia's IBM PS/2 article in 216 colors, in 2 s.

## Building on AIX PS/2 1.3

Needs the MetaWare High C compiler, `cc` (R2.2g), and X11 (AIX PS/2 has it in X Windows).
The kit has both.

1. Copy these 10 files (200 KB) into a folder on AIX PS/2, with ftp, rcp or NFS:
   `src/archaic.h`, `main.c`, `html.c`, `http.c`, `gif.c`, `pic.c`, `update.c`, `nsock.c`, `x11.c` and `mkaixps2`.
2. Build:

   ```
   sh mkaixps2     # compile and link: archaic, about 355 KB
   ```

   It takes about 3 s in the kit. The kit's `hcc` (`cc` with fixes for 5 MetaWare bugs) builds it too:
   `CC=hcc sh mkaixps2`. The pictures are the same.
3. Test. **Test mode, no window needed** (serve this folder from your computer:
   `python3 tools/idris.py 0 serve . 8077`):

   ```
   ./archaic -t http://10.0.2.2:8077/test/data/hn2.gif
   ```

   The last line ends with `sum ae2b58f3`: the same as on the other systems.
   With `hn2.abm` in the place of `hn2.gif`, it checks the ABM decoder: the same sum.
   All 9 pictures in `test/data` (7 GIF, 2 ABM) give the same sums as on the Mac, also with `-c 256`.

   **With the window:** `./archaic` in the AIX Shell.

**Measured in the kit** (QEMU on an Apple Silicon Mac), from Return until the picture is complete.
Archaic decodes a picture in less than 0.1 s here: most of a page's time is WRP's.

| What | Time |
|---|---|
| decode check (`-t`, `hn2.gif`, 11 KB, 256 colors), with the download | less than 0.1 s |
| decode check (`-t`, `hn2.abm`, 9 KB) | 0.1 s |
| Wikipedia's IBM PS/2 article in the window, 216 colors (37 KB) | 2 s |
| Wikipedia's IBM AIX article, after a click on its link (31 KB) | 7 s |
| the same page, C set to 2p (ABM, 13 KB) | 7 s |

## AIX PS/2 1.3: what to know

Test on a copy of your AIX PS/2 disk.

| Thing | AIX PS/2 1.3 kit |
|---|---|
| Computer | a 486 PC in QEMU (LEMU): 16 MB |
| Screen | S3 86C928 with 1 MB: 1024 x 768, 8 bits: 256 colors in one colormap (PseudoColor) |
| Window system | X11 from IBM, with xdm and mwm (Motif) |
| Network | AMD PCnet (the leaix driver), BSD sockets. QEMU NAT: AIX PS/2 10.0.2.15, **your computer 10.0.2.2** (its 127.0.0.1) |
| Names | QEMU's name server, 10.0.2.3 |

- `cc` is MetaWare High C R2.2g. It defines `_AIX`, `ps2` and `_I386`, but not `__STDC__`.
  `archaic.h` uses `_AIX` for the WRP address. Plain `char` is unsigned.
- MetaWare's code generator can shift by a register that it did not load, also with `-O`.
  In Archaic's GIF decoder it did so twice, and the pictures were wrong:
  `1 << mcs` as the first line of a function, and a static variable kept in a register over a call.
  `gif.c` has neither now (see its comments). Without `-O` the code is worse: keep `-O`.
  To look for this bug: `cc -O -S file.c`, then for each shift by `%cl`, find where `%ecx` got its value.
  A copy from a register that nothing set before it, or that a `call` changed, is the bug.
- IBM's X server loses pixels of 1-bit pictures (`XPutImage` of an `XYBitmap`, `XCopyPlane`, opaque stipples)
  at most places that do not start on a byte of the screen. WRP's buttons lost the right part of their pictures.
  8-bit pictures and text are right. So `x11.c` puts a 1-bit picture into a pixmap first, then copies it to the window.
  On other X servers the pixels are the same as before (tested with Xvfb, 24 and 8 bits).
- There are no shared libraries: Xlib (`/usr/lib/libX11.a`) goes into the program.
  BSD sockets are in libc. `fd_set` and its macros are in `<sys/types.h>`.
- From a telnet or rsh login as root, `DISPLAY=unix:0` reaches the screen (with root logged in at the xdm login).
- rsh's PATH is `:/bin:/usr/bin:/usr/ucb`: `mount` and `shutdown` are in `/etc`, `archaic` in `/usr/local/bin`.
- inetd stops rsh for 10 minutes after about 40 rsh logins in a minute
  (`shell/tcp server failing (looping)` in `/aixps2/adm/messages`). Telnet still works.
  A test script must wait 2 s between rsh logins.
- `/etc/mount HOST:/dir /mnt` mounts an NFS folder. HOST must be a name from `/etc/hosts`:
  with an address, mount says `not in hosts database`.
- The shell has no `printf`; `echo` knows `\n`. `grep` takes only one `-e`. `mkdir` has no `-p`. `strings` crashes.
- Stop AIX PS/2 with `shutdown -h now` (from rsh: `/etc/shutdown -h now`). Wait for `910-772 System halted`
  on the screen, then quit QEMU.

## Running on Ultrix 4

- **Ultrix 4.5** with DECwindows (UWS V4.5: X11 and Motif), on a DECstation 5000/200 (MIPS R3000).
  Kit: `decstation5000-ultrix45-gxemul` (gxemul), on osarchive in `os/ultrix/virt/`.
  Archaic is in the kit: `/usr/local/bin/archaic`, and its source in `/usr/local/src/archaic`.
- Release binary: `archaic-ultrix4` (the kit's). On another Ultrix 4 with X11, copy it as `/usr/local/bin/archaic`,
  then `chmod 755 /usr/local/bin/archaic`. Or build it there: see Building on Ultrix 4.
- Log in as `root` at the DECwindows login (the kit's password is `ultrix`).
  Open a DECterm (Session Manager: Applications > DECterm) and type `archaic HOST:8080`.
- **Ctrl-Q** quits, or Close in the window's menu.
- HOST is your computer's address on its network (its LAN address, as numbers). Or set `WRP` to `HOST:8080`.
  gxemul's network (NAT) has no address for your computer's own 127.0.0.1 (QEMU's NAT has one: 10.0.2.2).
  So on Ultrix Archaic has no WRP address of its own: with none, it tries 127.0.0.1 in Ultrix itself,
  and says `connection refused`. WRP listens on all addresses by default (`-l :8080`), so Ultrix reaches it.
- The screen has 8 bits, so the pictures have the 216 web colors (see 216 colors: GIP).
- `screenshots/ultrix.png`: the screen, 1280 x 1024, with mwm: `uname -a` in a DECterm,
  and Wikipedia's Ultrix article in 216 colors.

## Building on Ultrix 4

Needs Ultrix's C compiler, `cc`, and X11 (the UWS subsets: DECwindows). The kit has both.

1. Copy these 10 files (200 KB) into a folder on Ultrix, with ftp, rcp or NFS:
   `src/archaic.h`, `main.c`, `html.c`, `http.c`, `gif.c`, `pic.c`, `update.c`, `nsock.c`, `x11.c` and `mkultrix`.
   The kit lets root in with rsh, on port 45514 of your computer.
2. Build:

   ```
   sh mkultrix     # compile and link: archaic, about 800 KB (516 KB after strip)
   ```

   It takes about 6 s in the kit. gcc builds it too (3.0.4 in `/freeware/gnu-tools/mips/bin` in the kit):
   `CC=gcc sh mkultrix` takes 16 s.
3. Test. **Test mode, no window needed** (serve this folder from your computer, on its LAN address:
   `python3 tools/idris.py 0 serve . 8077 HOST`):

   ```
   ./archaic -t http://HOST:8077/test/data/hn2.gif
   ```

   The last line ends with `sum ae2b58f3`: the same as on the other systems.
   With `hn2.abm` in the place of `hn2.gif`, it checks the ABM decoder: the same sum.
   All 9 pictures in `test/data` (7 GIF, 2 ABM) give the same sums as on the Mac, also with `-c 256`.

   **With the window:** `./archaic HOST:8080` in a DECterm.

**Measured in the kit** (gxemul on an Apple Silicon Mac), from Return until the picture is complete.
Archaic decodes a picture in less than 0.1 s here: most of a page's time is WRP's.

| What | Time |
|---|---|
| decode check (`-t`, `hn2.gif`, 11 KB, 256 colors), with the download | less than 0.1 s |
| Wikipedia's Ultrix article in the window, 216 colors (45 KB) | 7 s |
| Wikipedia's Ken Olsen article, after a click on its link (46 KB) | 1 s |

## Ultrix 4: what to know

Test on a copy of your Ultrix disk.

| Thing | Ultrix 4.5 kit |
|---|---|
| Computer | a DECstation 5000/200 (3MAX) in gxemul: MIPS R3000, 128 MB |
| Screen | HX (PMAGB-BA): 1280 x 1024, 8 bits: 256 colors in one colormap (PseudoColor) |
| Window system | DECwindows (UWS V4.5): X11 from DEC, with mwm (Motif) and the Session Manager |
| Network | PMAD-AA Ethernet (`ln0`), BSD sockets. gxemul's NAT: Ultrix 10.0.0.1, the gateway and name server 10.0.0.254. **No address for your computer's 127.0.0.1**: use its LAN address |
| Names | gxemul's name server, 10.0.0.254 (after `/etc/hosts`) |

- `cc` is Ultrix's own C compiler. It defines `ultrix`, `__ultrix`, `mips`, `__mips`, `MIPSEL`, `unix` and `bsd4_2`.
  In its default mode it takes prototypes, but it does not define `__STDC__`. `mkultrix` needs no `-DNOPROTO`.
- gcc 3.0.4 (in `/freeware/gnu-tools/mips/bin` in the kit) defines the same names, and `__STDC__`.
- Ultrix has no shared libraries: Xlib (`/usr/lib/libX11.a`) goes into the program.
- `fd_set` and its macros are in `<sys/types.h>`: `nsock.c` and `x11.c` need nothing for Ultrix.
- DECwindows has no font named `6x13`. `x11.c` then takes `fixed`, which is 6 x 13 too.
- From an rsh login as root, `DISPLAY=unix:0` reaches the screen (with root logged in at the DECwindows login).
- The LK201 keyboard has no Page Up or Page Down: its Prev Screen and Next Screen come as Prior and Next,
  and scroll a window. gxemul types them for the host's Page Up and Page Down.
- root's shell is csh: `sh -c '...'` for sh syntax, `>&` for `2>&1`. Quote patterns such as `'x[y]'`: csh expands them first.
- `/` is 16 MB and almost full, and `/tmp` is on it: two builds there filled it. Build in `/usr/local/src` or `/var/tmp`.
- `/etc/mount -t nfs HOST:/dir /mnt` mounts an NFS folder (NFS version 2, through gxemul's NAT).
- `grep` has no `-o`. Use `egrep` for `|`.
- Stop Ultrix with `/etc/shutdown -h now` (from rsh: `/etc/shutdown -h now >& /dev/null &`). Wait for `>>`, then quit gxemul.
- Tests with no window: start gxemul with `--control unix:ctl.sock` (and `SDL_VIDEODRIVER=dummy`).
  `gxemul --ctl unix:ctl.sock keyboard`, `mouse` and `screendump` type, click and show the screen
  (`gxemul --mcp unix:ctl.sock` gives them as MCP tools). A `mouse` click sent with its move was lost once:
  move first, then send `down` and `up`. For Return, send `keys: ["Return"]`: `keyboard` drops control
  characters in `text` (gxemul 1.0.13), so `enter: true` with `text` types nothing.

## Running on Mac OS 6

- **System 6.0.8** on a Macintosh IIcx (68030), with MacTCP 2.0.6, in the
  [Snow](https://github.com/twvd/snow) emulator (1.6). Snow emulates a DaynaPORT SCSI Ethernet box,
  with NAT to your computer's network.
- Release files: `archaic-macos6.dsk`, an 800 KB floppy image with Archaic (put it in Snow's floppy drive),
  and `archaic-macos6.bin`, the same Archaic as MacBinary, for a real Mac. Or build them: `make mac`
  (see Building for the classic Mac OS). WRP's `/archaic-download` does not give them: it takes only names with no dot.
- Open Archaic in the Finder: click its icon, then File > Open (Cmd-O). From the floppy it takes about 10 s to start.
  Its window fills the screen under the menu bar.
- **Cmd-Q** quits (or Ctrl-Q, or the close box).
- In Snow, the Mac's `127.0.0.1` is your computer's. So Archaic looks for WRP at `http://127.0.0.1:8080/`
  with no options.
- Options, like WRP's address: the Mac has no command line, so they go in a text file `Archaic Options`
  in Archaic's folder. One line, as on Unix: `-c 16 10.0.0.1:8080`.
  System 6 has no text editor that saves a file. Put it in the floppy image on your computer with
  hfsutils (the Retro68 image has them):

  ```
  printf '127.0.0.1:8097\n' > opts.txt
  docker run --rm --platform linux/amd64 -v "$PWD":/w -w /w ghcr.io/autc04/retro68 sh -c \
    'hmount Archaic.dsk && hcopy -t opts.txt ":Archaic Options" && hattrib -t TEXT -c ttxt ":Archaic Options" && humount'
  ```

- Set the screen to 256 colors in Control Panel > Monitors: Archaic asks WRP for 216 (the web colors).
  In black and white, WRP sends ABM.
- `screenshots/system6.png`: the Mac's screen, 640 x 480 in 256 colors, with Wikipedia's Macintosh IIcx article.

**Measured in Snow** (a IIcx at its real speed: a 16 MHz 68030), from Return until the picture is complete:

| What | Time |
|---|---|
| WRP start page | 3 s |
| Wikipedia's Macintosh IIcx article, black and white (ABM, a slice of 4 KB) | 2 s, WRP's 2 s included |
| the same, 216 colors (AB8, a slice of 12 KB) | 3 s: WRP 2 s, then the Mac 1.2 s for the slice (Archaic 0.11, as a GIF: 2.5 s) |
| Wikipedia's Apple Inc. article, after a click on its link | 14 s: WRP 10 s |

## Building for the classic Mac OS

Archaic for the Mac is built on your computer, with [Retro68](https://github.com/autc04/Retro68)
(gcc 16.1 for the 68k Mac) in Docker:

```
make mac        # or: sh src/mkmac
```

- It needs Docker, and the image `ghcr.io/autc04/retro68` (5.6 GB, amd64: on Apple silicon Docker runs it with Rosetta).
- It makes `src/build-mac/Archaic.dsk` (an 800 KB floppy image) and `Archaic.bin` (MacBinary), about 160 KB.
  The build folder is `src/build-mac`: git ignores it.
- `CMakeLists.txt` names `main.c`'s `main` `archaic_main`: `mac.c` has the Mac's `main()`.
- `archaic.r` is the SIZE resource: 3 MB, at least 1.5 MB, and the flags for multitasking
  (see Multitasking on the classic Mac OS).
- The same files run on System 6.0.8 and Mac OS 7.5.5. Not tested: a Mac with no Color QuickDraw
  (Plus, SE, Classic), MultiFinder on System 6.

## Mac OS 6: what to know

Test on a copy of your Mac's disk.

| Thing | In Snow |
|---|---|
| Computer | Macintosh IIcx: 68030 at 16 MHz, 8 MB |
| Screen | Macintosh Display Card: 640 x 480, up to 256 colors (Monitors) |
| System | System 6.0.8, the Finder (no MultiFinder) |
| Network | MacTCP 2.0.6 on Snow's DaynaPORT. Snow's NAT: the Mac 10.0.0.2 (by RARP), the gateway 10.0.0.1. **Your computer = 127.0.0.1** |

- Retro68: `int`, `long` and pointers are 32 bits; it defines `macintosh`. All Toolbox names are in
  `<Multiverse.h>` (the Multiversal Interfaces). They have MacTCP's TCP, not its name resolver: `dnr.c` has it.
- In the Multiversal Interfaces, `WindowPtr` is a `GrafPtr`: cast it to `WindowPeek` for `updateRgn`.
  There is no `geneva`: its font number is 3. The Control key's flag is `ControlKey`.
- There is no `chmod`, `access` or `getuid`. `update.c` needs them only with a `SYSID`: the Mac build
  has none, so it has no updates.
- `main()` in `mac.c` starts the Toolbox, reads `Archaic Options` (in the current folder: the application's,
  when the Finder starts it), and calls `main.c`'s main.
- Text is Geneva 9. Archaic's text is Latin-1, the Mac's is Mac Roman: `mac.c` changes the letters both ways.
- Control and a letter: `mac.c` makes it the control code. Without that, Ctrl-U did nothing in System 6.0.8.
- MacTCP: each call is `PBControlAsync`, then `SystemTask` while it waits, for 2 ticks at most. After that,
  `mac_wait` (see Multitasking on the classic Mac OS). The parameter blocks are static.
  Archaic releases the stream before it quits (`atexit`): else MacTCP writes into memory that is no longer Archaic's.
- MacTCP picks local ports from a small range (about 1024 to 2047), and it gave a new connection
  the port of the one that had just closed. Snow's NAT then dropped its SYNs for 13 s, and a server
  in TIME_WAIT may refuse it too. So Archaic picks its own ports: 20000 to 39999, the next one each time.
- `n_read` waits at most 2 ticks for data, in `WaitNextEvent`: an event for Archaic ends the wait.
- `g_copy` copies in the window only when no other window covers it (its visRgn).
- `g_pix`: a PixMap with its own color table, with a new seed when the palette changes.
  Color QuickDraw maps the colors to the screen's.
- The scroll bar's values are 16 bits: a long page is counted in steps of more than 1 pixel.
- Snow with its MCP tools: `snowemu --mcp FILE.snoww`. The tools have the file's name as the VM's name.
  - A click right after the start can come while Archaic loads WRP's first page, and is lost: wait for the page.
  - The wait-screen tool can miss a small change (a few letters): look at the screenshot.
  - Shut the Mac down (Special > Shut Down) before you quit Snow.
- Snow's View menu: Display scale 1.0 and Scaling algorithm NearestNeighbor show the Mac's pixels 1:1.
  (1.5 and Linear blur them.)

## Running on Mac OS 7.5

- **Mac OS 7.5.5** on the same Macintosh IIcx in Snow, with Open Transport and the DaynaPORT driver.
- Release files: `archaic-macos7.dsk` (a floppy image) and `archaic-macos7.bin` (MacBinary). They are the same
  files as `archaic-macos6.*`: one build runs on both. It shares the CPU with the other programs
  (see Multitasking on the classic Mac OS).
- Open Archaic in the Finder. Its window fills the screen under the menu bar, less the column of
  disk icons at the right.
- **Cmd-Q** quits. The Finder's Shut Down and Restart quit it too.
- WRP's address: Open Transport has its own `127.0.0.1`, the Mac itself. So in Snow the Mac can't
  reach your computer's `127.0.0.1`. Snow's NAT connects to the address that the Mac asks for, from your computer:
  give WRP's address on another network of your computer (the host-only network of a VM program is good)
  in `Archaic Options`. WRP listens on all addresses by default (`-l :8080`).
- A name for WRP: a text file `Hosts` in the System Folder, with a line like `wrp.test A ADDRESS`.
  Open Transport reads it, also for Archaic (MacTCP's resolver, `dnr.c`).
- SimpleText opens `Archaic Options`, and saves it. In Snow, an edit on a floppy is lost when the Mac ejects
  the floppy (Put Away, Restart, Shut Down): put the file in the floppy image with hfsutils (see Running on Mac OS 6).
- `screenshots/macos75.png`: the Mac's screen, 832 x 624 in 256 colors, with Wikipedia's System 7 article.

**Measured in Snow** (the IIcx at its real speed), 832 x 624 in 256 colors, from Return until the picture is complete:

| What | Time |
|---|---|
| WRP start page | 3 s |
| Wikipedia's System 7 article, 216 colors (GIF, a slice of 735 x 528, 26 KB) | 9 s: WRP about 3 s, then the Mac about 6 s for the slice |

## Mac OS 7.5: what to know

Test on a copy of your Mac's disk.

| Thing | In Snow |
|---|---|
| Computer | Macintosh IIcx: 68030 at 16 MHz, 8 MB |
| Screen | Macintosh Display Card: 832 x 624 with Snow's 16" RGB monitor (`"monitor": "RGB16"` in the `.snoww` file), up to 256 colors |
| System | Mac OS 7.5.5 |
| Network | Open Transport on Snow's DaynaPORT, set by hand in TCP/IP: the Mac 10.0.0.2, mask 255.0.0.0, router 10.0.0.1, a name server |

- The disk: Snow boots a disk with a partition map and a driver, not a bare HFS volume (`.hfv`).
  Put a partition map and a driver in front of it: here the first 96 blocks (48 KB) of a System 6 disk image
  (`.hda`) whose HFS partition (at block 96) has the same size.
- The DaynaPORT driver: its Easy Install stops (`The file "Network" ... cannot be replaced`).
  Custom Install of "DaynaPORT SCSI/Link" alone works. Open Transport then calls it "Alternate Ethernet".
- A new monitor in Snow starts in black and white: set 256 colors in Monitors again.
  Its Options (click with Option down) has the gamma: "Page-White Gamma" shows white as cream,
  "Uncorrected Gamma" as white.
- Snow at uncapped speed: the Mac's clock runs fast too (the status line's times are too long),
  and a double-click can come as two clicks. Use accurate speed for times and double-clicks.

### Multitasking on the classic Mac OS

On System 7 (and with MultiFinder) the programs share the CPU only when each one asks for its next event
(`WaitNextEvent`, `GetNextEvent`, `EventAvail`). `SystemTask` alone gives the other programs no time.

- Archaic waits only in `WaitNextEvent`, with a sleep: `g_event` (30 ticks in front, 60 in the background,
  2 while a part of the scroll bar is held), and `mac_wait` for the network (`nmactcp.c`, `dnr.c`).
  Events for Archaic that come in `mac_wait` go to its queue.
- A picture in colors takes seconds to decode on a 68030: `YIELD` in `img_rows` (`main.c`) calls
  `mac_wait(0)` each 4 ticks (1/15 s). On the other systems `YIELD` is empty.
- `archaic.r`: `acceptSuspendResumeEvents`, `doesActivateOnFGSwitch`, `isHighLevelEventAware`, `canBackground`.
  Suspend and resume: the scroll bar and the grow box go inactive and active.
  The click that brings Archaic to the front does not go to the page.
- Apple events (where `Gestalt` says they are there): Quit quits (the Finder's Shut Down and Restart send it),
  Open Application does nothing, Open and Print Documents are not handled.
- Measured: Note Pad in front, and Archaic behind it loading and decoding Wikipedia's Apple Inc. article
  (216 colors). 150 keys typed 60 ms apart: Note Pad got all 150. The Archaic before this (no yields)
  lost 2 of them: the Mac's event queue was full while it decoded.

## Building for the Atari (FreeMiNT, GEM)

Archaic for FreeMiNT with GEM (XaAES), as in the ARAnyM kit `freemint119-aranym.tar.lz`,
is built on your computer, with Vincent Riviere's m68k-atari-mint cross tools in Docker:

```
make mint       # or: sh src/mkmint
```

- It needs Docker. The first run makes the image `archaic-mint`: Ubuntu 24.04 with `ppa:vriviere/ppa`
  (gcc 4.6.4, MiNTLib, GEMLib), amd64: on Apple silicon Docker runs it with Rosetta.
- It makes `archaic-mint` (about 230 KB): a MiNT program for a 68000 or newer, with a 128 KB stack.
  Git ignores it. The system name is `mint` (`__MINT__`): the release file is `archaic-mint`.
- In FreeMiNT, put it in a folder on the PATH as `archaic`. In the ARAnyM kit the PATH is `/sbin:/bin:/usr/sbin:/usr/bin`
  and there is no `/usr/local` (kits before 2026-11-26 v4 have no `/usr` at all): use `/bin/archaic`
  (on the host: `drive_c/mint/1-19-2af/sys-root/bin`). Copy it in through a HostFS drive (C: is the folder `drive_c`).
- Start it from a shell: Desk > TosWin2, File > Start shell, then `archaic &`.
  Under XaAES a program from a shell opens GEM windows.
- WRP's default address is `10.0.2.2:8080`: in ARAnyM's slirp, that is port 8080 of the host.
  Start WRP on the host with `wrp -l 127.0.0.1:8080`.

## FreeMiNT: what to know

- `gem.c` draws into `fb.c`'s screen in memory (0xRRGGBB). `g_flush` copies the changed part to the
  window with the VDI, through GEM's rectangle list (`wind_update` locks the screen meanwhile).
- The screen's pixels come from `vq_scrninfo`:
  - packed 32-bit 0x00RRGGBB (ARAnyM's fVDI, `mode ...x32` in `fvdi.sys`): `vro_cpyfm` takes `fb`'s rows
    as they are, when the window is a multiple of 16 pixels wide; else they are copied to a buffer first.
  - other packed pixels (15, 16 and 24 bits, also Intel byte order): through tables made from the
    bit numbers that `vq_scrninfo` gives.
  - bit planes, and pixels with a palette (2 to 256 colors): black and white with `vrt_cpyfm`.
    `g_colors()` is then 2, and WRP sends ABM pictures. No palette is set.
- The window has GEM's scroll bar (`g_bar` 0). Its arrows send `S_STEP`, its grey part `S_PAGE`,
  its slider `S_TO` (GEM's 0 to 1000, times the page's max). `g_sbar` sets the slider only when it changes.
- Keys: the up and down arrows scroll a quarter, with Shift a window (also a PC's Page Up and Page Down).
  ClrHome goes to the top, Shift+ClrHome to the end. Undo is Esc.
  The Atari's characters 0200 to 0377 become ISO 8859-1 (`latin[]`); the window's title goes the other way.
- The mouse: `evnt_multi` with `MU_BUTTON` (any button down, then all up) and `MU_M1` (the mouse moves
  while a button is down) gives `E_DOWN`, `E_MOVE` and `E_UP`. A click outside the work area, or on
  another window over Archaic's, gives none.
- Quit: Ctrl-Q, the close box, or `AP_TERM` (`shel_write(9, 1, ...)` says Archaic knows it).
- GEM has no file for `select()`: `g_fd()` is -1, so `nsock.c` waits 1/20 s at most, and
  `ui_poll()` takes GEM's events with no wait.
- `wind_set_str(WF_NAME)`: the AES keeps the pointer, so the title is a static buffer.
- Tested (2026-11-26): ARAnyM 1.2.0 Unofficial Beta 3, FreeMiNT 1.19 with XaAES and TeraDesk,
  fVDI 1280x1024x32, 1024x768x16 and 1024x768x8 (black and white), WRP 4.14.2:
  pages in colors (AB8), links, typing in the address box, Ctrl-U, the scroll bar (arrows, grey part,
  slider), the arrow keys, the fuller and the sizer (WRP makes the page again for the new width),
  the close box and Ctrl-Q.
  Not tested: a real Atari, MagiC, N.AES, an ST's monochrome screen, 24-bit screens.

## Running on A/UX 3

- **A/UX 3.1.1** (Apple's Unix, with the System 7 Finder) on a Quadra 800 (68040), in QEMU (LEMU).
  Kit: `aux311q800-lemu`, on osarchive in `os/a-ux/Virtual/qemu`. It has A/UX's C compiler and the A/UX Toolbox.
  The kit has Archaic in `/usr/local/bin` since 2026-11-26. There is no release file yet: elsewhere, build it
  (see Building on A/UX 3).
- Archaic is **two files**: `archaic`, the program, and `%archaic`, its resources. Keep them in one folder:
  without `%archaic` the A/UX Toolbox does not start the program (it says `FOLDER: No such file or directory`).
  Install: `cp archaic %archaic /usr/local/bin`, then `chmod 755 /usr/local/bin/archaic`.
  An update (see Updates) replaces only `archaic`: `%archaic` stays.
- Log in to the Finder. Apple menu > CommandShell, then type `/usr/local/bin/archaic` (root's sh has no
  `/usr/local/bin` on its PATH; bash has it). A telnet login as root, with root
  logged in to the Finder, starts it too: the window comes on the Finder's screen.
- **Cmd-Q**, Ctrl-Q or File > Quit quits.
- Archaic looks for WRP at `http://192.168.1.10:8080/`: your computer, in the kit's network (QEMU NAT).
  On another network (a bridged VM, a real Mac): `archaic HOST:8080`, or set `WRP` to `HOST:8080`.
- The screen has 256 colors: the pictures have the 216 web colors (AB8).
- `screenshots/aux.png`: the Quadra's screen, 1152 x 870 in 256 colors, with Wikipedia's A/UX article.

**Measured in LEMU** (QEMU on an Apple Silicon Mac), with WRP on the same Mac:

| What | Time |
|---|---|
| `sh mkaux` (or `make`) | 3 s |
| Wikipedia's A/UX article, 216 colors (AB8, slices of 56 to 67 KB) | 3 s from Return: WRP's time. A slice comes and decodes in 1 or 2 ticks (17 to 33 ms) |

## Building on A/UX 3

Needs A/UX's C compiler, `cc`, and the A/UX Toolbox's files for programs: the headers in `/usr/include/mac`,
`/usr/lib/libmac_s.a`, `/usr/lib/maccrt0.o`, `low.o` and `low.ld`, and `/mac/bin/rez` and `fcnvt`.
The full A/UX install has them, and so does the kit.

1. Copy these 11 files into a folder on A/UX, with NFS, ftp or rcp:
   `src/archaic.h`, `main.c`, `html.c`, `http.c`, `gif.c`, `pic.c`, `update.c`, `nsock.c`, `mac.c`, `mkaux` and `auxrsrc.r`.
   With the whole repository, `make` at its top runs `mkaux`, and moves `archaic` and `%archaic` up.
2. Build:

   ```
   sh mkaux        # compile and link: archaic, about 232 KB, and its resources: %archaic, 832 bytes
   ```

   It takes 3 s in the kit. `cc` warns `& before array or function: ignored` 4 times (`&qd.black`): that is right.
3. Test: `./archaic --version` says `Archaic v0.11 for aux3`. `./archaic -t` (no window) loads WRP's form and lists its parts.
   **With the window:** `./archaic` in CommandShell.

NFS from your computer: A/UX's `mount` takes a host name from `/etc/hosts`, not an address
(`192.168.1.10 not in hosts database`). Put a line `ADDRESS NAME` in `/etc/hosts`, then
`/etc/mount -t nfs NAME:/dir /mnt`. Archaic runs from an NFS folder too.

## A/UX 3: what to know

Test on a copy of your A/UX disk, or start QEMU with `-snapshot`.

| Thing | A/UX 3.1.1 kit |
|---|---|
| Computer | Quadra 800 in QEMU (LEMU, `-M q800`): 68040, 128 MB |
| Screen | 1152 x 870, 256 colors |
| System | A/UX 3.1.1: Unix (System V with BSD parts) and the System 7 Finder |
| Network | SONIC Ethernet (`-nic ...,model=dp83932`), BSD sockets. QEMU NAT: A/UX 192.168.1.2, **your computer 192.168.1.10** (its 127.0.0.1). No default route |

- The A/UX Toolbox: an A/UX program (COFF) calls the Mac's ROM and System 7 through `libmac`.
  The manual: *A/UX Toolbox: Macintosh ROM Interface* (A/UX 3.0, 1992; on bitsavers, `pdf/apple/mac/a_ux/aux_3.0`).
  The display is `mac.c`, as on Mac OS 6 and 7.5, built with `-DAUX`. Apple's examples: `/mac/src/examples`.
- `cc` is K&R: `-DNOPROTO`. `-B /usr/lib/big/` gives it larger tables, as Apple's makefile does.
  It does not define `AUX`: `mkaux` does, as Apple's makefile.
- The headers are A/UX's own, included as `<mac/quickdraw.h>` and so on. Some names are older ones, and `mac.c`
  maps them: `GetItem`, `AddResMenu`, `SetCtlValue`, `SetCtlMax`, `GetCtlValue`, `DisposHandle`, `controlKey`,
  `app4Evt`, and `transIndex` in a color table for `ctFlags`. `qd` is a macro (`*__tb_qdVars`).
  There is no `GetMBarHeight` (`mac.c` takes 20) and no Apple events.
- Without `_AUX_C_EXTENSIONS` (the c89 compiler of A/UX Developer's Tools) the headers have no prototypes
  and no `pascal`. The calls take Pascal strings, and `Point` by value, as on the Mac.
- The link, as in Apple's makefile: `ld /usr/lib/maccrt0.o /lib/crt2.o ... /usr/lib/low.o -lmac_s -lat -lld -lmr -lc_s /lib/crtn.o /usr/lib/low.ld`.
  The sockets are in `libc_s`.
- The resource file: `rez` makes `Rez.out` (AppleSingle) from `auxrsrc.r`, and `fcnvt -f -d` makes an AppleDouble
  pair of it: `%archaic` is the file with the resources. A/UX's `types.r` has the older SIZE flag names
  (`multiFinderAware` ...), so `archaic.r` (Retro68's) does not go through A/UX's `rez`.
  The Mac heap is small (512 KB): Archaic's pages and pictures are Unix memory (`malloc`).
- Sharing the CPU: the manual says to wait in `WaitNextEvent`, which lets the other programs run, not in `select()`.
  So `nsock.c` waits in `mac_sel` (`mac.c`): `ui_setselect` makes `WaitNextEvent` return when the socket can be read
  (or written, for the connect). Then `select()` looks with no wait. The masks are cleared before the event is handled.
  `g_fd()` is -1. `YIELD` is as on the Mac.
- `maccrt0` does not flush stdio after `main` returns: `mac.c`'s `main` flushes `stdout` and `stderr`
  (without it, `--version`, `--help` and `-t` print nothing into a pipe or a file, such as through rsh).
- `noCD = 1` (a variable of `libmac`): the current folder stays the shell's (for `-o FILE`). Else it is the program's folder.
- `TickCount` runs as on the Mac: `g_ms` is right to a tick (17 ms). A slice that comes in less shows `under 1 ms`.
- LEMU's mouse tools: A/UX speeds the pointer up, so moves land short. Use `pointer-locate`, then `mouse-move`
  with `step-px` 1, then `mouse-click` with no x and y. Menus: `mouse-button` down, move, up. Cmd is the key `meta_l`.
- Tested (2026-11-26): A/UX 3.1.1 in LEMU, on the LAN (bridged), with WRP 4.14.2 on the Mac's LAN address.
  Started from CommandShell and from telnet, as root. Pages in 216 colors (AB8), a click on the page, the address box,
  Ctrl-U, Page Down, the scroll bar's arrow (held, it repeats), the Finder in front while a page loads
  (Archaic goes on, and draws its page behind the Finder's windows), the click that brings Archaic back (it does not
  go to the page), File > Quit, Cmd-Q, Ctrl-Q, `-t`, `-u` (`WRP has no update: no archaic-aux3 in release v0.11`).
  Not tested: a double-click in the Finder, the close box, the grow and zoom boxes, a drag of the scroll box, Edit > Paste,
  the About box and the desk accessories, a screen in black and white, the sums of the test pictures
  (`-t` with `test/data`, which needs a web server), gcc, a real Mac.

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

## Roadmap: poor systems with no web browser

Old systems that never had a web browser: the next ports.
Each port is a display file, plus a network file if the system has no BSD sockets (see Porting).

| Target | What it is | Systems |
|---|---|---|
| MGR | a window system on a terminal line | SunOS, Coherent, Xenix, Minix, OS-9, VSTa |
| Blit, DMD 5620 | a graphics terminal on a serial line | Research Unix (System V on the AT&T 3B2, with layers: done) |
| QNX Windows | QNX's window system before Photon | QNX 2 |
| SunWindows, SunView | Sun's window system before X11 | SunOS 1 to 4, on Sun-1 to Sun-3 (SunOS 4 on a Sun-4, SunOS 3.5 on a Sun-3: done) |
| VWS, EWS | DEC's window systems before X11 | VAX/VMS (VWS), VAXELN (EWS) |
| NeWS, Display PostScript | PostScript displays | SunOS, SGI, OS/2, VMS, NeXTSTEP, AIX ... |
| Windows 1 and 2, OS/2 1.x | early PC window systems: TCP/IP over SLIP | Windows 1.0 and 2.x, OS/2 1.1 to 1.3 |
| Tektronix 4010, 4014, 4105 | graphics terminals on a serial line: lines on a storage tube, or pixels (4105) | any host with a network: 2.11BSD, 4.3BSD, VMS ... |

### MGR

[MGR](https://en.wikipedia.org/wiki/ManaGeR) (ManaGeR) is a window system by Stephen Uhler
at Bellcore, from 1984. It started on Sun workstations.
A program in an MGR window writes escape sequences to its terminal:
text, lines, bitmaps, fonts, menus. Events, like a mouse click, come back as text.

- Plan: one display file, for all the systems with MGR.
- MGR works on any terminal line, also telnet or a modem. So Archaic can run
  on another computer (one with a network), and show in an MGR window.
- Source: MGR 0.69, at [hack.org/mc/mgr](https://hack.org/mc/mgr/).

| System | MGR there |
|---|---|
| SunOS (Sun-3, Sun-4) | the first system. Network: BSD sockets, as `nsock.c` |
| Coherent (PC) | MGR 0.69, Harry Pulley's port |
| Xenix | an older MGR |
| Minix (386) | an older MGR |
| OS-9, VSTa | MGR ran there too |

To find: TCP/IP on Coherent, Xenix, Minix, OS-9 and VSTa.

### Blit and DMD 5620

The [Blit](https://en.wikipedia.org/wiki/Blit_(computer_terminal)) is a graphics terminal
by Rob Pike and Bart Locanthi at Bell Labs, 1982.
AT&T and Teletype sold it as the **DMD 5620** (1984): a WE 32100 CPU, 800 x 1024 pixels, 1 bit, green.
It connects to a Unix host on a serial line, and the host loads programs into it.
Its window system is mpx or mux (Research Unix), or layers (System V Release 3).

- Done: the DMD 5620 in layers, on the AT&T 3B2 with UNIX System V Release 3.2
  (`dmd.c`, `dmdterm.c`: see Running on the AT&T 3B2 with a DMD 5620). Archaic is in two parts.
  On the host: the network, the HTML and WRP's form. In the terminal: a program that draws,
  keeps and decodes the page's picture, and reads the mouse.
- Next: mpx or mux on Research Unix (V8, V9, V10), with the Blit or the DMD 5620. The terminal's part
  needs their library, and the host's part their tty.
- Emulators: the Blit in 9front, with mux on Research Unix v8 (SIMH).
  For the DMD 5620: [Seth Morabito's emulator](https://loomcom.com/3b2/dmd5620_emulator.html),
  made for the AT&T 3B2/400 in SIMH. It also works with mux on Research Unix V8, V9 and V10 (firmware 8;7;3).

### QNX 2 with QNX Windows

QNX 2 (Quantum Software Systems, 1987) is a small real-time system for the PC (8088, 286).
QNX Windows is its window system, before Photon.
[QNX Windows, first look](https://virtuallyfun.com/2013/03/22/the-shape-of-things-to-come/)
(virtuallyfun, 2013): QNX 2.2 in QEMU, 800 x 600 (`-vga cirrus`, `qw.vga_bios video7,1`).
The mouse did not work in QEMU.

- Plan: a display file for QNX Windows.
- QNX 2 is 16-bit: see the memory note in Porting.
- To find: the QNX Windows library, and TCP/IP for QNX 2.

### SunWindows and SunView: Sun-1 to Sun-4

Sun's window system before X11. **SunWindows** came with SunOS 1.0 (1983), on the Sun-1 and Sun-2.
**[SunView](https://en.wikipedia.org/wiki/SunView)**, its toolkit, came with SunOS 3.0,
and stayed up to SunOS 4.1: Sun-2, Sun-3, Sun-4.
OpenWindows (X11) took its place, and still showed SunView programs.

- Done: SunView on SunOS 4.1.4, on a Sun-4 (`sunview.c`, see Running on SunOS 4 with SunView),
  and on SunOS 3.5, on a Sun-3 (see Running on SunOS 3.5 on a Sun-3). Network: `nsock.c`.
- Next: the Sun-2 (68010), with the same `sunview.c`.
  Then SunWindows on SunOS 1 (Sun-1, Sun-2), with no SunView toolkit.

### VAX before X11: VWS and EWS

DEC's window systems before DECwindows (X11):
**VWS** (VAX Workstation Software) on VMS, with its library **UIS**,
and **EWS** on VAXELN, DEC's real-time system.

- Computers: VAXstation I and II (QVSS, black and white), II/GPX (QDSS, color), 2000, 3100.
- Emulator: SIMH. `microvax2` with `SET CPU MODEL=VAXstation` is a VAXstation II.
  [MicroVMS 4.7 and VWS 3.3 work there](https://gunkies.org/wiki/Installing_MicroVMS_V4.7_and_VAX_Workstation_Software_V3.3_on_SIMH).
- Plan: a display file for UIS.
- Network on VMS: TCP/IP from Wollongong (`ntwg.c` is for their 3B1 TCP/IP), CMU-Tek,
  TGV (MultiNet) or DEC (UCX).

### PostScript displays: NeWS and Display PostScript

A program sends PostScript to the display, and the display draws it.
So one display file can work on many systems:
the picture as a PostScript `image`, the text with `show`.

- [NeWS](https://en.wikipedia.org/wiki/NeWS) (Sun, 1986, James Gosling and David Rosenthal): SunOS.
  Ports: SGI's 4Sight, NeWS/2 (OS/2), MacNeWS, TGV's X11/NeWS (VMS on a VAXstation 2000),
  the Amiga 2000, Apollo and others. Sun's OpenWindows 3.3 (1993) dropped it.
- [Display PostScript](https://en.wikipedia.org/wiki/Display_PostScript) (Adobe and NeXT, 1987):
  NeXTSTEP, DEC's DECwindows, IBM and SGI workstations, Sun's OpenWindows 3.3.
  On most of them, it works in X11.

### Windows 1 and 2, OS/2 1.x: TCP/IP over SLIP

Early window systems on the PC. They are 16-bit, and have no TCP/IP of their own:
**Windows 1.0** (1985) and **Windows 2.x** (1987),
and **OS/2 1.1 to 1.3**, with Presentation Manager (from 1988). OS/2 1.0 had text mode only.

- Plan: a display file for Windows (GDI), and one for Presentation Manager.
- Network: SLIP on the serial port, with a small TCP/IP in Archaic, as netd does in Idris.
  The same code for both systems: only the serial port calls change.
  Archaic opens one connection at a time (`http.c`), so the TCP part can stay small.
- On the host: a SLIP bridge to libslirp, as `slirpbridge` in the Idris kit. The host is then 10.0.2.2.
- 16-bit: see the memory note in Porting.

### Tektronix terminals: 4010, 4014 and 4105

The [Tektronix 4010](https://en.wikipedia.org/wiki/Tektronix_4010) (1972) is a graphics terminal on a serial line.
Its screen is a storage tube: a line stays on the screen after it is drawn, with no screen memory.
It draws text and lines (vectors), green on dark, on 1024 x 780 points.
It can erase only all of the screen.
The 4014 has a larger screen, with 4 times more points in each direction.
Many systems drew on them: Tektronix's PLOT 10 library, Unix's `plot -T4014`.
The [4105](https://en.wikipedia.org/wiki/Tektronix_4105) is a color raster terminal (480 x 360 pixels).
It also knows the 4010's commands.

- Plan: one display file for the Tek commands. As with MGR, Archaic runs on the host
  (one with a network), and sends the commands on the terminal line.
- Pictures on the 4010 and 4014: each run of black pixels in a row is one line.
  The page shows bright on dark.
- Pictures on the 4105: its pixel operations, RASTER WRITE or RUNLENGTH WRITE
  (see the [4105 Programmer's Reference](https://vtda.org/docs/computing/Tektronix/070-4526-03_4105ComputerDisplayTerminalProgammersRef_Feb86.pdf), 1986).
- Clicks: GIN mode. The user moves the crosshair (with two thumbwheels on the 4010) and presses a key.
  The terminal sends the key and the place.
- Scrolling: erase the screen, then draw it again.
- Emulators: xterm's Tek window (a 4014: `xterm -t`), and René Richarz's
  [tek4010](https://github.com/rricharz/tek4010) (4010 and 4014), used with 2.11BSD and RSX-11 in SIMH.
  Many emulators do not have the 4105's pixel operations.
- To find: how long one picture takes on a 9600 baud line. A page of text has many runs of black pixels.
- 2.11BSD (PDP-11) is 16-bit: see the memory note in Porting.

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
  until the pointer is in it (see SunOS 4 and SunView: what to know).
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
