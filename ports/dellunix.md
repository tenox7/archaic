# Archaic on Dell UNIX

The parts that all systems share (Colors, Pictures, Updates, Porting ...) are in [AGENTS.md](../AGENTS.md).

## Running on Dell UNIX

- **Dell UNIX System V Release 4.0 Issue 2.2** (Dell's SVR4 for Intel PCs) with X11, in QEMU (LEMU, `-M pc`).
  Kit: `dellunix22-lemu`, on osarchive in `os/dellunix`. The kit has Archaic in `/usr/local/bin` since 2026-11-27.
- Release binary: `archaic-dellunix`. Copy it as `/usr/local/bin/archaic`, then `chmod 755 /usr/local/bin/archaic`.
  Or build it there: see Building on Dell UNIX.
- Log in at xdm as `root` (mwm). In an xterm, type `archaic &`.
- **Ctrl-Q** quits.
- Archaic looks for WRP at `http://192.168.1.10:8080/`: your computer, in the kit's network (QEMU NAT).
  Elsewhere (QEMU's default NAT: `10.0.2.2:8080`, a bridged VM): `archaic HOST:8080`, or set `WRP` to `HOST:8080`.
- The kit's screen is 1280 x 1024 in 256 colors: the pictures have the 216 web colors (AB8).
- A link on example.com: 2 s until iana.org's page is complete (WRP 4.14.5 in Docker on the same Mac).

## Building on Dell UNIX

Needs Dell's C compiler (`/usr/ccs/bin/cc`) and Dell's X11R5 in `/usr/X5`. The kit has both.

1. Copy `Makefile` and the folder `src` to Dell UNIX, with ftp, rcp or NFS.
2. Build:

   ```
   make            # or: cd src; sh mkdellunix
   ```

   `archaic` is about 116 KB.
3. Test. **Test mode, no window needed** (serve this folder from your computer: `python3 -m http.server 8077`):

   ```
   ./archaic -t http://HOST:8077/test/data/hn2.gif
   ```

   The last line ends with `sum ae2b58f3`: the same as on the other systems.

   **With the window:** `./archaic` in an xterm.

## Dell UNIX: what to know

Test on a copy of your Dell UNIX disk, or start QEMU with `-snapshot`.

| Thing | Dell UNIX kit |
|---|---|
| Computer | a PC in QEMU (LEMU, `-M pc`): Pentium, 64 MB |
| Screen | S3 Trio64, 1280 x 1024 in 256 colors (Dell's X server, `Xmach.s3`) |
| Window system | X11R5 in `/usr/X5`, mwm, xdm |
| Network | WD8003E, BSD sockets (`libsocket`, `libnsl`). QEMU NAT: Dell UNIX 192.168.1.111, **your computer 192.168.1.10** |

- `SYSID` is `dellunix` (`DELLUNIX`, set by `mkdellunix`).
- `cc -Xa` is ANSI C. Its default, `-Xt`, keeps K&R's unsigned promotions. It defines only `i386` and `unix`.
- `uname -s` prints the host's name, not the system's: `make` looks for release `4.0` on `i386`.
- `/usr/lib/libX11.so` is X11R4: `mkdellunix` links with `-L/usr/X5/lib`.
- mwm is click to focus. xdm's login box takes keys only with the pointer over it.
