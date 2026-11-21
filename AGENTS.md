# Archaic: notes for agents

The details: how Archaic works with WRP, the facts of each old system, building, tests, porting.

README.md is for people. Keep it short: what Archaic is, screenshots, platforms, quick start, build.
Put new details here, not in README.md.

Test on a copy of a system's disk, never on the user's own disk.

## What Archaic shows

| What Archaic shows | What it skips |
|---|---|
| text fields, password fields | JavaScript, CSS, HTTPS (WRP does all that) |
| buttons, drop-down menus (SELECT) | check boxes, radio buttons, TEXTAREA |
| links, page title, line breaks | tables, fonts, colors |
| GIF and ABM pictures, clickable (ISMAP) | PNG, JPEG (Archaic asks WRP for GIF or ABM) |

## Files

| File | What it is |
|---|---|
| `src/archaic.h` | everything shared, and the C rules for old compilers |
| `src/main.c` | the page on the screen: layout, drawing, mouse, keys, forms |
| `src/html.c` | HTML reader: forms, menus, links, images, text |
| `src/http.c` | URLs and HTTP/1.0, with a 16 KB read buffer |
| `src/gif.c` | GIF decoder: the colors, or black and white with a 4x4 dither. WRP's black and white GIFs go a fast way |
| `src/pic.c` | pictures: looks at what came (GIF or ABM); the ABM decoder |
| `src/x10.c` | display for X10 (Idris) |
| `src/nidris.c` | network for Idris: TCP through netd |
| `src/x11.c` | display for X11 (Unix, macOS with XQuartz) |
| `src/cocoa.m` | display for macOS: a Cocoa window |
| `src/sdl.c` | display for SDL 2 (macOS, Linux, Windows ...) |
| `src/headless.c` | display with no window, for tests and automation: commands on standard input, screenshots |
| `src/fb.c`, `fb.h` | a screen in memory, for `cocoa.m`, `sdl.c` and `headless.c`; writes PNG screenshots |
| `src/font6x13.h` | the X11 6x13 font (public domain), for `fb.c` |
| `src/nsock.c` | network for BSD sockets (Unix, macOS) |
| `src/fetch`, `mkinc`, `mk`, `lka` | Idris scripts: get the sources, headers, compile, link |
| `src/unixpc.c` | display for the AT&T UNIX PC (3B1) window system |
| `src/ntwg.c` | network for the 3B1: the BSD sockets of WIN/TCP (TWG) |
| `src/mkunixpc` | 3B1 script: compile and link with gcc |
| `src/sunview.c` | display for SunView (SunOS 3 and 4) |
| `src/mksunview` | SunOS 3 and 4 script: compile and link with Sun's cc |
| `tools/idris.py` | host tool: run commands in Idris over telnet, serve files |
| `tools/hget.c` | Idris tool: download one file over HTTP (through netd) |
| `tools/pcf2c.py` | host tool: an X11 PCF font as a C table (it made `font6x13.h`) |
| `test/run.sh` | tests: GIF decoder, HTML, forms, WRP, the headless display |
| `test/xshot.sh` | test: run the X11 build and take a screenshot |
| `test/data/` | GIFs saved from WRP, for the tests; two of them also as ABM |
| `test/cfacts.c`, `cfacts2.c` | probe for a new port: what its C compiler and linker do (name length, case, struct copies) |
| `screenshots/` | the pictures in README.md |

## Options

```
archaic [=WxH+X+Y] [-a] [-c n] [-f fmt] [-t] [-v] [-o file] [-s script] [url]
```

