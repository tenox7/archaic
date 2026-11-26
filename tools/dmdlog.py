#!/usr/bin/env python3
"""dmdlog.py LOG [W H [quiet]] - host tool for the DMD 5620 port: decode the
bytes that Archaic (dmd.c) sent to the terminal, print each command, and
draw the screen they make to LOG.pgm (W x H is the layer's size, 660 x 470
if not given). Make the log on the 3B2: ARCHAICLOG=file archaic ...
The letters are drawn with the 6x13 font of src/font6x13.h, in 9 x 14 cells;
the frame of the page's picture is gray: the terminal has its pixels, the
log has only the packed slices (D blocks, not shown)."""
import sys
import re, os

data = open(sys.argv[1], 'rb').read()
W = int(sys.argv[2]) if len(sys.argv) > 2 else 660
H = int(sys.argv[3]) if len(sys.argv) > 3 else 470
quiet = len(sys.argv) > 4
FW, FH = 9, 14
fb = bytearray(b'\xff' * (W * H))		# 255 white, 0 black, 200 the frame
src = os.environ.get('ARCHAIC_SRC', os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'src'))
rows = re.findall(r'\{([^{}]+)\}', open(src + '/font6x13.h').read().split('font6x13[256][13] = {', 1)[1])
font = [[int(v, 16) for v in r.split(',')] for r in rows]
def rect(x0, y0, x1, y1, v):
    x0, y0, x1, y1 = max(x0, 0), max(y0, 0), min(x1, W), min(y1, H)
    for yy in range(y0, y1):
        if v is None:
            for xx in range(x0, x1):
                fb[yy * W + xx] ^= 255
        elif x1 > x0:
            fb[yy * W + x0:yy * W + x1] = bytes([v]) * (x1 - x0)
def px(x, y, v):
    if 0 <= x < W and 0 <= y < H:
        fb[y * W + x] = v
pos = 0
clip = [0, 0, 32000, 32000]
view = [0, 0, 0, 0, 0]

def b():
    global pos
    v = data[pos]; pos += 1
    return v
def g2():
    return b() << 8 | b()
def g2s():
    v = g2()
    return v - 65536 if v & 0x8000 else v
def g4():
    return g2() << 16 | g2()
def isect(x, y, w, h):
    x0, y0 = max(x, clip[0]), max(y, clip[1])
    x1, y1 = min(x + w, clip[2]), min(y + h, clip[3])
    return (x0, y0, x1, y1) if x1 > x0 and y1 > y0 else None
def fill(x, y, w, h, op):
    r = isect(x, y, w, h)
    if not r:
        return 'nothing'
    x0, y0, x1, y1 = r
    rect(x0, y0, x1, y1, None if op == 2 else 0 if op else 255)
    return '-> %d,%d %dx%d' % (x0, y0, x1 - x0, y1 - y0)
def say(*a):
    if not quiet:
        print(*a)

