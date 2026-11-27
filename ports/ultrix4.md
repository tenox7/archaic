# Archaic on Ultrix 4

The parts that all systems share (Colors, Pictures, Updates, Porting ...) are in [AGENTS.md](../AGENTS.md).

## Running on Ultrix 4

- **Ultrix 4.5** with DECwindows (UWS V4.5: X11 and Motif), on a DECstation 5000/200 (MIPS R3000).
  Kit: `decstation5000-ultrix45-gxemul` (gxemul), on osarchive in `os/ultrix/virt/`.
  Archaic is in the kit: `/usr/local/bin/archaic`, and its source in `/usr/local/src/archaic`.
- Release binary: `archaic-ultrix4` (the kit's). On another Ultrix 4 with X11, copy it as `/usr/local/bin/archaic`,
  then `chmod 755 /usr/local/bin/archaic`. Or build it there: see Building on Ultrix 4.
- Log in as `root` at the DECwindows login (the kit's password is `ultrix`).
  Open a DECterm (Session Manager: Applications > DECterm) and type `archaic HOST:8080`.
- **Ctrl-Q** quits, or Close in the window's menu.
- HOST is your computer's address on its network (its LAN address, as numbers). Or set `WRP` to `HOST:8080`.
  gxemul's network (NAT) has no address for your computer's own 127.0.0.1 (QEMU's NAT has one: 10.0.2.2).
  So on Ultrix Archaic has no WRP address of its own: with none, it tries 127.0.0.1 in Ultrix itself,
  and says `connection refused`. WRP listens on all addresses by default (`-l :8080`), so Ultrix reaches it.
- The screen has 8 bits, so the pictures have the 216 web colors (see 216 colors: GIP).
- `screenshots/ultrix.png`: the screen, 1280 x 1024, with mwm: `uname -a` in a DECterm,
  and Wikipedia's Ultrix article in 216 colors.

## Building on Ultrix 4

Needs Ultrix's C compiler, `cc`, and X11 (the UWS subsets: DECwindows). The kit has both.

1. Copy these 10 files (200 KB) into a folder on Ultrix, with ftp, rcp or NFS:
   `src/archaic.h`, `main.c`, `html.c`, `http.c`, `gif.c`, `pic.c`, `update.c`, `nsock.c`, `x11.c` and `mkultrix`.
   The kit lets root in with rsh, on port 45514 of your computer.
2. Build:

   ```
   sh mkultrix     # compile and link: archaic, about 800 KB (516 KB after strip)
   ```

   It takes about 6 s in the kit. gcc builds it too (3.0.4 in `/freeware/gnu-tools/mips/bin` in the kit):
   `CC=gcc sh mkultrix` takes 16 s.
3. Test. **Test mode, no window needed** (serve this folder from your computer, on its LAN address:
   `python3 tools/idris.py 0 serve . 8077 HOST`):

   ```
   ./archaic -t http://HOST:8077/test/data/hn2.gif
   ```

   The last line ends with `sum ae2b58f3`: the same as on the other systems.
   With `hn2.abm` in the place of `hn2.gif`, it checks the ABM decoder: the same sum.
   All 9 pictures in `test/data` (7 GIF, 2 ABM) give the same sums as on the Mac, also with `-c 256`.

   **With the window:** `./archaic HOST:8080` in a DECterm.

**Measured in the kit** (gxemul on an Apple Silicon Mac), from Return until the picture is complete.
Archaic decodes a picture in less than 0.1 s here: most of a page's time is WRP's.

| What | Time |
|---|---|
| decode check (`-t`, `hn2.gif`, 11 KB, 256 colors), with the download | less than 0.1 s |
| Wikipedia's Ultrix article in the window, 216 colors (45 KB) | 7 s |
| Wikipedia's Ken Olsen article, after a click on its link (46 KB) | 1 s |

## Ultrix 4: what to know

Test on a copy of your Ultrix disk.

| Thing | Ultrix 4.5 kit |
|---|---|
| Computer | a DECstation 5000/200 (3MAX) in gxemul: MIPS R3000, 128 MB |
| Screen | HX (PMAGB-BA): 1280 x 1024, 8 bits: 256 colors in one colormap (PseudoColor) |
| Window system | DECwindows (UWS V4.5): X11 from DEC, with mwm (Motif) and the Session Manager |
| Network | PMAD-AA Ethernet (`ln0`), BSD sockets. gxemul's NAT: Ultrix 10.0.0.1, the gateway and name server 10.0.0.254. **No address for your computer's 127.0.0.1**: use its LAN address |
| Names | gxemul's name server, 10.0.0.254 (after `/etc/hosts`) |

- `cc` is Ultrix's own C compiler. It defines `ultrix`, `__ultrix`, `mips`, `__mips`, `MIPSEL`, `unix` and `bsd4_2`.
  In its default mode it takes prototypes, but it does not define `__STDC__`. `mkultrix` needs no `-DNOPROTO`.
- gcc 3.0.4 (in `/freeware/gnu-tools/mips/bin` in the kit) defines the same names, and `__STDC__`.
- Ultrix has no shared libraries: Xlib (`/usr/lib/libX11.a`) goes into the program.
- `fd_set` and its macros are in `<sys/types.h>`: `nsock.c` and `x11.c` need nothing for Ultrix.
- DECwindows has no font named `6x13`. `x11.c` then takes `fixed`, which is 6 x 13 too.
- From an rsh login as root, `DISPLAY=unix:0` reaches the screen (with root logged in at the DECwindows login).
- The LK201 keyboard has no Page Up or Page Down: its Prev Screen and Next Screen come as Prior and Next,
  and scroll a window. gxemul types them for the host's Page Up and Page Down.
- root's shell is csh: `sh -c '...'` for sh syntax, `>&` for `2>&1`. Quote patterns such as `'x[y]'`: csh expands them first.
- `/` is 16 MB and almost full, and `/tmp` is on it: two builds there filled it. Build in `/usr/local/src` or `/var/tmp`.
- `/etc/mount -t nfs HOST:/dir /mnt` mounts an NFS folder (NFS version 2, through gxemul's NAT).
- `grep` has no `-o`. Use `egrep` for `|`.
- Stop Ultrix with `/etc/shutdown -h now` (from rsh: `/etc/shutdown -h now >& /dev/null &`). Wait for `>>`, then quit gxemul.
- Tests with no window: start gxemul with `--control unix:ctl.sock` (and `SDL_VIDEODRIVER=dummy`).
  `gxemul --ctl unix:ctl.sock keyboard`, `mouse` and `screendump` type, click and show the screen
  (`gxemul --mcp unix:ctl.sock` gives them as MCP tools). A `mouse` click sent with its move was lost once:
  move first, then send `down` and `up`. For Return, send `keys: ["Return"]`: `keyboard` drops control
  characters in `text` (gxemul 1.0.13), so `enter: true` with `text` types nothing.
