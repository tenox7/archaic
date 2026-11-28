# Archaic on Windows NT (x86, MIPS, Alpha, PowerPC)

The parts that all systems share (Colors, Pictures, Updates, Porting ...) are in [AGENTS.md](../AGENTS.md).

## Running on Windows NT

- **Windows NT 3.1** and newer, on x86, MIPS and Alpha. On PowerPC, NT 3.51 and newer (NT starts there at 3.51).
  TCP/IP must be installed (Winsock).
- The release has a file for each CPU: `archaic-ntx86.exe`, `archaic-ntmips.exe`, `archaic-ntalpha.exe`,
  `archaic-ntppc.exe`. Copy it to NT as `archaic.exe`, in any folder (rcp, FTP, a shared folder, a floppy image).
- Start it from the Program Manager (File, Run): `C:\archaic.exe HOST:8080`. Or from a Command Prompt:
  `start archaic HOST:8080`. Without `start`, the prompt waits until Archaic quits.
- WRP's default address is `10.0.2.2:8080` (QEMU's NAT, as in the kits). An NT on the LAN needs WRP's address.
- Ctrl-Q or the close box quits. The window's scroll bar is NT's own.
- `screenshots/nt.png`: NT 3.1's screen on the Pentium PC, 1280 x 1024 in 256 colors: `ver` and `start archaic`
  in a Command Prompt, and Archaic with Wikipedia's Windows NT 3.1 article (216n, AB8).
  `screenshots/lemu-nt.png`: the same in LEMU's window, for README.md.

## Building for Windows NT

```
make nt              # or: sh src/mknt [x86 mips alpha ppc]
```

- In Docker: the image `tenox7/ntsdk` (github.com/tenox7/ntsdk) has Microsoft's own SDKs, and runs their tools
  with no Wine. `mknt` builds in `/tmp` in the container, with no network, from a copy of the sources.
- x86, MIPS and Alpha: the Win32 SDK for NT 3.1 (July 1993). PowerPC: the SDK for NT 4.0 with Visual C++ 4.1,
  linked with `-subsystem:console,3.51`, so it runs on NT 3.51.

| CPU | Compiler | File (KB) | Machine in the header |
|---|---|---|---|
| x86 | cl 8.00.3190a | 116 | 14C |
| MIPS | cl 8.00.081 ("Centaur") | 192 | 166 (R4000) |
| Alpha | cl 8.00.9B ("MSCGEM") | 267 | 184 |
| PowerPC | cl 10.10.6101, link 3.10 | 176 | 1F0 |

- It makes `archaic-ntx86.exe` ... next to `src`. The release has them with these names: for the system `ntx86`,
  WRP's updates take `archaic-ntx86.exe` (see Updates in AGENTS.md).
- Each `.c` is compiled alone (`cl -c`): the Alpha compiler of the NT 3.1 SDK stops with
  "c2n32: Severe: No such file or directory" when it gets several in one call.
- The NT 3.1 linker takes no `.res`: `cvtres` makes `archaic.rbj` of it first.
- `archaic.rc` has the icon, `archaic.ico`: the candle of Archaic's logo, lit, in the 3D look of NT 3.1's own icons
  (the front, the top, and the side at the right): a square candle on a brass tray.
  `tools/ntico.py` draws it. It has 3 pictures, and NT takes the one for its screen: 32 x 32 and 16 x 16
  in the 16 colors of the VGA (on a screen of 256 colors these are the system's own colors), and 32 x 32 in black
  and white. NT 3.1 uses the 32 x 32 one (Program Manager, a window made small); NT 4.0 also the 16 x 16 one
  (the title bar, the task bar).
- `-Ox` on all four. The decoders give the same pixels as on the Mac (the `-t` sums of `test/data`).

## Windows NT: what to know

- Only calls that NT 3.1 has. The SDK's KERNEL32.LIB and WSOCK32.LIB have exactly the names of NT 3.1's DLLs:
  a program that links, runs. NT 3.1 has no `SetScrollInfo` and no `CreateDIBSection`.
- The compilers define `_WIN32`, `_MSC_VER` and the CPU: `_M_IX86`, `_M_MRX000`, `_M_ALPHA`, `_M_PPC`.
  `archaic.h` gives the system names from these: `ntx86`, `ntmips`, `ntalpha`, `ntppc`.
- `near` and `far` are words of Microsoft's C: no variable can have these names.
  `IO.H` makes `creat` a macro: `update.c` takes it away. Files are opened `O_BINARY`.
- Archaic is a console program, so that `--help`, `-t`, `-v` and `-u` write to the console.
  Started from the Program Manager it gets a console of its own: `g_open` lets it go (`FreeConsole`) when nothing
  was written to it yet (its cursor is at 0, 0). So only Archaic's window stays.
- The display, `ntwin.c`: a window class with `CS_OWNDC` (one DC for all drawing), the ANSI variable font
  (`GetCharWidth` for the widths), `PatBlt` for boxes, `TextOut` for text.
  `WM_PAINT` gives Archaic an expose of the update rectangle. `-g WxH+X+Y` (or `=WxH+X+Y`) is the inside of the window;
  the default is 800 x 600, at most 9/10 of the screen.
- Pictures go to the screen as DIBs (`SetDIBitsToDevice`), rows bottom-up: 1 bit for black and white, 8 bits with the
  picture's colors.
- Colors: `GetDeviceCaps` (bits a pixel times planes). A screen with a palette (256 colors) gets a logical palette
  of Archaic's own: black and white, then the colors that the pictures use, up to 236 (the other 20 are NT's).
  WRP's 216n (the web colors) is the same on each page, so the palette stays. When a picture needs more, a new palette
  starts with black, white and its colors. `WM_QUERYNEWPALETTE` and `WM_PALETTECHANGED` put it in again.
