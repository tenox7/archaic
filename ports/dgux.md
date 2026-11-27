# Archaic on DG/UX

The parts that all systems share (Colors, Pictures, Updates, Porting ...) are in [AGENTS.md](../AGENTS.md).

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
