# Archaic on TOS (Atari ST, STE, TT, Falcon)

The parts that all systems share (Colors, Pictures, Updates, Porting ...) are in [AGENTS.md](../AGENTS.md).

## Running on TOS

- **TOS** with GEM on an Atari ST, STE, Mega ST, TT or Falcon (a 68000 or newer), with **STinG** 1.26
  and a NetUSBee or EtherNEC card (`ENEC.STX`). Tested in Hatari with its NetUSBee and NAT:
  the Mega ST VM in `~/VM/ataritos` (TOS 2.06, STinG in `C:\AUTO`).
- Copy `archaic-tos` to the Atari as `ARCHAIC.PRG`, in any folder. The desktop gives a `.PRG` no command line:
  Archaic reads its options from `ARCHAIC.INF` next to it (words, as on a command line), such as `http://HOST:8080/ -v`.
  A command line (a shell, a `.TTP` name) goes before the file. With `-v` the messages go to `ARCHAIC.LOG` there,
  not over GEM's screen.
- Open `ARCHAIC.PRG` on the desktop. Ctrl-Q or the close box quits.
- WRP's default address is `10.0.2.2:8080`. Hatari's NAT (NeoST's) takes 10.0.2.2 to the host's 127.0.0.1 only with
  `--nat-loopback on` (tenox7/hatari, branch netusbee; `bNatLoopback` in the `[Net]` section of hatari.cfg).
  Without it, start WRP on the host's LAN address and give that in `ARCHAIC.INF`, for example
  `docker run --rm -p LANIP:8080:8080 tenox7/wrp:4.14.3-arm64`.

## Building for TOS

```
make tos        # or: sh src/mktos
```

- The image `archaic-mint` of `mkmint` (gcc 4.6.4, MiNTLib, GEMLib). MiNTLib runs on TOS with no MiNT.
- `main.c` is compiled with `-Dmain=archaic_main`: `tos.c` has `main()`. `-DATARITOS` gives the system name `tos`
  (`__MINT__` is set too: `archaic.h` looks at `ATARITOS` first).
- It makes `archaic-tos` (about 220 KB), with a 128 KB stack.

## TOS: what to know

- The screen: `v_opnvwk` gives its size, `vq_extnd` its planes and whether it has a palette.
  `vq_scrninfo` only with the cookie `EdDI` (NVDI, fVDI): TOS's own VDI and EmuTOS do not have it.
  The kind of `tos.c`'s pixels:

| Screen | Pixels in memory | Copy | `g_colors()` |
|---|---|---|---|
| 1 plane: ST high 640x400, TT high | 1 bit, 1 black | `vrt_cpyfm`, pens 1 and 0 | 2 |
| 2 planes: ST medium 640x200, 4 colors | 1 bit | `vrt_cpyfm` | 2 |
| 4 and 8 planes: ST low 320x200, TT medium 640x480, TT low, Falcon 16 and 256 | the Atari's interleaved planes: 16 pixels in n words | `vro_cpyfm` | 16, 256 |
| 8 bits, a byte a pixel (a card: `vq_scrninfo` says packed) | bytes | `vro_cpyfm` | 256 |
| 15, 16, 24, 32 bits (cards, fVDI) | as `vq_scrninfo` says | `vro_cpyfm` | millions |
| 16 bits on a Falcon, no `vq_scrninfo` (TOS 4, EmuTOS) | RGB 565 | `vro_cpyfm` | millions |
| anything else | 1 bit | `vrt_cpyfm` | 2 |

- Archaic draws into these pixels itself (the 6x13 font of `fb.c`), and `g_flush` and `WM_REDRAW` only copy
  through GEM's rectangle list.
  `g_copy` moves whole rows in memory (a part of a row: it returns 0).
- With a palette (16 or 256 colors), the pictures' colors take pens of Archaic's own: 16 to 255 (0 to 15 stay GEM's),
  with 16 colors 2 to 15. For each picture: a pen for each color it uses, the most used first (`vs_color`, then
  `vq_color` for the color the hardware shows). When no pen is free, the pens that no pixel in the window shows are free
  again; then the nearest color. At the end the palette is set back. With 16 colors, WRP makes a palette for each slice,
  so a slice can get the nearest colors when the one above it holds the pens. With 216n (256 colors) all colors fit.
- VDI's pen numbers are not the pixel values (pen 1, black, is 15 on ST low): `tos.c` draws each pen on the window
  at the start and reads it back with `v_get_pixel`.
- AES 4 only (MultiTOS, MagiC, XaAES): `shel_write(9)` (AP_TERM) and `menu_register`. On TOS 1 and 2 they would
  start a program or add Archaic to the Desk menu.
- `nsting.c`: STinG's functions are Pure C `cdecl`. Their arguments go on the stack in their own sizes (16-bit words),
  and they may change d2, which gcc keeps. `st_call()`, a few lines of assembler, pushes the words and saves the
  registers. `TCP_open` returns at once; Archaic asks `TCP_info` for the state while the window is drawn (Esc stops).
  `CNbyte_count` and `CNget_block` read, `TCP_send` writes (`E_OBUFFULL`: again), `TCP_close` with no wait.
  `n_read` waits 1/20 s at most (`_hz_200`).
- The clock: `g_ms()` reads `_hz_200` (Supexec).
- Tested (2026-11-27) in Hatari 2.6.1 with NetUSBee and its NAT, STinG 1.26 with ENEC.STX, WRP 4.14.3:
  Mega ST, TOS 2.06, ST high (mono) and, on a color monitor, ST low (16 colors) and ST medium (black and white);
  TT with EmuTOS 1.4, TT medium (16 colors); Falcon with EmuTOS 1.4, 640x480 in 256 colors and 320x480 in
  true color (16 bits). Pages, typing in the address box, the arrow keys, the scroll bar, Ctrl-Q.
  The picture test picks ABM on the 8 MHz ST (ABM3 is slow there), AB8 on the color screens.
  TT with MagiC 6.2 (its ROM image `MAGIC62E.IMG` in place of TOS), TT high 1280x960: EdDI's `vq_scrninfo`, the window
  on MagiC's desktop, real-time sizing (WM_SIZED while the button is down: only the window changes, the new buffer
  and WRP's picture at the new width come at the release; see `resize()`).
  Not tested: a real Atari, NVDI and graphics cards, TT low (256 colors, as the Falcon's).
- Hatari for tests: EmuTOS does not run AHDI's boot sector, so it boots from A: and `C:\AUTO` does not run.
  Start `C:\AUTO\STING.PRG` from the desktop (STinG stays after a resolution change). The Falcon has no ACSI:
  give the same disk image with `--ide-master`. The NetUSBee takes the cartridge port, so no GEMDOS drive (`--harddrive`).
- MagiC in Hatari: osarchive `os/tos/desktops/magic/MAGICE62.ZIP`. `--tos MAGIC62E.IMG` (MagiC as a 256 KB ROM, also on a TT)
  and its `TO_C_ROOT` files on C:. It boots from the ACSI disk like TOS, so `C:\AUTO\STING.PRG` runs.