| Option | Meaning |
|---|---|
| `url` | WRP's address, like `10.0.2.2:8080`. Default: `$WRP`, else port 8080 on the host (Idris, the 3B1 and the Sun-3: `10.0.2.2`, SunView on SPARC: `192.168.1.10`, Unix: `127.0.0.1`) |
| `=WxH+X+Y` | window size and place (also `-g WxH+X+Y`). Default on Idris and SunView: about 9/10 of the screen, in the middle. On the 3B1: 686 x 248 inside a frame; a larger size gets no frame, up to 720 x 288 (`-g 720x288`) |
| `-a` | also show WRP's M, T and C menus, and send them as they are set |
| `-c n` | start with pictures of n colors (2 to 256), not as many as the display shows (see Colors). With `-t`: as if the display showed n colors |
| `-f fmt` | `gif`: ask WRP for GIFs only. `abm` (as without `-f`): WRP may send its black and white pictures as ABM (see Pictures: GIF and ABM) |
| `-t` | test mode: no window. Prints the page items, and for each picture what it came as (`gif`, `abm`) and a checksum |
| `-v` | status messages also go to stderr |
| `-o file` | at the end, save the first picture: PBM, or PPM if it has colors |
| `-s script` | play a script, then quit (see Scripts, in Building in Idris) |

## WRP's form

**WRP's settings are set by Archaic, and not shown:**

| WRP box | Archaic sends |
|---|---|
| W, H | the window's size: the picture fills the window. Always, also with `-a` |
| M (mode) | ISMAP |
| T (picture type) | GIF. WRP sends its black and white pictures (2 colors) as ABM in its place (see Pictures: GIF and ABM) |
| C (colors) | black and white display: 2 (or `-c n`), not shown. A display with colors: the menu shows (see Colors) |
| SL (slices) | 1: the whole page, in slices (see Scrolling). An older WRP ignores it |

So WRP's form is one line: the address box, **Go**, WRP's buttons, **Z** (zoom),
**C** (colors, only on a display with colors), **D** (dither, only for black and white) and **K** (keys).

**D** says what WRP dithers in a black and white picture (a WRP of November 2026 or newer has it):

| D | The picture |
|---|---|
| All | all of the page is dithered. Good for photos. Text in a color, gray text and text on a color become dots |
| Pics | WRP's default, for reading: only photos are dithered. Text and lines are sharp, and the pictures are about 20% smaller |
| None | no dither at all. The smallest pictures |

Choose another one, and Archaic sends WRP's form at once with St: WRP makes the picture again.

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
| ⌫ | Bs | Backspace, for the web page. With text in K: the text, then Backspace |
| ↵ | Rt | Return (Enter), for the web page. With text in K: the text, then Return |

To type into a box on the web page: click the box in the picture, then type in **K**.

- Click ↵: WRP types the text into the page, then presses Return (to search, for example).
- Or press Return: WRP types only the text. Use this to fill in more boxes before you send the page's form.

(WRP alone does only the button's key when it gets both a button and keys, so Archaic sends
the text and the button's key together as keys: `k=text%0D`. This works with every WRP.)

## Colors

Archaic asks the display how many colors it shows.
The first status line says what it found, like `Archaic 0.5, millions of colors`.

| Display | Colors |
|---|---|
| 3B1, Idris (X10) | black and white (they are) |
| SunView | a color screen (8 bits, as the cg3): 128, in a colormap segment of Archaic's own. A black and white screen (bw2, as the Sun-3's bwtwo): black and white |
| X11 | the default visual. Depth 1: black and white. TrueColor: millions. A colormap (8-bit PseudoColor, as on old workstations): its size, like 256 |
| Cocoa, SDL | the screen's depth: millions |
| `archaic-headless` | millions, like SDL (`-c 2` for black and white) |

On a display with colors:

- WRP's **C** menu shows. It is set to the display's colors: the nearest of WRP's
  choices (256, 216, 128, 64, 16, 2) that is not more, or to `-c n`.
- Choose another number there, and Archaic sends WRP's form at once with St (stop).
  WRP takes the picture again in those colors, and does not load the page again.
  The frame stays where it was.
- The pictures keep their colors: a byte for each pixel, and the GIF's colors.
- SunView: Archaic's colormap segment has 128 places: white, black and 126 for the picture.
  A segment of 256 would take all of the screen's colors, and the other windows would lose theirs.
  When a picture has more colors than places (C at 256, or 128 with no pure black),
  the color nearest to another one gives up its place, until they fit:
  so a small red line keeps its red. The others take the nearest color.
