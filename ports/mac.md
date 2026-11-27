# Archaic on the classic Mac OS (System 6, Mac OS 7.5)

The parts that all systems share (Colors, Pictures, Updates, Porting ...) are in [AGENTS.md](../AGENTS.md).

## Running on Mac OS 6

- **System 6.0.8** on a Macintosh IIcx (68030), with MacTCP 2.0.6, in the
  [Snow](https://github.com/twvd/snow) emulator (1.6). Snow emulates a DaynaPORT SCSI Ethernet box,
  with NAT to your computer's network.
- Release files: `archaic-macos6.dsk`, an 800 KB floppy image with Archaic (put it in Snow's floppy drive),
  and `archaic-macos6.bin`, the same Archaic as MacBinary, for a real Mac. Or build them: `make mac`
  (see Building for the classic Mac OS). WRP's `/archaic-download` does not give them: it takes only names with no dot.
- Open Archaic in the Finder: click its icon, then File > Open (Cmd-O). From the floppy it takes about 10 s to start.
  Its window fills the screen under the menu bar.
- **Cmd-Q** quits (or Ctrl-Q, or the close box).
- In Snow, the Mac's `127.0.0.1` is your computer's. So Archaic looks for WRP at `http://127.0.0.1:8080/`
  with no options.
- Options, like WRP's address: the Mac has no command line, so they go in a text file `Archaic Options`
  in Archaic's folder. One line, as on Unix: `-c 16 10.0.0.1:8080`.
  System 6 has no text editor that saves a file. Put it in the floppy image on your computer with
  hfsutils (the Retro68 image has them):

  ```
  printf '127.0.0.1:8097\n' > opts.txt
  docker run --rm --platform linux/amd64 -v "$PWD":/w -w /w ghcr.io/autc04/retro68 sh -c \
    'hmount Archaic.dsk && hcopy -t opts.txt ":Archaic Options" && hattrib -t TEXT -c ttxt ":Archaic Options" && humount'
  ```

- Set the screen to 256 colors in Control Panel > Monitors: Archaic asks WRP for 216 (the web colors).
  In black and white, WRP sends ABM.
- `screenshots/system6.png`: the Mac's screen, 640 x 480 in 256 colors, with Wikipedia's Macintosh IIcx article.

**Measured in Snow** (a IIcx at its real speed: a 16 MHz 68030), from Return until the picture is complete:

| What | Time |
|---|---|
| WRP start page | 3 s |
| Wikipedia's Macintosh IIcx article, black and white (ABM, a slice of 4 KB) | 2 s, WRP's 2 s included |
| the same, 216 colors (AB8, a slice of 12 KB) | 3 s: WRP 2 s, then the Mac 1.2 s for the slice (Archaic 0.11, as a GIF: 2.5 s) |
| Wikipedia's Apple Inc. article, after a click on its link | 14 s: WRP 10 s |

## Building for the classic Mac OS

Archaic for the Mac is built on your computer, with [Retro68](https://github.com/autc04/Retro68)
(gcc 16.1 for the 68k Mac) in Docker:

```
make mac        # or: sh src/mkmac
```

- It needs Docker, and the image `ghcr.io/autc04/retro68` (5.6 GB, amd64: on Apple silicon Docker runs it with Rosetta).
- It makes `src/build-mac/Archaic.dsk` (an 800 KB floppy image) and `Archaic.bin` (MacBinary), about 160 KB.
  The build folder is `src/build-mac`: git ignores it.
- `CMakeLists.txt` names `main.c`'s `main` `archaic_main`: `mac.c` has the Mac's `main()`.
- `archaic.r` is the SIZE resource: 3 MB, at least 1.5 MB, and the flags for multitasking
  (see Multitasking on the classic Mac OS).
- The same files run on System 6.0.8 and Mac OS 7.5.5. Not tested: a Mac with no Color QuickDraw
  (Plus, SE, Classic), MultiFinder on System 6.

## Mac OS 6: what to know

Test on a copy of your Mac's disk.

| Thing | In Snow |
|---|---|
| Computer | Macintosh IIcx: 68030 at 16 MHz, 8 MB |
| Screen | Macintosh Display Card: 640 x 480, up to 256 colors (Monitors) |
| System | System 6.0.8, the Finder (no MultiFinder) |
| Network | MacTCP 2.0.6 on Snow's DaynaPORT. Snow's NAT: the Mac 10.0.0.2 (by RARP), the gateway 10.0.0.1. **Your computer = 127.0.0.1** |

- Retro68: `int`, `long` and pointers are 32 bits; it defines `macintosh`. All Toolbox names are in
  `<Multiverse.h>` (the Multiversal Interfaces). They have MacTCP's TCP, not its name resolver: `dnr.c` has it.
- In the Multiversal Interfaces, `WindowPtr` is a `GrafPtr`: cast it to `WindowPeek` for `updateRgn`.
  There is no `geneva`: its font number is 3. The Control key's flag is `ControlKey`.
- There is no `chmod`, `access` or `getuid`. `update.c` needs them only with a `SYSID`: the Mac build
  has none, so it has no updates.
- `main()` in `mac.c` starts the Toolbox, reads `Archaic Options` (in the current folder: the application's,
  when the Finder starts it), and calls `main.c`'s main.
- Text is Geneva 9. Archaic's text is Latin-1, the Mac's is Mac Roman: `mac.c` changes the letters both ways.
- Control and a letter: `mac.c` makes it the control code. Without that, Ctrl-U did nothing in System 6.0.8.
- MacTCP: each call is `PBControlAsync`, then `SystemTask` while it waits, for 2 ticks at most. After that,
  `mac_wait` (see Multitasking on the classic Mac OS). The parameter blocks are static.
  Archaic releases the stream before it quits (`atexit`): else MacTCP writes into memory that is no longer Archaic's.
- MacTCP picks local ports from a small range (about 1024 to 2047), and it gave a new connection
  the port of the one that had just closed. Snow's NAT then dropped its SYNs for 13 s, and a server
  in TIME_WAIT may refuse it too. So Archaic picks its own ports: 20000 to 39999, the next one each time.
- `n_read` waits at most 2 ticks for data, in `WaitNextEvent`: an event for Archaic ends the wait.
- `g_copy` copies in the window only when no other window covers it (its visRgn).
- `g_pix`: a PixMap with its own color table, with a new seed when the palette changes.
  Color QuickDraw maps the colors to the screen's.
- The scroll bar's values are 16 bits: a long page is counted in steps of more than 1 pixel.
- Snow with its MCP tools: `snowemu --mcp FILE.snoww`. The tools have the file's name as the VM's name.
  - A click right after the start can come while Archaic loads WRP's first page, and is lost: wait for the page.
  - The wait-screen tool can miss a small change (a few letters): look at the screenshot.
  - Shut the Mac down (Special > Shut Down) before you quit Snow.
- Snow's View menu: Display scale 1.0 and Scaling algorithm NearestNeighbor show the Mac's pixels 1:1.
  (1.5 and Linear blur them.)

## Running on Mac OS 7.5

- **Mac OS 7.5.5** on the same Macintosh IIcx in Snow, with Open Transport and the DaynaPORT driver.
- Release files: `archaic-macos7.dsk` (a floppy image) and `archaic-macos7.bin` (MacBinary). They are the same
  files as `archaic-macos6.*`: one build runs on both. It shares the CPU with the other programs
  (see Multitasking on the classic Mac OS).
- Open Archaic in the Finder. Its window fills the screen under the menu bar, less the column of
  disk icons at the right.
- **Cmd-Q** quits. The Finder's Shut Down and Restart quit it too.
- WRP's address: Open Transport has its own `127.0.0.1`, the Mac itself. So in Snow the Mac can't
  reach your computer's `127.0.0.1`. Snow's NAT connects to the address that the Mac asks for, from your computer:
  give WRP's address on another network of your computer (the host-only network of a VM program is good)
  in `Archaic Options`. WRP listens on all addresses by default (`-l :8080`).
- A name for WRP: a text file `Hosts` in the System Folder, with a line like `wrp.test A ADDRESS`.
  Open Transport reads it, also for Archaic (MacTCP's resolver, `dnr.c`).
- SimpleText opens `Archaic Options`, and saves it. In Snow, an edit on a floppy is lost when the Mac ejects
  the floppy (Put Away, Restart, Shut Down): put the file in the floppy image with hfsutils (see Running on Mac OS 6).
- `screenshots/macos75.png`: the Mac's screen, 832 x 624 in 256 colors, with Wikipedia's System 7 article.

**Measured in Snow** (the IIcx at its real speed), 832 x 624 in 256 colors, from Return until the picture is complete:

| What | Time |
|---|---|
| WRP start page | 3 s |
| Wikipedia's System 7 article, 216 colors (GIF, a slice of 735 x 528, 26 KB) | 9 s: WRP about 3 s, then the Mac about 6 s for the slice |

## Mac OS 7.5: what to know

Test on a copy of your Mac's disk.

| Thing | In Snow |
|---|---|
| Computer | Macintosh IIcx: 68030 at 16 MHz, 8 MB |
| Screen | Macintosh Display Card: 832 x 624 with Snow's 16" RGB monitor (`"monitor": "RGB16"` in the `.snoww` file), up to 256 colors |
| System | Mac OS 7.5.5 |
| Network | Open Transport on Snow's DaynaPORT, set by hand in TCP/IP: the Mac 10.0.0.2, mask 255.0.0.0, router 10.0.0.1, a name server |

- The disk: Snow boots a disk with a partition map and a driver, not a bare HFS volume (`.hfv`).
  Put a partition map and a driver in front of it: here the first 96 blocks (48 KB) of a System 6 disk image
  (`.hda`) whose HFS partition (at block 96) has the same size.
- The DaynaPORT driver: its Easy Install stops (`The file "Network" ... cannot be replaced`).
  Custom Install of "DaynaPORT SCSI/Link" alone works. Open Transport then calls it "Alternate Ethernet".
- A new monitor in Snow starts in black and white: set 256 colors in Monitors again.
  Its Options (click with Option down) has the gamma: "Page-White Gamma" shows white as cream,
  "Uncorrected Gamma" as white.
- Snow at uncapped speed: the Mac's clock runs fast too (the status line's times are too long),
  and a double-click can come as two clicks. Use accurate speed for times and double-clicks.

### Multitasking on the classic Mac OS

On System 7 (and with MultiFinder) the programs share the CPU only when each one asks for its next event
(`WaitNextEvent`, `GetNextEvent`, `EventAvail`). `SystemTask` alone gives the other programs no time.

- Archaic waits only in `WaitNextEvent`, with a sleep: `g_event` (30 ticks in front, 60 in the background,
  2 while a part of the scroll bar is held), and `mac_wait` for the network (`nmactcp.c`, `dnr.c`).
  Events for Archaic that come in `mac_wait` go to its queue.
- A picture in colors takes seconds to decode on a 68030: `YIELD` in `img_rows` (`main.c`) calls
  `mac_wait(0)` each 4 ticks (1/15 s). On the other systems `YIELD` is empty.
- `archaic.r`: `acceptSuspendResumeEvents`, `doesActivateOnFGSwitch`, `isHighLevelEventAware`, `canBackground`.
  Suspend and resume: the scroll bar and the grow box go inactive and active.
  The click that brings Archaic to the front does not go to the page.
- Apple events (where `Gestalt` says they are there): Quit quits (the Finder's Shut Down and Restart send it),
  Open Application does nothing, Open and Print Documents are not handled.
- Measured: Note Pad in front, and Archaic behind it loading and decoding Wikipedia's Apple Inc. article
  (216 colors). 150 keys typed 60 ms apart: Note Pad got all 150. The Archaic before this (no yields)
  lost 2 of them: the Mac's event queue was full while it decoded.
