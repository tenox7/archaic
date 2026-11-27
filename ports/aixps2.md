# Archaic on AIX PS/2 1.3

The parts that all systems share (Colors, Pictures, Updates, Porting ...) are in [AGENTS.md](../AGENTS.md).

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