- The scroll bar is a scroll bar control at the right of the window (`g_bar` 0). Its range is the page's place over
  `sk`, so that it fits in 30000 (a scroll bar takes 16 bits). While the button is down on it, the control runs a loop
  of its own: each `WM_VSCROLL` calls `ui_poll()`, and the page moves at once. In that loop `g_event` gives only the
  events in its queue, and takes no messages: they are the control's (else it does not see the button go up, and goes
  on scrolling). After the loop, `g_event` goes back to `main.c` with no event, so the slices that are needed come.
- `g_copy` moves the pixels with `ScrollDC`. It returns 0 (all is drawn again) when a `WM_PAINT` waits,
  or when a part of the window was covered (the region `ScrollDC` gives is more than the new rows).
- The mouse wheel (`WM_MOUSEWHEEL`, NT 4.0 with a wheel mouse): a quarter of the frame a step.
  The right button scrolls as on the other systems. Page Up, Page Down, the arrows, Home and End scroll.
- `g_ms()` is `GetTickCount()`. `g_fd()` is -1: `n_read` waits 1/20 s in `select()`, then the window gets its turn.
  `g_event` waits 1/10 s with `MsgWaitForMultipleObjects`.
- Moving or sizing the window: while the button is down, NT runs a loop of its own, and Archaic waits (a page that
  loads stops meanwhile). A new width: WRP's picture at that width comes 0.4 s after (see A new window size).
- The network, `nwin.c`: Winsock 1.1 (WSOCK32.DLL). The connect does not hold up the window (`FIONBIO`, then `select()`;
  a failed connect is in its third set). Esc stops it; after 20 s Archaic gives up.
- Updates (`update.c`): `GetModuleFileName` finds the program (`nt_self`). NT can't delete the program that runs, but
  it can rename it: so `archaic.exe` goes to `archaic.old`, then the new file takes its name. The new file comes as
  `archaic.NNN` (8.3 names, for FAT). NT's `access()` sees only the read-only flag: a folder that is read-only gets
  `To upgrade, start it as Administrator`; a folder that NTFS keeps from the user gets `unable to write archaic.NNN`.
  In both cases nothing changes.

## Tests

Tested on 2026-11-27 in LEMU, on private copies (APFS clones) of the VMs in `~/VM/NT*`, with NAT, rsh (ntrshd)
on a local port, and WRP 4.14.3:

- NT 3.1 on x86 (`NT31INTEL`: Pentium, S3 928, 1280 x 1024 in 256 colors): the `-t` sums (ABM, ABM3, AB8, GIF in
  2 and 256 colors); in the window, pages, the picture test (AB8), the C menu, the palette, the address box,
  the scroll bar (an arrow held, the box dragged, a click in the shaft), Page Down, the right button, a new width,
  typing into the page (with the new picture after 3 s), clicks on the picture, Ctrl-Q.
  `-u` with a test WRP: installed 99.0 and kept `archaic.old`, then "is the newest"; a wrong CRC: nothing changed,
  no file left.
- NT 3.1 on MIPS (`NT31JAZZ`: Magnum R4000, G364 in 256 colors), NT 3.1 on Alpha (`NT31JENSEN`: S3 928 in 256 colors)
  and NT 3.51 on PowerPC (`NT351PPC`: 40p, P9100 in 65536 colors): the `-t` sums, and in the window pages,
  Page Down and the scroll bar.
- One WRP for each client: WRP keeps one page. When two VMs used the same WRP, the first one's next slice failed
  (`Picture: can't show the slice`, and WRP's log said `Unable to find image`).
- The release files of v0.14 (built from the same sources, only the time stamps differ), with NAT and rsh on a local
  port, in all the NT VMs of `~/VM`: NT 3.1 (x86, MIPS, Alpha), NT 3.51 (x86, MIPS Magnum and RM200, Alpha, PowerPC)
  and NT 4.0 (x86, MIPS Magnum and RM200, Alpha, PowerPC). On each: `--version`, the `-t` sums of an ABM and an AB8,
  and `-u` with WRP 4.14.5 and the GitHub release (`Archaic 0.14 is the newest`). NT 4.0 on x86 also in the window:
  WRP's page, the picture test (AB8), a page, Page Down.
- The candle icon and the candle logo (the release files of v0.14, built again with them): on copies of NT 3.1
  and NT 4.0 on x86, Program Manager and a window made small (NT 3.1), the desktop, the title bar and the task bar
  (NT 4.0), and the logo lit while a page loads. The 4 programs have the 3 pictures (their resources are the
  same bytes as `archaic.ico`). On NT 3.1 (x86, MIPS, Alpha) and NT 3.51 (PowerPC): `--version` and the `-t` sums.
  Program Manager keeps an item's old icon: Properties, then OK, reads it again.
- The NT VMs in `~/VM` are bridged on the LAN. For a test, `LEMU_NIC` makes it NAT, with `hostfwd` to the guest's
  port 514 for rsh. NT 3.1's VMs have fixed addresses: their `runs.sh` has a NAT network for them.
  NT 3.1 on MIPS asks to connect a network drive at logon: answer No.
- Not tested: NT 3.5, a screen of 16 colors, a real computer. NT for the DECstation (R3000): the MIPS build is for
  the R4000.