- X11 with a colormap: Archaic gets the picture's colors in the default colormap,
  shared with the other programs. When the colormap is full, it takes the nearest color
  the colormap has. A new picture's colors free the last ones.

On a black and white display, the C menu is hidden. Archaic asks WRP for 2 colors
(or for `-c n`). The pictures are 1 bit a pixel; one with more colors gets a 4x4 dither.

Archaic's own start page request asks WRP for the same settings as the next ones
(`/?m=ismap&t=gif&c=256&sl=1`), so that WRP's form does not change after the first page.

## Pictures: GIF and ABM

A GIF has a code for every few pixels. An old CPU needs a long time for that:
a 10 MHz 68010 needs about 8 seconds for one picture of 551 x 310 pixels.
That was most of the time of a page, also on a slow line.

**ABM** is WRP's black and white picture for 1-bit displays. It is the display's own bytes
(1 bit a pixel), packed with a byte LZ. Archaic decodes it in whole bytes, with no work for each pixel.

- Archaic's requests have `Accept: image/x-abm`.
- A WRP that knows ABM (it has `abm.go`) then sends its black and white pictures (2 colors) as ABM,
  and pictures with more colors as GIFs. An older WRP sends only GIFs.
- On a display with colors, an ABM picture shows as it is: black and white (C set to 2).
- `-f gif` makes Archaic ask for GIFs only. Use it to compare the two on your computer:
  `archaic -t -f gif ...`, then `archaic -t ...`. The last status line has the time.
- WRP's black and white GIF (2 colors in its color table) goes a fast way in the GIF decoder too:
  about 5 times faster than before. Other GIFs are decoded a pixel at a time.

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

The ABM file format is in `src/pic.c`.

## Mouse and keys

**Mouse**

- Left button: click links, buttons, fields, menus, and the picture.
- Right button: scroll a window. Top half of the window = up, bottom half = down.
- The scroll bar at the right: see The scroll bar.

**Keys**

| Key | Does |
|---|---|
| Tab | next text field |
| Return | send the form |
| Backspace / Delete | delete the last character |
| Ctrl-U | clear the field |
| Esc | stop loading (or leave the field) |
| Space / b | scroll a window down / up (when no field has the keyboard) |
| Ctrl-Q | quit |

**On the 3B1**, its own keys and the window's frame work too:

