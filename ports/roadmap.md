# Archaic: the next ports

The parts that all systems share (Colors, Pictures, Updates, Porting ...) are in [AGENTS.md](../AGENTS.md).

## Roadmap: poor systems with no web browser

Old systems that never had a web browser: the next ports.
Each port is a display file, plus a network file if the system has no BSD sockets (see Porting).

| Target | What it is | Systems |
|---|---|---|
| MGR | a window system on a terminal line | SunOS, Coherent, Xenix, Minix, OS-9, VSTa |
| Blit, DMD 5620 | a graphics terminal on a serial line | Research Unix (System V on the AT&T 3B2, with layers: done) |
| QNX Windows | QNX's window system before Photon | QNX 2 |
| SunWindows, SunView | Sun's window system before X11 | SunOS 1 to 4, on Sun-1 to Sun-3 (SunOS 4 on a Sun-4, SunOS 3.5 on a Sun-3: done) |
| VWS, EWS | DEC's window systems before X11 | VAX/VMS (VWS), VAXELN (EWS) |
| NeWS, Display PostScript | PostScript displays | SunOS, SGI, OS/2, VMS, NeXTSTEP, AIX ... |
| Windows 1 and 2, OS/2 1.x | early PC window systems: TCP/IP over SLIP | Windows 1.0 and 2.x, OS/2 1.1 to 1.3 |
| Tektronix 4010, 4014, 4105 | graphics terminals on a serial line: lines on a storage tube, or pixels (4105) | any host with a network: 2.11BSD, 4.3BSD, VMS ... |

### MGR

