# Archaic on FreeMiNT (Atari, GEM)

The parts that all systems share (Colors, Pictures, Updates, Porting ...) are in [AGENTS.md](../AGENTS.md).

## Building for the Atari (FreeMiNT, GEM)

Archaic for FreeMiNT with GEM (XaAES), as in the ARAnyM kit `freemint119-aranym.tar.lz`,
is built on your computer, with Vincent Riviere's m68k-atari-mint cross tools in Docker:

```
make mint       # or: sh src/mkmint
```

- It needs Docker. The first run makes the image `archaic-mint`: Ubuntu 24.04 with `ppa:vriviere/ppa`
  (gcc 4.6.4, MiNTLib, GEMLib), amd64: on Apple silicon Docker runs it with Rosetta.
- It makes `archaic-mint` (about 230 KB): a MiNT program for a 68000 or newer, with a 128 KB stack.
  Git ignores it. The system name is `mint` (`__MINT__`): the release file is `archaic-mint`.
- In FreeMiNT, put it in a folder on the PATH as `archaic`. In the ARAnyM kit the PATH is `/sbin:/bin:/usr/sbin:/usr/bin`
  and there is no `/usr/local` (kits before 2026-11-26 v4 have no `/usr` at all): use `/bin/archaic`
  (on the host: `drive_c/mint/1-19-2af/sys-root/bin`). Copy it in through a HostFS drive (C: is the folder `drive_c`).
- Start it from a shell: Desk > TosWin2, File > Start shell, then `archaic &`.
  Under XaAES a program from a shell opens GEM windows.
- WRP's default address is `10.0.2.2:8080`: in ARAnyM's slirp, that is port 8080 of the host.
  Start WRP on the host with `wrp -l 127.0.0.1:8080`.

## FreeMiNT: what to know

- `gem.c` draws into `fb.c`'s screen in memory (0xRRGGBB). `g_flush` copies the changed part to the
  window with the VDI, through GEM's rectangle list (`wind_update` locks the screen meanwhile).
- The screen's pixels come from `vq_scrninfo`:
  - packed 32-bit 0x00RRGGBB (ARAnyM's fVDI, `mode ...x32` in `fvdi.sys`): `vro_cpyfm` takes `fb`'s rows
    as they are, when the window is a multiple of 16 pixels wide; else they are copied to a buffer first.
  - other packed pixels (15, 16 and 24 bits, also Intel byte order): through tables made from the
    bit numbers that `vq_scrninfo` gives.
  - bit planes, and pixels with a palette (2 to 256 colors): black and white with `vrt_cpyfm`.
    `g_colors()` is then 2, and WRP sends ABM pictures. No palette is set.
- The window has GEM's scroll bar (`g_bar` 0). Its arrows send `S_STEP`, its grey part `S_PAGE`,
  its slider `S_TO` (GEM's 0 to 1000, times the page's max). `g_sbar` sets the slider only when it changes.
- Keys: the up and down arrows scroll a quarter, with Shift a window (also a PC's Page Up and Page Down).
  ClrHome goes to the top, Shift+ClrHome to the end. Undo is Esc.
  The Atari's characters 0200 to 0377 become ISO 8859-1 (`latin[]`); the window's title goes the other way.
- The mouse: `evnt_multi` with `MU_BUTTON` (any button down, then all up) and `MU_M1` (the mouse moves
  while a button is down) gives `E_DOWN`, `E_MOVE` and `E_UP`. A click outside the work area, or on
  another window over Archaic's, gives none.
- Quit: Ctrl-Q, the close box, or `AP_TERM` (`shel_write(9, 1, ...)` says Archaic knows it).
- GEM has no file for `select()`: `g_fd()` is -1, so `nsock.c` waits 1/20 s at most, and
  `ui_poll()` takes GEM's events with no wait.
- `wind_set_str(WF_NAME)`: the AES keeps the pointer, so the title is a static buffer.
- Tested (2026-11-26): ARAnyM 1.2.0 Unofficial Beta 3, FreeMiNT 1.19 with XaAES and TeraDesk,
  fVDI 1280x1024x32, 1024x768x16 and 1024x768x8 (black and white), WRP 4.14.2:
  pages in colors (AB8), links, typing in the address box, Ctrl-U, the scroll bar (arrows, grey part,
  slider), the arrow keys, the fuller and the sizer (WRP makes the page again for the new width),
  the close box and Ctrl-Q.
  Not tested: a real Atari, MagiC, N.AES, an ST's monochrome screen, 24-bit screens.
