# Archaic on NetBSD/m68k

The parts that all systems share (Colors, Pictures, Updates, Porting ...) are in [AGENTS.md](../AGENTS.md).

## Running on NetBSD/m68k

- **NetBSD 6.1.4** with X11, on an Atari TT030 (68030, 68882). Tested in Hatari (tenox7/hatari, branch main)
  with its NetUSBee card (NetBSD `ne0`) and NAT: the VM in `~/VM/ttnetbsd`, and its kit
  `ataritt-netbsd614-hatari` on osarchive in `os/netbsd/virt/`.
  Archaic is in the VM (and in its kits after release 2): `/usr/local/bin/archaic`, and its source in `/usr/local/src/archaic`.
- Release binary: `archaic-netbsdm68k`. It is one ELF program for NetBSD 6 and newer on 68k machines
  with a 68020 or newer and an FPU, or the kernel's FPU emulation (Atari, Amiga, Mac, HP 300 ...).
  Only the Atari TT is tested.
  Copy it as `/usr/local/bin/archaic`, then `chmod 755 /usr/local/bin/archaic`. Or build it there: see Building on NetBSD.
- Log in as `root` at the console, then `startx` (twm and xterms). In an xterm, type `archaic &`.
- **Ctrl-Q** quits.
- Archaic looks for WRP at `http://10.0.2.2:8080/`: your computer, in Hatari's NAT
  (with `--nat-loopback on`, as the VM's `run.sh` has it). Elsewhere: `archaic HOST:8080`, or set `WRP` to `HOST:8080`.
- The TT's screen in high resolution is 1280 x 960 with 1 bit (mono): the pictures are ABM, dithered to black and white.
- `screenshots/hatari-netbsd.png`: the TT's screen, 1280 x 960 mono, with twm: `uname -a` in an xterm,
  and Wikipedia's Atari TT030 article, in 17 s from Return until the page is complete (WRP 4.14.5 in Docker on the Mac).

## Building on NetBSD

Needs NetBSD's `cc` (gcc) and X11 (the `comp`, `xbase` and `xcomp` sets). The VM has gcc 4.5.3 and X11R7 in `/usr/X11R7`.

1. Copy `Makefile` and the folder `src` to NetBSD, with ftp, rcp or NFS.
   In the VM: `mount -t nfs -o -T HOST:/dir /mnt` (NFS over TCP passes Hatari's NAT).
2. Build:

   ```
   make            # archaic, about 105 KB (90 KB after strip)
   ```

   `make` sees NetBSD in `uname`: it adds `-I/usr/X11R7/include -L/usr/X11R7/lib -Wl,-R/usr/X11R7/lib`.
   Without the `-R`, the program does not find `libX11.so.7` when it starts.
   It takes 30 minutes in the VM, 12 of them for `main.c` (Hatari runs the TT at its real speed).
3. Test. **Test mode, no window needed** (serve this folder from your computer: `python3 -m http.server 8077`):

   ```
   ./archaic -t http://10.0.2.2:8077/test/data/hn2.gif
   ```

   The last line ends with `sum ae2b58f3`: the same as on the other systems.
   NetBSD's own web server can serve the folder inside NetBSD, with no server on your computer:
   `/usr/libexec/httpd -b -I 8077 DIR`, then `http://127.0.0.1:8077/test/data/hn2.gif`.

   **With the window:** `./archaic` in an xterm.

## NetBSD/m68k: what to know

Test on a copy of your NetBSD disk.

| Thing | NetBSD 6.1.4 VM |
|---|---|
| Computer | an Atari TT030 in Hatari: 68030 at 32 MHz, 68882, 4 MB ST-RAM, 64 MB TT-RAM |
| Screen | TT high resolution: 1280 x 960, 1 bit (mono) |
| Window system | X11R7 libraries (NetBSD 6), the X server of XFree86 3.3.6 (`XF68_FBDev`, from NetBSD 1.6.2), twm |
| Network | Hatari's NetUSBee (an NE2000 on the cartridge port, NetBSD `ne0`), BSD sockets. Hatari NAT: NetBSD 10.0.2.15, **your computer 10.0.2.2** (its 127.0.0.1) |
| Names | DNS at 10.0.2.3 |

- `SYSID` is `netbsdm68k` (`__NetBSD__` and `__m68k__`).
- gcc 4.5.3 makes 68020 code with 68881 FPU instructions (`-m68020` is its default here).
  `file archaic` says `ELF 32-bit MSB executable, Motorola 68020`. `ldd archaic` lists `libX11.so.7` in `/usr/X11R7/lib`.
- Two warnings, both harmless: `-pedantic` warns about a named variadic macro in `Xfuncproto.h`,
  and gcc 4.5 does not know `-Wno-deprecated-non-prototype`.
- `strip` keeps the marks for the updates (`strings archaic | grep '@(#)'`).
- From a serial login as root, `DISPLAY=:0` reaches the screen (with root's X session on the console).
- Hatari's MCP mouse tools can't read X's pointer, and X speeds up large moves (a move lands twice as far).
  Move the pointer in small steps (`mouse-move` with `step-px` 1), then click with no x and y.
- System programs (`mount`, `ifconfig`) are in `/sbin`: rsh's PATH does not have it.
