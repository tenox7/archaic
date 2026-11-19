#!/usr/bin/env python3
# pcf2c.py FONT.pcf.gz NAME > NAME.h - a fixed-width X11 PCF font as a C
# table: NAME[256][H], one byte per row, the high bit is the leftmost pixel.
import gzip, struct, sys

PCF_ACCEL, PCF_METRICS, PCF_BITMAPS, PCF_ENCODINGS, PCF_BDF_ACCEL = 2, 4, 8, 32, 256
PCF_COMPRESSED_METRICS = 0x100

data = gzip.open(sys.argv[1]).read() if sys.argv[1].endswith('.gz') else open(sys.argv[1], 'rb').read()
name = sys.argv[2]
assert data[:4] == b'\x01fcp'
ntab = struct.unpack('<i', data[4:8])[0]
tabs = {}
for i in range(ntab):
    t, fmt, size, off = struct.unpack('<iiii', data[8 + 16 * i:24 + 16 * i])
    tabs[t] = (fmt, off)

def table(t):
    fmt, off = tabs[t]
    f = struct.unpack('<i', data[off:off + 4])[0]
    e = '>' if f & 4 else '<'
    return f, e, off + 4

# accelerators: the font's ascent and descent
f, e, p = table(PCF_BDF_ACCEL if PCF_BDF_ACCEL in tabs else PCF_ACCEL)
ascent, descent = struct.unpack(e + 'ii', data[p + 8:p + 16])

# metrics
f, e, p = table(PCF_METRICS)
mets = []
if f & PCF_COMPRESSED_METRICS:
    n = struct.unpack(e + 'h', data[p:p + 2])[0]
    p += 2
    for i in range(n):
        lsb, rsb, wid, asc, desc = [b - 0x80 for b in data[p:p + 5]]
        mets.append((lsb, rsb, wid, asc, desc))
        p += 5
else:
    n = struct.unpack(e + 'i', data[p:p + 4])[0]
    p += 4
    for i in range(n):
        lsb, rsb, wid, asc, desc, attr = struct.unpack(e + 'hhhhhH', data[p:p + 12])
        mets.append((lsb, rsb, wid, asc, desc))
        p += 12

# bitmaps
f, e, p = table(PCF_BITMAPS)
msbbit = bool(f & 8)
pad = 1 << (f & 3)
n = struct.unpack(e + 'i', data[p:p + 4])[0]
offs = struct.unpack(e + '%di' % n, data[p + 4:p + 4 + 4 * n])
bits = p + 4 + 4 * n + 16

# encodings
f, e, p = table(PCF_ENCODINGS)
min2, max2, minb1, maxb1, defc = struct.unpack(e + 'hhhhh', data[p:p + 10])
cols = max2 - min2 + 1
idx = struct.unpack(e + '%dH' % (cols * (maxb1 - minb1 + 1)), data[p + 10:p + 10 + 2 * cols * (maxb1 - minb1 + 1)])

W = mets[idx[ord('M') - min2]][2]
H = ascent + descent
for m in mets:
    assert m[2] == W, 'not a fixed-width font'
assert W <= 8

def glyph(c):
    rows = [0] * H
    if c < min2 or c > max2 or minb1 != 0:
        return rows
    g = idx[c - min2]
    if g == 0xffff:
        return rows
    lsb, rsb, wid, asc, desc = mets[g]
    gw = rsb - lsb
    bpr = ((gw + 7) // 8 + pad - 1) // pad * pad
    o = bits + offs[g]
    for r in range(asc + desc):
        y = ascent - asc + r
        for x in range(gw):
            byte = data[o + r * bpr + x // 8]
            bit = (byte >> (7 - x % 8)) & 1 if msbbit else (byte >> (x % 8)) & 1
            px = lsb + x
            if bit and 0 <= y < H and 0 <= px < W:
                rows[y] |= 0x80 >> px
    return rows

print('/* %s.h - the X11 misc-fixed %dx%d font (ISO 8859-1), which is in the' % (name, W, H))
print(' * public domain. %s[c][row]: one byte per row, the high bit is the' % name)
print(' * leftmost pixel. Made from %s by pcf2c.py. */' % sys.argv[1].split('/')[-1])
print('#define FONTW %d' % W)
print('#define FONTH %d' % H)
print('#define FONTA %d\t\t/* ascent: the baseline is row FONTA */' % ascent)
print('static unsigned char %s[256][%d] = {' % (name, H))
for c in range(256):
    rows = glyph(c)
    ch = chr(c) if 32 < c < 127 and c not in (92,) else ''
    print('\t{ %s },%s' % (', '.join('0x%02x' % r for r in rows), '\t/* %s */' % ch if ch and ch != '*' and ch != '/' else ''))
print('};')
