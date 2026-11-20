# Archaic

A very small web browser for very old computers.

Archaic is a client of **[WRP, the Web Rendering Proxy](https://github.com/tenox7/wrp)**.
WRP runs a modern browser on a modern computer. It sends each web page
as **one GIF picture** with a click map (ISMAP), plus a small HTML form.
Archaic shows the form and the picture, and sends your clicks and keys back.
WRP does all the hard parts: HTTPS, JavaScript, CSS, fonts.

![Archaic in Hatari: Idris on an Atari Mega ST, showing Wikipedia](screenshots/hatari.png)

![Archaic in FreeBee: the AT&T UNIX PC (3B1), showing Wikipedia](screenshots/freebee.png)

First port: **Whitesmiths Idris** (a 1988 Unix clone) on the **Atari ST**,
with **X10** (X Version 10 Release 4) and TCP/IP over **SLIP**.
Second port: the **AT&T UNIX PC** (3B1, PC 7300, 1985), with its own
window system and **WIN/TCP** (The Wollongong Group's TCP/IP) on its Ethernet board.
Also, for development and tests: **macOS** (Cocoa), anything with **SDL 2**,
any Unix with **X11**, and **no window at all** (commands in, screenshots out).
The code is plain old C, written to port to DOS, OS/2, Win16 and others.

| What Archaic shows | What it skips |
|---|---|
| text fields, password fields | JavaScript, CSS, HTTPS (WRP does all that) |
| buttons, drop-down menus (SELECT) | check boxes, radio buttons, TEXTAREA |
| links, page title, line breaks | tables, fonts, colors |
| GIF pictures, clickable (ISMAP) | PNG, JPEG (Archaic asks WRP for GIF) |

---

## Quick start: Idris on the Atari ST

You need:

- **WRP** on your computer: [github.com/tenox7/wrp](https://github.com/tenox7/wrp).
- **Idris** on an Atari ST, or in the Hatari emulator: the Idris kit
  `atarist-idris314-hatari` (on osarchive, in `os/idris/virt/`).
  Newer kits have Archaic in `/usr/X/bin`.
- Or **this repo's [release](https://github.com/tenox7/archaic/releases)**:
  the `archaic` binary for Idris. Copy it into Idris's `/usr/X/bin`
  (kermit works), then `chmod /usr/X/bin/archaic`.
  With no mode, Idris's chmod makes a file executable.

Then:

1. On your computer, start WRP: `wrp`. It listens on port 8080.
2. In Idris, log in as `root` and start X: `/usr/X/bin/xstart`.
3. In an xterm, type `archaic`. It opens on the whole screen.
   (xstart puts `/usr/X/bin` on the search path. From a telnet login,
   type `/usr/X/bin/archaic`.)
4. Click the address box, type an address, press Return.
5. **Ctrl-Q** quits.

In the emulator kit, your computer is `10.0.2.2` for Idris.
So Archaic on Idris looks for WRP at `http://10.0.2.2:8080/`.
Somewhere else: `archaic HOST:PORT`, or `setenv WRP HOST:PORT`.

The ST's own screen, 640 x 400, black and white
(Archaic under the xterm, after three clicks on the ↓ button):

![Archaic on the Atari ST screen: Wikipedia's Idris article](screenshots/idris.png)

---

## Quick start: the AT&T UNIX PC (3B1)

You need:

- **WRP** on your computer, as above.
- A **3B1** with the Ethernet board and **WIN/TCP** (TWG's TCP/IP, in `/usr/ethernet`),
  or the **FreeBee** emulator: the kit `att3b1-unix351-freebee` (on osarchive, in `os/att/3b1/virt/`).
- The `archaic-unixpc` binary from this repo's **[release](https://github.com/tenox7/archaic/releases)**,
  or build it (see below). Copy it to the 3B1 as `/usr/local/bin/archaic`,
  then `chmod 755 /usr/local/bin/archaic`.

Then:

1. On your computer, start WRP: `wrp`. It listens on port 8080.
2. On the 3B1, open a shell window (Office > UNIX System) and type `archaic`.
   It opens a window of its own, as large as the screen allows.
   (A telnet or rsh login works too: the window opens on the 3B1's screen.)
3. Click the address box, type an address with `https://`, press Return.
4. **Exit** (or the close box in the window's frame) quits.

In the FreeBee kit, your computer is `10.0.2.2` for the 3B1, as for Idris.
So Archaic on the 3B1 looks for WRP at `http://10.0.2.2:8080/`.
The 3B1 has no DNS: give WRP's address as numbers, or put the name in `/etc/hosts`.

The 3B1's screen in FreeBee (green on black), 720 x 348.
Wikipedia's AT&T UNIX PC article, after one click on the ↓ button:

![Archaic on the AT&T UNIX PC: Wikipedia's AT&T UNIX PC article](screenshots/unixpc.png)

---

## Using it

```
archaic [=WxH+X+Y] [-a] [-c n] [-t] [-v] [-o file] [-s script] [url]
```

| Option | Meaning |
|---|---|
| `url` | WRP's address, like `10.0.2.2:8080`. Default: `$WRP`, else port 8080 on the host (Idris and the 3B1: `10.0.2.2`, Unix: `127.0.0.1`) |
| `=WxH+X+Y` | window size and place (also `-g WxH+X+Y`). Default: the whole screen. On the 3B1: 686 x 248 inside a frame; a larger size gets no frame, up to 720 x 288 (`-g 720x288`) |
| `-a` | also show WRP's M, T and C menus, and send them as they are set |
| `-c n` | start with pictures of n colors (2 to 256), not as many as the display shows (see Colors). With `-t`: as if the display showed n colors |
| `-t` | test mode: no window. Prints the page items and a checksum of each picture |
| `-v` | status messages also go to stderr |
| `-o file` | at the end, save the first picture: PBM, or PPM if it has colors |
| `-s script` | play a script, then quit (see Testing) |

**WRP's settings are set by Archaic, and not shown:**

| WRP box | Archaic sends |
|---|---|
| W, H | the window's size: the picture fills the window. Always, also with `-a` |
| M (mode) | ISMAP |
| T (picture type) | GIF |
| C (colors) | black and white display: 2 (or `-c n`), not shown. A display with colors: the menu shows (see Colors) |
| SL (slices) | 1: the whole page, in slices (see Scrolling). An older WRP ignores it |

So WRP's form is one line: the address box, **Go**, WRP's buttons, **Z** (zoom),
**C** (colors, only on a display with colors) and **K** (keys).

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

### Colors

Archaic asks the display how many colors it shows.
The first status line says what it found, like `Archaic 0.3, millions of colors`.

| Display | Colors |
|---|---|
| 3B1, Idris (X10) | black and white (they are) |
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
- X11 with a colormap: Archaic gets the picture's colors in the default colormap,
  shared with the other programs. When the colormap is full, it takes the nearest color
  the colormap has. A new picture's colors free the last ones.

On a black and white display, the C menu is hidden. Archaic asks WRP for 2 colors
(or for `-c n`). The pictures are 1 bit a pixel; one with more colors gets a 4x4 dither.

Archaic's own start page request asks WRP for the same settings as the next ones
(`/?m=ismap&t=gif&c=256&sl=1`), so that WRP's form does not change after the first page.

To type into a box on the web page: click the box in the picture, then type in **K**.

- Click ↵: WRP types the text into the page, then presses Return (to search, for example).
- Or press Return: WRP types only the text. Use this to fill in more boxes before you send the page's form.

(WRP alone does only the button's key when it gets both a button and keys, so Archaic sends
the text and the button's key together as keys: `k=text%0D`. This works with every WRP.)

**Mouse**

- Left button: click links, buttons, fields, menus, and the picture.
- Right button: scroll a window. Top half of the window = up, bottom half = down.
- The scroll bar at the right: see Scrolling below.

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
| the resize box | a new window size. Then click ↻ for a picture that fits |

**On macOS (Cocoa) and with SDL:** Page Up and Page Down scroll a window.
The up and down arrows and each step of the mouse wheel scroll a quarter of it.
A trackpad scrolls smoothly, by pixels. Ctrl+click is the right button.
Cmd-Q and the window's close button quit.
Cocoa has the Mac's own scroll bar; SDL has Archaic's (see Scrolling).

On the 3B1, the help line at the bottom of the screen lists the main keys.

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

**Scrolling: the whole page in slices**

With a WRP that knows slices (`sl=1`), WRP makes a picture of the whole page once, and keeps it.

- Archaic shows the picture in a frame under WRP's form. Only the frame scrolls: the form stays at the top.
- The picture comes in **slices** as high as the frame. Archaic keeps 4 of them:
  the ones on the screen, and the next one above and below.
  It gets the next ones while you read, so most scrolls do not wait for the network.
- A click on the picture goes to WRP as a place on the whole page.
- After a click or keys on the same page, the frame stays where it was. A new page starts at its top.
- WRP leaves out its Up and Dn buttons.
- A slice in 2 colors is about 5 to 9 KB for 632 x 351 pixels.
  In 256 colors it is bigger: about 20 to 100 KB.

| Input | Scrolls the frame |
|---|---|
| a step of the wheel, ↑ ↓, the 3B1's scroll arrows and Roll keys | a quarter of the frame |
| Page Up, Page Down, Space, b, the right button, the 3B1's Page, Next, Prev | the frame, less 2 text lines |
| a trackpad (Cocoa, SDL) | by pixels |

An older WRP ignores `sl=1`: then the picture fills the window, and WRP's Up and Dn buttons scroll the page.

**The scroll bar**

A system that has scroll bars uses its own. Where it has none, Archaic draws one.

| Display | Scroll bar |
|---|---|
| Cocoa (macOS) | the Mac's own (NSScroller), always shown. A click in its track scrolls as Page Down or Page Up, or jumps to that place, as set in System Settings (with Option, the other one) |
| the 3B1 | the window frame's scroll arrows |
| SDL, X11, X10 (the Atari ST), headless | Archaic's own, in the style of the Atari ST's GEM |

Archaic's own scroll bar is at the right of the window, 17 pixels wide:

![Archaic's own scroll bar at the right, in GEM style: Wikipedia's Atari ST article](screenshots/scrollbar.png)

| Part | Does |
|---|---|
| ⇧ and ⇩ arrow boxes | scroll a quarter of the frame. Hold the button down: again every 1/10 s, after 4/10 s |
| the dotted shaft, above or below the elevator | scroll as Page Up or Page Down. Held down, it goes on until the elevator gets to the pointer |
| the white elevator | drag it. Its length shows how much of the page is in the window, its place where |

On X10 the elevator moves when the button is let go (as in GEM), and held parts do not repeat:
X10 has no events for the mouse moving with a button down, and can't wait for a time.

---

## Files

| File | What it is |
|---|---|
| `src/archaic.h` | everything shared, and the C rules for old compilers |
| `src/main.c` | the page on the screen: layout, drawing, mouse, keys, forms |
| `src/html.c` | HTML reader: forms, menus, links, images, text |
| `src/http.c` | URLs and HTTP/1.0, with a 16 KB read buffer |
| `src/gif.c` | GIF decoder: the colors, or black and white with a 4x4 dither |
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
| `tools/idris.py` | host tool: run commands in Idris over telnet, serve files |
| `tools/hget.c` | Idris tool: download one file over HTTP (through netd) |
| `tools/pcf2c.py` | host tool: an X11 PCF font as a C table (it made `font6x13.h`) |
| `test/run.sh` | tests: GIF decoder, HTML, forms, WRP, the headless display |
| `test/xshot.sh` | test: run the X11 build and take a screenshot |
| `test/data/` | GIFs saved from WRP, for the tests |
| `test/cfacts.c`, `cfacts2.c` | probe for a new port: what its C compiler and linker do (name length, case, struct copies) |
| `screenshots/` | the pictures in this README |

---

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
2. Gray and color GIFs decode at the right size.
   With colors (`-c 256`), the colors are **exactly** ImageMagick's.
3. HTML: title, entities, fields, menus, hidden fields, buttons, links, comments.
4. A button click sends the form with the right query.
5. WRP's form: W, H, M, T and C are hidden, and Archaic sends its own values (and with `-a`, the menus). WRP's 7 buttons are pictures, Go stays text.
   With colors: C shows, set to the colors (for 100 colors: 64); another choice sends St at once;
   the start page request has WRP's settings.
   Text in K, then ↵ or ⌫: the text and the key go as keys (`k=ab%0D`), with no button.
   The address box's clear button empties it.
6. With WRP running (default `127.0.0.1:8089`): the start page, then a page with a window-wide picture.
7. `archaic-headless`: PNG and PBM screenshots (the same pixels), typing into a form and sending it, a new size.
   A color picture on the screen is exactly the GIF's, and the PNG is in colors.
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

---

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

All 10 files (65 KB) take about 70 seconds.

### 2. Build

```
sh mkinc     # once per folder: the X10 headers
sh mk        # compile and link: archaic, about 80 KB
```

With Hatari's fast-forward on, `sh mk` takes about 1.5 minutes.
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
| WRP start page | 4 s |
| Hacker News page with its 10.7 KB picture | 36 s from Return |
| Wikipedia page with its 9 KB picture | 30 s from Return |
| decode check (`-t`, two 11 KB downloads) | 25 s |

---

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

---

## Building on the AT&T UNIX PC (3B1)

Needs **gcc** (2.5.8 in `/usr/local/bin`: the FreeBee kit has it) and WIN/TCP.

1. Copy these 8 files (74 KB) into a folder on the 3B1, with ftp, rcp or a floppy:
   `src/archaic.h`, `main.c`, `html.c`, `http.c`, `gif.c`, `unixpc.c`, `ntwg.c` and `mkunixpc`.
   The FreeBee kit forwards the 3B1's ftp to port 35121 on your computer, and rsh to 35514.
2. Build:

   ```
   sh mkunixpc     # compile and link: archaic, about 43 KB
   ```

   It takes about 40 s in FreeBee at turbo 10.
3. Test. **Test mode, no window needed** (serve this folder from your computer:
   `python3 -m http.server 8077`):

   ```
   ./archaic -t http://10.0.2.2:8077/test/data/hn2.gif
   ```

   The last line ends with `sum ae2b58f3`: the same as on Unix and on Idris.

   **With the window:** `./archaic` in a shell window.

**Measured in FreeBee** (from Return until the picture is complete):

| What | 10 MHz, as a real 3B1 | turbo 10, as in the kit |
|---|---|---|
| WRP start page (`-t`) | 1 s | 0.1 s |
| decode check (`-t`, an 11 KB GIF) | 10 s | 1 s |
| Wikipedia page with its picture | 12 s | 2 s |
| ↓ on it (a dithered photo) | 18 s | 7 s |

---

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

---

## Porting to DOS, OS/2, Win16 and others

The core (`main.c`, `html.c`, `http.c`, `gif.c`) does not change.
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

**Memory note for 16-bit systems:** the static data is about 130 KB
(page 30 KB, items 50 KB, read buffer 16 KB, GIF tables 18 KB).
On DOS or Win16, use the large memory model, or make these smaller in `archaic.h`.

---

## Roadmap: poor systems with no web browser

Old systems that never had a web browser: the next ports.
Each port is a display file, plus a network file if the system has no BSD sockets (see Porting).

| Target | What it is | Systems |
|---|---|---|
| MGR | a window system on a terminal line | SunOS, Coherent, Xenix, Minix, OS-9, VSTa |
| Blit, DMD 5620 | a graphics terminal on a serial line | Research Unix, System V (AT&T 3B2) |
| QNX Windows | QNX's window system before Photon | QNX 2 |
| SunWindows, SunView | Sun's window system before X11 | SunOS 1 to 4, on Sun-1 to Sun-4 |
| VWS, EWS | DEC's window systems before X11 | VAX/VMS (VWS), VAXELN (EWS) |
| NeWS, Display PostScript | PostScript displays | SunOS, SGI, OS/2, VMS, NeXTSTEP, AIX ... |

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

- Plan: a display file for SunView (pixrect and pixwin).
- Network: BSD sockets, as `nsock.c`.

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

---

## Known limits

- One page at a time. No history of its own: use WRP's back button (← arrow).
- A page over 30 KB of HTML is cut (WRP's pages are about 3 KB).
- On the 3B1, the font has only ASCII: other letters in WRP's form show as `?`.
- WRP before commit 3126bc0 (November 2026) could hang for good on back (← arrow),
  when Chrome took the page from its back-forward cache. Use a newer WRP.
- With slices, a link to a place on the same page (`#name`) does not scroll the frame there.

---

## License

Apache License 2.0, like WRP. See `LICENSE`.