[MGR](https://en.wikipedia.org/wiki/ManaGeR) (ManaGeR) is a window system by Stephen Uhler
at Bellcore, from 1984. It started on Sun workstations.
A program in an MGR window writes escape sequences to its terminal:
text, lines, bitmaps, fonts, menus. Events, like a mouse click, come back as text.

- Plan: one display file, for all the systems with MGR.
- MGR works on any terminal line, also telnet or a modem. So Archaic can run
  on another computer (one with a network), and show in an MGR window.
- Source: MGR 0.69, at [hack.org/mc/mgr](https://hack.org/mc/mgr/).

| System | MGR there |
|---|---|
| SunOS (Sun-3, Sun-4) | the first system. Network: BSD sockets, as `nsock.c` |
| Coherent (PC) | MGR 0.69, Harry Pulley's port |
| Xenix | an older MGR |
| Minix (386) | an older MGR |
| OS-9, VSTa | MGR ran there too |

To find: TCP/IP on Coherent, Xenix, Minix, OS-9 and VSTa.

### Blit and DMD 5620

The [Blit](https://en.wikipedia.org/wiki/Blit_(computer_terminal)) is a graphics terminal
by Rob Pike and Bart Locanthi at Bell Labs, 1982.
AT&T and Teletype sold it as the **DMD 5620** (1984): a WE 32100 CPU, 800 x 1024 pixels, 1 bit, green.
It connects to a Unix host on a serial line, and the host loads programs into it.
Its window system is mpx or mux (Research Unix), or layers (System V Release 3).

- Done: the DMD 5620 in layers, on the AT&T 3B2 with UNIX System V Release 3.2
  (`dmd.c`, `dmdterm.c`: see [3b2.md](3b2.md)). Archaic is in two parts.
  On the host: the network, the HTML and WRP's form. In the terminal: a program that draws,
  keeps and decodes the page's picture, and reads the mouse.
- Next: mpx or mux on Research Unix (V8, V9, V10), with the Blit or the DMD 5620. The terminal's part
  needs their library, and the host's part their tty.
- Emulators: the Blit in 9front, with mux on Research Unix v8 (SIMH).
  For the DMD 5620: [Seth Morabito's emulator](https://loomcom.com/3b2/dmd5620_emulator.html),
  made for the AT&T 3B2/400 in SIMH. It also works with mux on Research Unix V8, V9 and V10 (firmware 8;7;3).

### QNX 2 with QNX Windows

QNX 2 (Quantum Software Systems, 1987) is a small real-time system for the PC (8088, 286).
QNX Windows is its window system, before Photon.
[QNX Windows, first look](https://virtuallyfun.com/2013/03/22/the-shape-of-things-to-come/)
(virtuallyfun, 2013): QNX 2.2 in QEMU, 800 x 600 (`-vga cirrus`, `qw.vga_bios video7,1`).
The mouse did not work in QEMU.

- Plan: a display file for QNX Windows.
- QNX 2 is 16-bit: see the memory note in Porting.
- To find: the QNX Windows library, and TCP/IP for QNX 2.

### SunWindows and SunView: Sun-1 to Sun-4

Sun's window system before X11. **SunWindows** came with SunOS 1.0 (1983), on the Sun-1 and Sun-2.
**[SunView](https://en.wikipedia.org/wiki/SunView)**, its toolkit, came with SunOS 3.0,
and stayed up to SunOS 4.1: Sun-2, Sun-3, Sun-4.
OpenWindows (X11) took its place, and still showed SunView programs.

- Done: SunView on SunOS 4.1.4, on a Sun-4 (`sunview.c`, see [sun4.md](sun4.md)),
  and on SunOS 3.5, on a Sun-3 (see [sun3.md](sun3.md)). Network: `nsock.c`.
- Next: the Sun-2 (68010), with the same `sunview.c`.
  Then SunWindows on SunOS 1 (Sun-1, Sun-2), with no SunView toolkit.

### VAX before X11: VWS and EWS

DEC's window systems before DECwindows (X11):
**VWS** (VAX Workstation Software) on VMS, with its library **UIS**,
and **EWS** on VAXELN, DEC's real-time system.

- Computers: VAXstation I and II (QVSS, black and white), II/GPX (QDSS, color), 2000, 3100.
- Emulator: SIMH. `microvax2` with `SET CPU MODEL=VAXstation` is a VAXstation II.
  [MicroVMS 4.7 and VWS 3.3 work there](https://gunkies.org/wiki/Installing_MicroVMS_V4.7_and_VAX_Workstation_Software_V3.3_on_SIMH).
- Plan: a display file for UIS.
- Network on VMS: TCP/IP from Wollongong (`ntwg.c` is for their 3B1 TCP/IP), CMU-Tek,
  TGV (MultiNet) or DEC (UCX).

### PostScript displays: NeWS and Display PostScript

A program sends PostScript to the display, and the display draws it.
So one display file can work on many systems:
the picture as a PostScript `image`, the text with `show`.

- [NeWS](https://en.wikipedia.org/wiki/NeWS) (Sun, 1986, James Gosling and David Rosenthal): SunOS.
  Ports: SGI's 4Sight, NeWS/2 (OS/2), MacNeWS, TGV's X11/NeWS (VMS on a VAXstation 2000),
  the Amiga 2000, Apollo and others. Sun's OpenWindows 3.3 (1993) dropped it.
- [Display PostScript](https://en.wikipedia.org/wiki/Display_PostScript) (Adobe and NeXT, 1987):
  NeXTSTEP, DEC's DECwindows, IBM and SGI workstations, Sun's OpenWindows 3.3.
  On most of them, it works in X11.

### Windows 1 and 2, OS/2 1.x: TCP/IP over SLIP

Early window systems on the PC. They are 16-bit, and have no TCP/IP of their own:
**Windows 1.0** (1985) and **Windows 2.x** (1987),
and **OS/2 1.1 to 1.3**, with Presentation Manager (from 1988). OS/2 1.0 had text mode only.

- Plan: a display file for Windows (GDI), and one for Presentation Manager.
- Network: SLIP on the serial port, with a small TCP/IP in Archaic, as netd does in Idris.
  The same code for both systems: only the serial port calls change.
  Archaic opens one connection at a time (`http.c`), so the TCP part can stay small.
- On the host: a SLIP bridge to libslirp, as `slirpbridge` in the Idris kit. The host is then 10.0.2.2.
- 16-bit: see the memory note in Porting.

### Tektronix terminals: 4010, 4014 and 4105

The [Tektronix 4010](https://en.wikipedia.org/wiki/Tektronix_4010) (1972) is a graphics terminal on a serial line.
Its screen is a storage tube: a line stays on the screen after it is drawn, with no screen memory.
It draws text and lines (vectors), green on dark, on 1024 x 780 points.
It can erase only all of the screen.
The 4014 has a larger screen, with 4 times more points in each direction.
Many systems drew on them: Tektronix's PLOT 10 library, Unix's `plot -T4014`.
The [4105](https://en.wikipedia.org/wiki/Tektronix_4105) is a color raster terminal (480 x 360 pixels).
It also knows the 4010's commands.

- Plan: one display file for the Tek commands. As with MGR, Archaic runs on the host
  (one with a network), and sends the commands on the terminal line.
- Pictures on the 4010 and 4014: each run of black pixels in a row is one line.
  The page shows bright on dark.
- Pictures on the 4105: its pixel operations, RASTER WRITE or RUNLENGTH WRITE
  (see the [4105 Programmer's Reference](https://vtda.org/docs/computing/Tektronix/070-4526-03_4105ComputerDisplayTerminalProgammersRef_Feb86.pdf), 1986).
- Clicks: GIN mode. The user moves the crosshair (with two thumbwheels on the 4010) and presses a key.
  The terminal sends the key and the place.
- Scrolling: erase the screen, then draw it again.
- Emulators: xterm's Tek window (a 4014: `xterm -t`), and René Richarz's
  [tek4010](https://github.com/rricharz/tek4010) (4010 and 4014), used with 2.11BSD and RSX-11 in SIMH.
  Many emulators do not have the 4105's pixel operations.
- To find: how long one picture takes on a 9600 baud line. A page of text has many runs of black pixels.
- 2.11BSD (PDP-11) is 16-bit: see the memory note in Porting.
