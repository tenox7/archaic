#!/usr/bin/env python3
# ntico.py - draws Archaic's icon for Windows NT (src/archaic.ico): the logo of
# main.c (a white A in a black box) at 32 x 32, in black and white, as NT 3.1
# shows icons. Run: python3 tools/ntico.py src/archaic.ico (no file: only print it).
import struct, sys
N = 32
# '#' black box, '.' white A, ' ' transparent. The A as in main.c's logo:
# a thin left stroke, a thick right one, a bar, and serifs.
g = [['#'] * N for _ in range(N)]
for (y, x) in [(0,0),(0,1),(0,2),(1,0),(2,0),(1,1)]:
    for yy, xx in [(y, x), (y, N-1-x), (N-1-y, x), (N-1-y, N-1-x)]:
        g[yy][xx] = ' '
def w(r, a, b):
    for x in range(a, b + 1): g[r][x] = '.'
rows = {  # row: (left stroke start, right stroke start)
}
w(5, 15, 16); w(6, 15, 16)
spec = [(7, 14, 16), (8, 14, 16), (9, 14, 17), (10, 13, 17), (11, 13, 17), (12, 13, 18),
        (13, 12, 18), (14, 12, 18), (15, 12, 19), (16, 11, 19), (17, 11, 19),
        (20, 10, 20), (21, 10, 21), (22, 9, 21), (23, 9, 21), (24, 9, 22)]
for r, l, rr in spec:
    w(r, l, l + 1); w(r, rr, rr + 2)
w(18, 10, 22); w(19, 10, 22)        # the bar
w(25, 6, 12); w(25, 19, 25)         # the serifs
w(26, 6, 12); w(26, 19, 25)
for row in g: print(''.join(row))
if len(sys.argv) > 1:
    xor = b''; andm = b''
    for row in reversed(g):
        xb = 0; ab = 0
        for x, c in enumerate(row):
            if c == '.': xb |= 1 << (31 - x)
            if c == ' ': ab |= 1 << (31 - x)
        xor += struct.pack('>I', xb); andm += struct.pack('>I', ab)
    bmp = struct.pack('<IiiHHIIiiII', 40, 32, 64, 1, 1, 0, len(xor) + len(andm), 0, 0, 0, 0)
    bmp += bytes([0,0,0,0, 255,255,255,0]) + xor + andm
    ico = struct.pack('<HHH', 0, 1, 1) + struct.pack('<BBBBHHII', 32, 32, 2, 0, 1, 1, len(bmp), 22) + bmp
    open(sys.argv[1], 'wb').write(ico)
