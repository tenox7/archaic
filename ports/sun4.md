# Archaic on SunOS 4 (SunView, Sun-4)

The parts that all systems share (Colors, Pictures, Updates, Porting ...) are in [AGENTS.md](../AGENTS.md).

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
