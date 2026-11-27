# Archaic on AIX 3.2

The parts that all systems share (Colors, Pictures, Updates, Porting ...) are in [AGENTS.md](../AGENTS.md).

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
