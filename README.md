# Archaic

A very small web browser for very old computers.

Archaic is a client of **[WRP, the Web Rendering Proxy](https://github.com/tenox7/wrp)**.
WRP runs a modern browser on a modern computer. It sends each web page
as **one GIF picture** with a click map (ISMAP), plus a small HTML form.
Archaic shows the form and the picture, and sends your clicks and keys back.
WRP does all the hard parts: HTTPS, JavaScript, CSS, fonts.

![Archaic in Hatari: Idris on an Atari Mega ST, showing Wikipedia](screenshots/hatari.png)

First port: **Whitesmiths Idris** (a 1988 Unix clone) on the **Atari ST**,
with **X10** (X Version 10 Release 4) and TCP/IP over **SLIP**.
Also: any Unix with X11, for development and tests.
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
  the `archaic` binary for Idris. Copy it into Idris (kermit works),
  then `chmod 755 archaic`.

Then:

1. On your computer, start WRP: `wrp`. It listens on port 8080.
2. In Idris, log in as `root` and start X: `/usr/X/bin/xstart`.
3. In an xterm, type `archaic`. It opens on the whole screen.
4. Click the address box, type an address, press Return.
5. **Ctrl-Q** quits.

In the emulator kit, your computer is `10.0.2.2` for Idris.
So Archaic on Idris looks for WRP at `http://10.0.2.2:8080/`.
Somewhere else: `archaic HOST:PORT`, or `setenv WRP HOST:PORT`.

The ST's own screen, 640 x 400, black and white
(Archaic under the xterm, after two clicks on **Dn**):

![Archaic on the Atari ST screen: Wikipedia's Idris article](screenshots/idris.png)

---

## Using it

```
archaic [=WxH+X+Y] [-a] [-t] [-v] [-o file.pbm] [-s script] [url]
```

| Option | Meaning |
|---|---|
| `url` | WRP's address, like `10.0.2.2:8080`. Default: `$WRP`, else port 8080 on the host (Idris: `10.0.2.2`, Unix: `127.0.0.1`) |
| `=WxH+X+Y` | window size and place (also `-g WxH+X+Y`). Default: the whole screen |
| `-a` | also show WRP's M, T and C menus (for future color ports; Idris does not need it) |
| `-t` | test mode: no window. Prints the page items and a checksum of each picture |
| `-v` | status messages also go to stderr |
| `-o file.pbm` | at the end, save the first picture as a PBM file |
| `-s script` | play a script, then quit (see Testing) |

**WRP's settings are set by Archaic, and not shown:**

| WRP box | Archaic sends |
|---|---|
| W, H | the window's size: the picture fills the window. Always, also with `-a` |
| M (mode) | ISMAP |
| T (picture type) | GIF |
| C (colors) | 2 |

So WRP's form is one line: the address, Go, Bk, St, Re, Up, Dn, Z (zoom), K (keys), Bs, Rt.

**Mouse**

- Left button: click links, buttons, fields, menus, and the picture.
- Right button: scroll. Top half of the window = up, bottom half = down.

**Keys**

| Key | Does |
|---|---|
| Tab | next text field |
| Return | send the form |
| Backspace / Delete | delete the last character |
| Ctrl-U | clear the field |
| Esc | stop loading (or leave the field) |
| Space / b | scroll down / up (when no field has the keyboard) |
| Ctrl-Q | quit |

---

## Files

| File | What it is |
|---|---|
| `src/archaic.h` | everything shared, and the C rules for old compilers |
| `src/main.c` | the page on the screen: layout, drawing, mouse, keys, forms |
| `src/html.c` | HTML reader: forms, menus, links, images, text |
| `src/http.c` | URLs and HTTP/1.0, with a 16 KB read buffer |
| `src/gif.c` | GIF decoder: to black and white, 4x4 dither |
| `src/x10.c` | display for X10 (Idris) |
| `src/nidris.c` | network for Idris: TCP through netd |
| `src/x11.c` | display for X11 (Unix, macOS with XQuartz) |
| `src/nsock.c` | network for BSD sockets (Unix, macOS) |
| `src/fetch`, `mkinc`, `mk`, `lka` | Idris scripts: get the sources, headers, compile, link |
| `tools/idris.py` | host tool: run commands in Idris over telnet, serve files |
| `tools/hget.c` | Idris tool: download one file over HTTP (through netd) |
| `test/run.sh` | tests: GIF decoder, HTML, forms, WRP |
| `test/xshot.sh` | test: run the X11 build and take a screenshot |
| `test/data/` | GIFs saved from WRP, for the tests |
| `test/cfacts.c`, `cfacts2.c` | probe for a new port: what its C compiler and linker do (name length, case, struct copies) |
| `screenshots/` | the pictures in this README |

---

## Building and testing on Unix or macOS

Needs X11 (on macOS: XQuartz in `/opt/X11`). The tests need ImageMagick.

```
make              # ./archaic, X11 + BSD sockets
make archaic-asan # the same with address and undefined-behaviour checks
make knr          # syntax check as a pre-ANSI compiler sees it (-DNOPROTO)
make test         # sh test/run.sh
```

`test/run.sh` checks:

1. GIF decoding gives **exactly** the same pixels as ImageMagick.
2. Gray and color GIFs decode at the right size.
3. HTML: title, entities, fields, menus, hidden fields, buttons, links, comments.
4. A button click sends the form with the right query.
5. WRP's form: W, H, M, T and C are hidden, and Archaic sends its own values (and with `-a`, the menus).
6. With WRP running (default `127.0.0.1:8089`): the start page, then a page with a window-wide picture.

Run WRP for the tests: `wrp -l 127.0.0.1:8089`.

To see the real window without a desktop, use Xvfb:

```
Xvfb :7 -screen 0 660x420x24 -listen tcp -nolisten local -nolisten unix &
PATH=/opt/X11/bin:$PATH sh test/xshot.sh 127.0.0.1:7 shot.png 1 '%%' 127.0.0.1:8089
```

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
- `rm` has no `-f`. There is no `tail` and no `which`.

### X10 on this ST

- The screen size reads as 0 x 0. Archaic then uses 640 x 400.
- The keyboard goes to the window under the mouse pointer.
  Archaic moves the pointer into its window when it opens.
- Bitmaps: rows of 16-bit shorts, the **low** bit is the **left** pixel.
- The **left** mouse button comes as `MiddleButton` (1), the right one as `RightButton` (0).
- All four arrow keys come as the same key. Archaic does not use them.
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
| `g_event(&e, wait)` | next key, click, expose, resize, quit |
| `g_fd()` | the display's file descriptor, or -1 |

**Network (`n_` functions)**

| Function | Does |
|---|---|
| `n_open(host, port, err)` | connect; on error, the reason in `err` |
| `n_read(h, buf, n, wait)` | bytes read; -2 nothing yet, 0 end, -1 error. wait 1: up to 1 s |
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

## Known limits

- One page at a time. No history of its own: use WRP's **Bk** button.
- A page over 30 KB of HTML is cut (WRP's pages are about 3 KB).
- WRP before commit 3126bc0 (November 2026) could hang for good on **Bk**,
  when Chrome took the page from its back-forward cache. Use a newer WRP.

---

## License

Apache License 2.0, like WRP. See `LICENSE`.
