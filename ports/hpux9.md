# Archaic on HP-UX 9

The parts that all systems share (Colors, Pictures, Updates, Porting ...) are in [AGENTS.md](../AGENTS.md).

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
