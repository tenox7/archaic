# Archaic on A/UX 3

The parts that all systems share (Colors, Pictures, Updates, Porting ...) are in [AGENTS.md](../AGENTS.md).

## Running on A/UX 3

- **A/UX 3.1.1** (Apple's Unix, with the System 7 Finder) on a Quadra 800 (68040), in QEMU (LEMU).
  Kit: `aux311q800-lemu`, on osarchive in `os/a-ux/Virtual/qemu`. It has A/UX's C compiler and the A/UX Toolbox.
  The kit has Archaic in `/usr/local/bin` since 2026-11-26. There is no release file yet: elsewhere, build it
  (see Building on A/UX 3).
- Archaic is **two files**: `archaic`, the program, and `%archaic`, its resources. Keep them in one folder:
  without `%archaic` the A/UX Toolbox does not start the program (it says `FOLDER: No such file or directory`).
  Install: `cp archaic %archaic /usr/local/bin`, then `chmod 755 /usr/local/bin/archaic`.
  An update (see Updates) replaces only `archaic`: `%archaic` stays.
- Log in to the Finder. Apple menu > CommandShell, then type `/usr/local/bin/archaic` (root's sh has no
  `/usr/local/bin` on its PATH; bash has it). A telnet login as root, with root
  logged in to the Finder, starts it too: the window comes on the Finder's screen.
- **Cmd-Q**, Ctrl-Q or File > Quit quits.
- Archaic looks for WRP at `http://192.168.1.10:8080/`: your computer, in the kit's network (QEMU NAT).
  On another network (a bridged VM, a real Mac): `archaic HOST:8080`, or set `WRP` to `HOST:8080`.
- The screen has 256 colors: the pictures have the 216 web colors (AB8).
- `screenshots/aux.png`: the Quadra's screen, 1152 x 870 in 256 colors, with Wikipedia's A/UX article.

**Measured in LEMU** (QEMU on an Apple Silicon Mac), with WRP on the same Mac:

| What | Time |
|---|---|
| `sh mkaux` (or `make`) | 3 s |
| Wikipedia's A/UX article, 216 colors (AB8, slices of 56 to 67 KB) | 3 s from Return: WRP's time. A slice comes and decodes in 1 or 2 ticks (17 to 33 ms) |

## Building on A/UX 3

Needs A/UX's C compiler, `cc`, and the A/UX Toolbox's files for programs: the headers in `/usr/include/mac`,
`/usr/lib/libmac_s.a`, `/usr/lib/maccrt0.o`, `low.o` and `low.ld`, and `/mac/bin/rez` and `fcnvt`.
The full A/UX install has them, and so does the kit.

1. Copy these 11 files into a folder on A/UX, with NFS, ftp or rcp:
   `src/archaic.h`, `main.c`, `html.c`, `http.c`, `gif.c`, `pic.c`, `update.c`, `nsock.c`, `mac.c`, `mkaux` and `auxrsrc.r`.
   With the whole repository, `make` at its top runs `mkaux`, and moves `archaic` and `%archaic` up.
2. Build:

   ```
   sh mkaux        # compile and link: archaic, about 232 KB, and its resources: %archaic, 832 bytes
   ```

   It takes 3 s in the kit. `cc` warns `& before array or function: ignored` 4 times (`&qd.black`): that is right.
3. Test: `./archaic --version` says `Archaic v0.11 for aux3`. `./archaic -t` (no window) loads WRP's form and lists its parts.
   **With the window:** `./archaic` in CommandShell.

NFS from your computer: A/UX's `mount` takes a host name from `/etc/hosts`, not an address
(`192.168.1.10 not in hosts database`). Put a line `ADDRESS NAME` in `/etc/hosts`, then
`/etc/mount -t nfs NAME:/dir /mnt`. Archaic runs from an NFS folder too.

## A/UX 3: what to know

Test on a copy of your A/UX disk, or start QEMU with `-snapshot`.

| Thing | A/UX 3.1.1 kit |
|---|---|
| Computer | Quadra 800 in QEMU (LEMU, `-M q800`): 68040, 128 MB |
| Screen | 1152 x 870, 256 colors |
| System | A/UX 3.1.1: Unix (System V with BSD parts) and the System 7 Finder |
| Network | SONIC Ethernet (`-nic ...,model=dp83932`), BSD sockets. QEMU NAT: A/UX 192.168.1.2, **your computer 192.168.1.10** (its 127.0.0.1). No default route |

- The A/UX Toolbox: an A/UX program (COFF) calls the Mac's ROM and System 7 through `libmac`.
  The manual: *A/UX Toolbox: Macintosh ROM Interface* (A/UX 3.0, 1992; on bitsavers, `pdf/apple/mac/a_ux/aux_3.0`).
  The display is `mac.c`, as on Mac OS 6 and 7.5, built with `-DAUX`. Apple's examples: `/mac/src/examples`.
- `cc` is K&R: `-DNOPROTO`. `-B /usr/lib/big/` gives it larger tables, as Apple's makefile does.
  It does not define `AUX`: `mkaux` does, as Apple's makefile.
- The headers are A/UX's own, included as `<mac/quickdraw.h>` and so on. Some names are older ones, and `mac.c`
  maps them: `GetItem`, `AddResMenu`, `SetCtlValue`, `SetCtlMax`, `GetCtlValue`, `DisposHandle`, `controlKey`,
  `app4Evt`, and `transIndex` in a color table for `ctFlags`. `qd` is a macro (`*__tb_qdVars`).
  There is no `GetMBarHeight` (`mac.c` takes 20) and no Apple events.
- Without `_AUX_C_EXTENSIONS` (the c89 compiler of A/UX Developer's Tools) the headers have no prototypes
  and no `pascal`. The calls take Pascal strings, and `Point` by value, as on the Mac.
- The link, as in Apple's makefile: `ld /usr/lib/maccrt0.o /lib/crt2.o ... /usr/lib/low.o -lmac_s -lat -lld -lmr -lc_s /lib/crtn.o /usr/lib/low.ld`.
  The sockets are in `libc_s`.
- The resource file: `rez` makes `Rez.out` (AppleSingle) from `auxrsrc.r`, and `fcnvt -f -d` makes an AppleDouble
  pair of it: `%archaic` is the file with the resources. A/UX's `types.r` has the older SIZE flag names
  (`multiFinderAware` ...), so `archaic.r` (Retro68's) does not go through A/UX's `rez`.
  The Mac heap is small (512 KB): Archaic's pages and pictures are Unix memory (`malloc`).
- Sharing the CPU: the manual says to wait in `WaitNextEvent`, which lets the other programs run, not in `select()`.
  So `nsock.c` waits in `mac_sel` (`mac.c`): `ui_setselect` makes `WaitNextEvent` return when the socket can be read
  (or written, for the connect). Then `select()` looks with no wait. The masks are cleared before the event is handled.
  `g_fd()` is -1. `YIELD` is as on the Mac.
- `maccrt0` does not flush stdio after `main` returns: `mac.c`'s `main` flushes `stdout` and `stderr`
  (without it, `--version`, `--help` and `-t` print nothing into a pipe or a file, such as through rsh).
- `noCD = 1` (a variable of `libmac`): the current folder stays the shell's (for `-o FILE`). Else it is the program's folder.
- `TickCount` runs as on the Mac: `g_ms` is right to a tick (17 ms). A slice that comes in less shows `under 1 ms`.
- LEMU's mouse tools: A/UX speeds the pointer up, so moves land short. Use `pointer-locate`, then `mouse-move`
  with `step-px` 1, then `mouse-click` with no x and y. Menus: `mouse-button` down, move, up. Cmd is the key `meta_l`.
- Tested (2026-11-26): A/UX 3.1.1 in LEMU, on the LAN (bridged), with WRP 4.14.2 on the Mac's LAN address.
  Started from CommandShell and from telnet, as root. Pages in 216 colors (AB8), a click on the page, the address box,
  Ctrl-U, Page Down, the scroll bar's arrow (held, it repeats), the Finder in front while a page loads
  (Archaic goes on, and draws its page behind the Finder's windows), the click that brings Archaic back (it does not
  go to the page), File > Quit, Cmd-Q, Ctrl-Q, `-t`, `-u` (`WRP has no update: no archaic-aux3 in release v0.11`).
  Not tested: a double-click in the Finder, the close box, the grow and zoom boxes, a drag of the scroll box, Edit > Paste,
  the About box and the desk accessories, a screen in black and white, the sums of the test pictures
  (`-t` with `test/data`, which needs a web server), gcc, a real Mac.
