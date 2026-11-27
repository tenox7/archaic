# Archaic on SunOS 3.5 (SunView, Sun-3)

The parts that all systems share (Colors, Pictures, Updates, Porting ...) are in [AGENTS.md](../AGENTS.md).

## Running on SunOS 3.5 on a Sun-3

- **SunOS 3.5** with SunView, on a Sun-3 (68020), in TME (The Machine Emulator):
  a Sun-3/160 with the bwtwo screen, and slirp for the network.
  Kit: `sun3-sunos35-tme`, on osarchive in `os/sunos/Virtual/tme/`.
  Archaic is in the kit: `/usr/local/bin/archaic`, and its source in `/usr/local/src/archaic`.
- Release binary: `archaic-sun3` (the kit's). Copy it to SunOS as `/usr/local/bin/archaic`,
  then `chmod 755 /usr/local/bin/archaic`. Or build it there: see Building on SunOS 3.5.
- Log in as `root` at the console and type `suntools` (SunOS 3's name for SunView).
  In a shelltool, type `archaic`.
- **Ctrl-Q** quits, or Quit in the frame's menu.
- With TME's slirp, your computer is `10.0.2.2` for SunOS (SunOS is `10.0.2.15`).
  So Archaic on a Sun-3 looks for WRP at `http://10.0.2.2:8080/`.
- `screenshots/sun3.png`: the Sun-3's screen (bwtwo), 1152 x 900, black and white:
  `/usr/5bin/uname -a` and the release line of `/etc/motd` in a shelltool,
  and Wikipedia's Sun-3 article, in 2 s.

## Building on SunOS 3.5

The same 10 files and the same script as on SunOS 4 (see Building on SunOS 4, in [sun4.md](sun4.md)), with Sun's `cc`:

```
sh mksunview    # compile and link: archaic, about 580 KB
```

It takes about 30 s in TME. The program is large: SunOS 3.5 has no shared libraries.
Test mode: `./archaic -t http://10.0.2.2:8077/test/data/hn2.gif` ends with `sum ae2b58f3`.

**Measured in TME** (on an Apple Silicon Mac), from Return until the picture is complete:

| What | Time |
|---|---|
| decode check (`-t`, `hn2.gif`, 11 KB, 256 colors) | 0.6 s |
| decode check (`-t`, `hn2.abm`, 9 KB) | less than 0.1 s |
| Wikipedia's Sun-3 article in the window (ABM, 26 KB) | 2 s |

## SunOS 3.5: what to know

Test on a copy of your SunOS disk.

| Thing | SunOS 3.5 in TME |
|---|---|
| Computer | Sun-3/160: 68020 and 68881, 32 MB |
| Screen | bwtwo: 1152 x 900, black and white |
| Window system | SunView 1: `suntools` at the console |
| Network | Intel Ethernet (`ie0`), BSD sockets (4.2BSD). TME's slirp: SunOS 10.0.2.15, **your computer 10.0.2.2** (its 127.0.0.1) |

- Sun's `cc` defines `sun`, `mc68000` and `mc68020`, but not `sun3`.
- gcc 1.42 (if `/usr/local/bin` has it) stops in `<sundev/vuid_event.h>`. Use Sun's `cc`.
- `<suntool/sunview.h>` guards itself with the name `SUNVIEW` (`#ifndef SUNVIEW.H`).
  The build's `-DSUNVIEW` made it empty, so `sunview.c` undefines `SUNVIEW` first.
- `<pixrect/memvar.h>` declares `mem_ops` only in its `mpr_static` macro: `sunview.c` declares it.
- `<sys/types.h>` has `fd_set` (one `int`), but no `FD_SET`, `FD_ZERO` or `FD_ISSET`:
  `nsock.c` and `sunview.c` define them.
- `time_t` is only in `<sys/types.h>`, not in `<time.h>`.
- `stderr` is buffered when it goes to a file: Archaic flushes each `-v` line.
- For Archaic, SunView is the same as on SunOS 4: the same toolkit calls, and the same
  variables for a program started from rsh (`WINDOW_PARENT=/dev/win0`, `WMGR_ENV_PLACEHOLDER=/dev/win1`).
- The shell's tools are old: no `printf`, and `grep` has no `\|` (use `egrep`).
  `uname` is only System V's, `/usr/5bin/uname`: it is not on root's path, and it does not
  name the release. `head -1 /etc/motd` does.