nbytes = {}
while pos < len(data):
    at = pos
    c = chr(b())
    if c == 'Z':
        rect(0, 0, W, H, 255); clip = [0, 0, 32000, 32000]
        say(at, 'Z clear')
    elif c == 'K':
        x, y, w, h = g2(), g2(), g2(), g2()
        clip = [x, y, x + w, y + h]
        say(at, 'K clip %d,%d %dx%d' % (x, y, w, h))
    elif c == 'f':
        col = b()
        say(at, ' f fill clip %s' % ('black' if col else 'white'), fill(clip[0], clip[1], clip[2] - clip[0], clip[3] - clip[1], col))
    elif c == 'i':
        say(at, ' i invert clip', fill(clip[0], clip[1], clip[2] - clip[0], clip[3] - clip[1], 2))
    elif c == 'F':
        x, y, w, h, col = g2(), g2(), g2(), g2(), b()
        say(at, ' F %d,%d %dx%d %s' % (x, y, w, h, 'black' if col else 'white'), fill(x, y, w, h, col))
    elif c == 'I':
        x, y, w, h = g2(), g2(), g2(), g2()
        say(at, ' I %d,%d %dx%d' % (x, y, w, h), fill(x, y, w, h, 2))
    elif c == 'T':
        x, y, inv, n = g2s(), g2s(), b(), b()
        s = data[pos:pos + n].decode('latin-1'); pos += n
        r = isect(x, y, n * FW, FH)
        if r:
            x0, y0, x1, y1 = r
            for yy in range(y0, y1):
                for xx in range(x0, x1):
                    i, cx, cy = (xx - x) // FW, (xx - x) % FW - 1, yy - y
                    on = 0 <= cx < 6 and cy < 13 and font[ord(s[i])][cy] >> (7 - cx) & 1
                    px(xx, yy, 0 if bool(on) != bool(inv) else 255)
        say(at, ' T %d,%d %s%r' % (x, y, 'inv ' if inv else '', s), r)
    elif c in 'BP':
        x, y, w, h = g2s(), g2s(), g2(), g2()
        bpl = (w + 7) // 8
        if c == 'P':                    # a pattern: its first rows, the others repeat them
            per = b()
            first = data[pos:pos + bpl * per]; pos += bpl * per
            rows = b''.join(first[(i % per) * bpl:(i % per + 1) * bpl] for i in range(h))
        else:
            rows = data[pos:pos + bpl * h]; pos += bpl * h
        r = isect(x, y, w, h)
        if r:
            for yy in range(r[1], min(r[3], H)):
                for xx in range(r[0], min(r[2], W)):
                    bit = rows[(yy - y) * bpl + ((xx - x) >> 3)] >> (7 - ((xx - x) & 7)) & 1
                    px(xx, yy, 0 if bit else 255)
        say(at, ' %s %d,%d %dx%d (%d bytes)' % (c, x, y, w, h, pos - at), r)
    elif c == 'R':
        r = isect(view[0], view[1], view[2], view[3])
        if r:
            rect(r[0], r[1], r[2], r[3], 200)
        say(at, ' R frame', r)
    elif c == 'C':
        x, y, w, h, dy = g2(), g2(), g2(), g2(), g2s()
        part = [bytes(fb[(y + i) * W + x:(y + i) * W + x + w]) for i in range(h)]
        for i in range(h):
            if 0 <= y + dy + i < H:
                fb[(y + dy + i) * W + x:(y + dy + i) * W + x + w] = part[i]
        say(at, 'C copy %d,%d %dx%d dy %d' % (x, y, w, h, dy))
    elif c == 'V':
        x, y, w, h, oy = g2s(), g2s(), g2(), g2(), g4()
        view = [x, y, w, h, oy]
        say(at, 'V frame %d,%d %dx%d row %d' % (x, y, w, h, oy))
    elif c == 'H':
        x, y, w, h = g2(), g2(), g2(), g2()
        say(at, 'H hole %d,%d %dx%d' % (x, y, w, h))
    elif c == 'N':
        s, row = b(), g4()
        say(at, 'N slice for slot %d, page row %d' % (s, row))
    elif c == 'D':
        n = b(); pos += n
        nbytes['D'] = nbytes.get('D', 0) + n + 2
        continue
    elif c == 'E':
        say(at, 'E end ok=%d' % b())
    elif c == 'X':
        say(at, 'X drop slot %d' % b())
    elif c == 'W':
        on, x, y = b(), g2(), g2()
        say(at, 'W spinner %d at %d,%d' % (on, x, y))
    elif c == 'G':
        x, y, w, h, n = g2(), g2(), g2(), g2(), g4()
        say(at, 'G bar %d,%d %dx%d for %d bytes' % (x, y, w, h, n))
    elif c == 'Q':
        say(at, 'Q quit')
    else:
        print(at, 'unknown byte', repr(c)); break
    nbytes[c] = nbytes.get(c, 0) + pos - at
open(sys.argv[1] + '.pgm', 'wb').write(b'P5 %d %d 255\n' % (W, H) + bytes(fb))
print('bytes:', len(data), ' '.join('%s=%d' % kv for kv in sorted(nbytes.items())))
