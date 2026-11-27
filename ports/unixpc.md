# Archaic on the AT&T UNIX PC (3B1)

The parts that all systems share (Colors, Pictures, Updates, Porting ...) are in [AGENTS.md](../AGENTS.md).

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