| Key or frame part | Does |
|---|---|
| Exit, Cancl, the close box | quit |
| Page, Next / Shift+Page, Prev | scroll a window down / up |
| ↓ / ↑, the scroll arrows (also FreeBee's mouse wheel) | scroll a quarter of the window down / up. The frame's scroll arrows are the window's own scroll bar: Archaic draws none on the 3B1 |
| Rfrsh, Clear | draw the window again |
| the resize box | a new window size. A new width: WRP makes the picture again, as wide (see A new window size) |

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

On the 3B1, the help line at the bottom of the screen lists the main keys.

## The status line

**The status line** at the bottom of the window says what Archaic does, as it does it.
While Archaic loads what you asked for, a **spinner** turns at its left end: 8 dots in a circle.
It stops when the page is done (or stopped). It does not turn for the slices that Archaic gets
ahead, while you read.

| Status line | Means |
|---|---|
| `Page: connecting to 127.0.0.1 port 8080` | Archaic connects to WRP |
| `Page: waiting for 127.0.0.1, 3 s` | the request went out, and WRP makes the page. The seconds count up |
| `Page: 2 KB` | the page comes |
| `Picture 1 of 2: 12 of 31 KB` | a picture comes. The bar at the right shows how much has come |
| `Picture part 3 of 9: 4 of 7 KB` | a part (a slice) of the page's picture comes, for the frame |
| `Picture part 4 of 9 (ahead): 2 of 7 KB` | the next part comes, before you scroll to it |
| `2 s, 45 KB  http://...` | the page is done: the time it took, and how much came |

The bar shows when the server sends the size (WRP does for its pictures).
The spinner turns 8 steps a second by the display's clock (`g_ms`).
The 3B1 and Idris displays have no clock: there it steps each time the network waits
(1/10 s on the 3B1, up to 1 s on Idris).
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
- No page at all (WRP did not answer at the start): the status line says
  `can't connect to ... Return: try again`. Start WRP, then press Return.

## A new window size

Make the window wider or narrower: once its size has stayed the same for 0.4 s,
Archaic asks WRP for the page again, as wide as the window. It sends WRP's form with St,
so WRP does not load the page again: the browser lays it out at the new width, and WRP
takes a new picture. The frame keeps its place on the page, in proportion
(a narrower page is longer). Text you typed in a box stays.

A new height alone needs no new picture: the frame gets taller or shorter.
On the 3B1 and Idris (no clock), the new picture is asked for at once.

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

| Input | Scrolls the frame |
|---|---|
| a step of the wheel, ↑ ↓, the 3B1's scroll arrows and Roll keys | a quarter of the frame |
| Page Up, Page Down, Space, b, the right button, the 3B1's Page, Next, Prev | the frame, less 2 text lines |
| a trackpad (Cocoa, SDL) | by pixels |

An older WRP ignores `sl=1`: then the picture fills the window, and WRP's Up and Dn buttons scroll the page.

## The scroll bar

A system that has scroll bars uses its own. Where it has none, Archaic draws one.

| Display | Scroll bar |
|---|---|
| Cocoa (macOS) | the Mac's own (NSScroller), always shown. A click in its track scrolls as Page Down or Page Up, or jumps to that place, as set in System Settings (with Option, the other one) |
| the 3B1 | the window frame's scroll arrows |
| SDL, X11, X10 (the Atari ST), headless | Archaic's own, in the style of the Atari ST's GEM |

Archaic's own scroll bar is at the right of the window, 17 pixels wide (`screenshots/scrollbar.png`):

| Part | Does |
|---|---|
| ⇧ and ⇩ arrow boxes | scroll a quarter of the frame. Hold the button down: again every 1/10 s, after 4/10 s |
| the dotted shaft, above or below the elevator | scroll as Page Up or Page Down. Held down, it goes on until the elevator gets to the pointer |
| the white elevator | drag it. Its length shows how much of the page is in the window, its place where |

On X10 the elevator moves when the button is let go (as in GEM), and held parts do not repeat:
X10 has no events for the mouse moving with a button down, and can't wait for a time.

## Building and testing on Unix or macOS

```
make              # ./archaic, X11 + BSD sockets
make cocoa        # ./archaic-cocoa, a macOS window
make sdl          # ./archaic-sdl, SDL 2
make headless     # ./archaic-headless, no window: for tests and automation
make archaic-asan # X11 with address and undefined-behaviour checks
make knr          # syntax check as a pre-ANSI compiler sees it (-DNOPROTO)
make test         # sh test/run.sh
```

- `make` needs X11 (on macOS: XQuartz in `/opt/X11`).
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

```
printf '%s\n' 'click 20 12' 'type https://en.wikipedia.org/wiki/AT%26T_UNIX_PC' \
    'key return' 'shot page.png' 'scroll down' 'shot down.png' |
    ./archaic-headless -v -g 800x600 127.0.0.1:8080
```

`test/run.sh` checks:

1. GIF decoding gives **exactly** the same pixels as ImageMagick.
   ABM pictures give exactly the pixels of the GIF, and a broken ABM is refused.
   Archaic asks for ABM and GIF, and with `-f gif` only for GIF.
2. Gray and color GIFs decode at the right size.
   With colors (`-c 256`), the colors are **exactly** ImageMagick's.
3. HTML: title, entities, fields, menus, hidden fields, buttons, links, comments.
4. A button click sends the form with the right query.
5. WRP's form: W, H, M, T and C are hidden, and Archaic sends its own values (and with `-a`, the menus). WRP's 7 buttons are pictures, Go stays text. A new choice in WRP's D menu sends the form at once.
   With colors: C shows, set to the colors (for 100 colors: 64); another choice sends St at once;
   the start page request has WRP's settings.
   Text in K, then ↵ or ⌫: the text and the key go as keys (`k=ab%0D`), with no button.
   The address box's clear button empties it.
   WRP's error (an answer with no form, after a click on the picture): WRP's page stays, the error
   is on the status line, and St goes at once. With no page (no server), Return tries again.
6. With WRP running (default `127.0.0.1:8089`): the start page, then a page with a window-wide picture.
7. `archaic-headless`: PNG and PBM screenshots (the same pixels), typing into a form and sending it, a new size.
   A color picture on the screen is exactly the GIF's, and the PNG is in colors.
   A new width: one St with the new width, once the size stays (none for a new height); typed text stays.
8. The tall picture, with no WRP: the test server cuts a random 600 x 1500 GIF into slices.
   After a wheel step, a window and a scroll to the end, the frame must show exactly the right rows.
   Also: the next slice comes before it is needed, and a click sends the place on the whole page.
9. Archaic's scroll bar, on the same picture: the arrows, a click in the shaft, the elevator
   dragged (back to where it was, and past the end), an arrow held down for 1 s.
   In 10 shots, the frame must show the right rows, and the elevator must be at the right place.
   Clicks on the bar must not go to the page.

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
- `screenshots/idris.png`: the ST's own screen, 640 x 400, black and white
  (Archaic under the xterm, after three clicks on the ↓ button).

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

All 11 files (124 KB) take about 2 minutes.

### 2. Build

```
sh mkinc     # once per folder: the X10 headers
sh mk        # compile and link: archaic, about 118 KB
```

With Hatari's fast-forward on, `sh mk` takes about 2.5 minutes.
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
With `hn2.abm` in the place of `hn2.gif`, it checks the ABM decoder: the same sum.

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
- `<X/Xlib.h>` is not on the include path, and `Xlib.h` wants a
  `<sys/param.h>` that Idris does not have. `sh mkinc` fixes both,
  in the build folder.
- The default link line allows only a small BSS. `lka` adds `-m0xffffffff` to `+bss`.
- The default link also gives a program only about 26 KB for `malloc`
  (one slice of a long page). `lka` adds `-b0x30000`: 192 KB for the heap
  and the stack. The 4-byte field at offset 16 of the program file holds
  this size.
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
- `screenshots/unixpc.png`: the 3B1's screen in FreeBee (green on black), 720 x 348.
  Wikipedia's AT&T UNIX PC article, after one click on the ↓ button.

## Building on the AT&T UNIX PC (3B1)

Needs **gcc** (2.5.8 in `/usr/local/bin`: the FreeBee kit has it) and WIN/TCP.

1. Copy these 9 files (126 KB) into a folder on the 3B1, with ftp, rcp or a floppy:
   `src/archaic.h`, `main.c`, `html.c`, `http.c`, `gif.c`, `pic.c`, `unixpc.c`, `ntwg.c` and `mkunixpc`.
   The FreeBee kit forwards the 3B1's ftp to port 35121 on your computer, and rsh to 35514.
2. Build:

   ```
   sh mkunixpc     # compile and link: archaic, about 62 KB
   ```

   It takes about 1 minute in FreeBee at turbo 10.
3. Test. **Test mode, no window needed** (serve this folder from your computer:
   `python3 -m http.server 8077`):

   ```
   ./archaic -t http://10.0.2.2:8077/test/data/hn2.gif
   ```

   The last line ends with `sum ae2b58f3`: the same as on Unix and on Idris.
   With `hn2.abm` in the place of `hn2.gif`, it checks the ABM decoder: the same sum.

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
- `keypad(wn, 1)`: the 3B1's keys come as one code each (`kcodes.h`): Exit 0350, Cancl 0370, Page 0371.
  The window is a tty: with VMIN 0 and VTIME 1, a read waits at most 0.1 s.
- Mouse: `wsetmouse()` with MSDOWN. A click comes as the Mouse key (0375),
  then `wreadmouse()` gives x, y in the window and the buttons: 4 left, 2 middle, 1 right.
- The frame's scroll arrows send Roll Up and Roll Down (Shift+↑, Shift+↓). FreeBee's wheel does too.
- **SIGWIND** (the window has a new size) goes only to the window's process group.
  Archaic calls `setpgrp()` before it opens the window, then `ioctl(wn, WIOCPGRP, 0)`.
- `wprompt()` text shows on the screen's prompt line: 72 characters fit.

### WIN/TCP

- Headers in `/usr/ethernet/include`, the library `/usr/lib/libnet.a`.
  Compile with `-DSYSTEM5 -DSHORT_IDENT` (TWG's READ_ME).
- `select()` takes bit masks (a `long`), not `fd_set`, and waits as long as asked: 0.1 s works.
  Archaic waits on the network in 0.1 s steps and reads the window's keys between them.
- No DNS: `gethostbyname()` reads only `/etc/hosts`. Archaic tries `inet_addr()` first.

## Running on SunOS 4 with SunView

- **SunOS 4.1** with SunView, on a Sun-4 (SPARC). Kit: `sunos414-lemu` (QEMU),
  on osarchive in `os/sunos/Virtual/qemu/SunOS414/`. It has Sun's C compiler.
- Release binary: `archaic-sun4`. Copy it to SunOS as `/usr/local/bin/archaic`,
  then `chmod 755 /usr/local/bin/archaic`. Or build it there: see Building on SunOS 4. It takes about 6 s.
- Log in as `root` at the console and type `sunview`.
  In the shelltool, type `archaic` (in the folder where you built it: `./archaic`).
  It opens a frame a little smaller than the screen, in the middle.
- **Ctrl-Q** quits, or Quit in the frame's menu.
- In the kit, your computer is `192.168.1.10` for SunOS (SunOS is `192.168.1.123`).
  So Archaic on SunOS looks for WRP at `http://192.168.1.10:8080/`.
  SunOS has no name server there: give addresses as numbers.
- On a color screen (8 bits, as the cg3) the pictures have up to 128 colors (see Colors).
- `screenshots/sunview.png`: the SPARCstation's screen (cg3), 1152 x 900:
  Wikipedia's SunView article in 128 colors, in 1 s.

## Building on SunOS 4 (SunView)

Needs Sun's C compiler, `cc` (the kit has it), and SunView's libraries in `/usr/lib`:
`libsuntool`, `libsunwindow`, `libpixrect` (SunOS has them with SunView).

1. Copy these 9 files (131 KB) into a folder on SunOS, with ftp, rcp or a CD
   (the kit: `CD=file.iso ./run.sh`, then `mount -rt hsfs /dev/sr0 /cdrom`):
   `src/archaic.h`, `main.c`, `html.c`, `http.c`, `gif.c`, `pic.c`, `nsock.c`, `sunview.c` and `mksunview`.
   The kit lets root in with rsh, on port 41414 of your computer.
2. Build:

   ```
   sh mksunview    # compile and link: archaic, about 96 KB
   ```

   It takes about 6 s in the kit.
3. Test. **Test mode, no window needed** (serve this folder from your computer:
   `python3 -m http.server 8077`):

   ```
   ./archaic -t http://192.168.1.10:8077/test/data/hn2.gif
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
| Network | Lance Ethernet, BSD sockets. QEMU user networking: SunOS 192.168.1.123, **your computer 192.168.1.10** (its 127.0.0.1) |
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

- **SunOS 3.5** with SunView, on a Sun-3 (68020). Tested in TME (The Machine Emulator):
  a Sun-3/160 with the bwtwo screen, and slirp for the network.
- Release binary: `archaic-sun3`. Copy it to SunOS as `/usr/local/bin/archaic`,
  then `chmod 755 /usr/local/bin/archaic`. Or build it there: see Building on SunOS 3.5.
- Log in as `root` at the console and type `suntools` (SunOS 3's name for SunView).
  In a shelltool, type `archaic`.
- **Ctrl-Q** quits, or Quit in the frame's menu.
- With TME's slirp, your computer is `10.0.2.2` for SunOS (SunOS is `10.0.2.15`).
  So Archaic on a Sun-3 looks for WRP at `http://10.0.2.2:8080/`.
- `screenshots/sun3.png`: the Sun-3's screen (bwtwo), 1152 x 900, black and white:
  Wikipedia's Sun-3 article, in 2 s.

## Building on SunOS 3.5

The same 9 files and the same script as on SunOS 4 (see Building on SunOS 4), with Sun's `cc`:

```
sh mksunview    # compile and link: archaic, about 570 KB
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
  `archaic.h` uses `mc68000` for the default address.
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
- The shell's tools are old: no `uname` and no `printf`, and `grep` has no `\|` (use `egrep`).

## Porting to DOS, OS/2, Win16 and others

The core (`main.c`, `html.c`, `http.c`, `gif.c`, `pic.c`) does not change.
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
| `g_event(&e, wait)` | next key, click (`E_DOWN`, `E_UP`, and `E_MOVE` with a button down), expose, resize, quit, scroll (`E_SCROLL`: pixels, quarter windows, windows, or to a place). wait 0: don't wait; 1: wait for an event; 2: wait at most 1/10 s (or as 1) |
| `g_fd()` | the display's file descriptor, or -1 |
| `g_ms()` | a clock in milliseconds, from any start (for the spinner); -1 if the system has none |
| `g_bar` | a variable: 0 if the system has scroll bars (Archaic then calls `g_sbar`), 1 if Archaic must draw its own |
| `g_sbar(pos, max, vis)` | show the page's place in the system's scroll bar: pos, 0 up to max; vis pixels show. Empty where `g_bar` is 1. The scroll bar sends `E_SCROLL` (`S_TO` for its knob) |

**Network (`n_` functions)**

| Function | Does |
|---|---|
| `n_open(host, port, err)` | connect; on error, the reason in `err` |
| `n_read(h, buf, n, wait)` | bytes read; -2 nothing yet, 0 end, -1 error. wait 1: a short time (`nsock.c` 1/20 s), less when the display has input |
| `n_write(h, buf, n)`, `n_close(h)` | |

**C rules** (they are in `archaic.h` too):

- K&R function definitions. Prototypes only through `P()`. `-DNOPROTO` turns them off.
- No `const`, `enum`, `void *`, struct assignment, structs by value.
- Parameters only `int`, `long` or pointers.
- No arrays in local variables (Whitesmiths bug). Make them static.
- `unsigned char` for bytes. `long` for byte counts and pixel offsets (int may be 16 bits).
- Few library calls: malloc, free, sprintf, printf, fprintf, getenv, time.

**Memory note for 16-bit systems:** the static data is about 145 KB
(page 30 KB, items 50 KB, read buffer 16 KB, GIF tables 32 KB).
On DOS or Win16, use the large memory model, or make these smaller in `archaic.h`.

## Roadmap: poor systems with no web browser

Old systems that never had a web browser: the next ports.
Each port is a display file, plus a network file if the system has no BSD sockets (see Porting).

| Target | What it is | Systems |
|---|---|---|
| MGR | a window system on a terminal line | SunOS, Coherent, Xenix, Minix, OS-9, VSTa |
| Blit, DMD 5620 | a graphics terminal on a serial line | Research Unix, System V (AT&T 3B2) |
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

- Plan: Archaic in two parts. On the host: the network, HTML and GIF, as now.
  In the terminal: a small program that shows the pictures and reads the mouse.
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
- A page over 30 KB of HTML is cut (WRP's pages are about 3 KB).
- On the 3B1, the font has only ASCII: other letters in WRP's form show as `?`.
- In SunView, a second window with many colors (another Archaic) can change the colors of the first one,
  until the pointer is in it (see SunOS 4 and SunView: what to know).
- WRP before commit 3126bc0 (November 2026) could hang for good on back (← arrow),
  when Chrome took the page from its back-forward cache. Use a newer WRP.
- With slices, a link to a place on the same page (`#name`) does not scroll the frame there.
- Only WRP's black and white pictures come as ABM. Pictures with colors are GIFs, and the GIF decoder
  does them a pixel at a time: slow on an old CPU (see Pictures: GIF and ABM).
