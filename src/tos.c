/* tos.c - Archaic's display for GEM on the Atari with TOS: a GEM window
 * with GEM's own scroll bar, on the screens of the ST, STE, TT and
 * Falcon, and of graphics cards (NVDI). Built with nsting.c (STinG's
 * TCP: a NetUSBee or an EtherNEC card) by mktos. main() here reads the
 * options, then calls main.c's main (archaic_main).
 *
 * The screen: v_opnvwk gives its size, vq_extnd its planes and whether
 * it has a palette, vq_scrninfo (with the cookie EdDI: NVDI, fVDI) how
 * its pixels are made. Archaic keeps the window's pixels in memory in
 * the screen's own format, and draws them itself. A redraw then only
 * copies them (vrt_cpyfm, vro_cpyfm), and an 8 MHz ST stays fast:
 *
 * - B_MONO: 1 bit a pixel, 1 black. A screen of 1 plane (ST high, TT
 *   high), the 4 colors of ST medium, and any screen Archaic does not
 *   know. vrt_cpyfm copies it with pens 1 (black) and 0 (white).
 *   g_colors() is 2.
 * - B_PLANES: the Atari's interleaved bit planes, 4 or 8 (ST low, the
 *   TT's and the Falcon's 16 and 256 colors): 16 pixels in n words, a
 *   word for each plane. g_colors() is 16 or 256.
 * - B_PACKED: a pixel in 1 to 4 bytes: a graphics card, the Falcon's
 *   true color (RGB 565 when there is no vq_scrninfo), ARAnyM's fVDI.
 *   256 colors with a palette, else millions.
 *
 * With a palette, the pictures' colors take pens of their own: 16 to
 * 255 (0 to 15 stay GEM's), with 16 colors 2 to 15. A pen is set
 * (vs_color) for each color a picture uses, the most used first. When
 * no pen is free, the pens no pixel in the window shows are free again;
 * then the nearest color. At the end the palette is set back. VDI's pen
 * numbers are not the screen's pixel values: v_get_pixel says them.
 *
 * Keys: the up and down arrows scroll a quarter, with Shift a window;
 * ClrHome goes to the top, Shift ClrHome to the end; Undo is Esc. The
 * Atari's characters go to Archaic as ISO 8859-1. Ctrl+Q, the close box
 * and AP_TERM quit. GEM has no file to wait on: g_fd() is -1, and
 * nsting.c waits 1/20 s at most.
 */
#include "archaic.h"
#include "font6x13.h"
#include <gem.h>
#include <gemx.h>
#include <osbind.h>
#include <stdlib.h>
#include <string.h>

#define KIND (NAME | CLOSER | FULLER | MOVER | SIZER | UPARROW | DNARROW | VSLIDE)
#define BIG 2000000000L		/* S_TO past the end */
#define NQ 8			/* events from one evnt_multi */
#define MINW 200		/* the smallest work area */
#define MINH 100

#define B_MONO 0
#define B_PLANES 1
#define B_PACKED 2

int g_bar = 0;

static short ap = -1;		/* the AES's id for Archaic */
static short vh;		/* the VDI workstation */
static short wh = -1;		/* the window */
static int sw, sh;		/* the screen's size */
static short wx, wy, wkw, wkh;	/* the work area, on the screen */

static int kind;		/* B_MONO, B_PLANES, B_PACKED */
static int planes;		/* the screen's planes (bits a pixel) */
static int bpp, swap;		/* B_PACKED: bytes a pixel, Intel's order */
static int clut;		/* the screen has a palette */
static long ncolors;		/* g_colors() */
static unsigned long white, black;	/* their pixel values */
static unsigned long rt[256], gt[256], bt[256];	/* true color: the bits */

static unsigned char *buf;	/* the window's pixels */
static int bw, bh, tw;		/* its size; tw: bw up to a multiple of 16 */
static long bpl;		/* bytes a row */
static MFDB bm, scr;		/* buf, and the screen (fd_addr 0) */
static int dx0, dy0, dx1, dy1;	/* changed since g_flush: x0 <= x < x1 */

/* the palette */
static int npens;		/* 16 or 256; 0: no palette of Archaic's */
static int pen0, pen1;		/* Archaic's pens: pen0 up to pen1 - 1 */
static unsigned long pixof[256];	/* the pixel value of each pen */
static int penof[256];		/* the pen of each pixel value */
static short orig[768];		/* the palette at the start (0 to 1000) */
static unsigned char prgb[768];	/* the pens' colors as they show */
static unsigned char want[768];	/* what Archaic asked for */
static char taken[256];		/* Archaic set the pen */
static int setpal;		/* Archaic changed the palette */
static unsigned long map[256];	/* g_pix: the picture's colors, as pixels */
static unsigned long s4[16];	/* 4 bits, a byte each: for the planes */

static char wtitle[100];	/* the AES keeps a pointer to it */
static long smax = -1;		/* g_sbar's max */
static int lsize = -1, lpos = -1;	/* the slider as it was set */
static Event q[NQ];		/* events not given yet */
static int nq, iq;
static int held;		/* a mouse button is down */
static int down;		/* ... in the window: 1 left, 3 right */
static short mx, my;		/* the mouse, on the screen */
static int szpend;		/* sized while a button was down: sized() at the release */
static long jar;		/* the cookie jar */
static long hz;			/* _hz_200 */

/* the Atari's characters 0200 to 0377 in ISO 8859-1; 0: none there */
static unsigned char latin[128] = {
	0307, 0374, 0351, 0342, 0344, 0340, 0345, 0347,	/* 0200 */
	0352, 0353, 0350, 0357, 0356, 0354, 0304, 0305,
	0311, 0346, 0306, 0364, 0366, 0362, 0373, 0371,	/* 0220 */
	0377, 0326, 0334, 0242, 0243, 0245, 0337, 0,
	0341, 0355, 0363, 0372, 0361, 0321, 0252, 0272,	/* 0240 */
	0277, 0, 0254, 0275, 0274, 0241, 0253, 0273,
	0343, 0365, 0330, 0370, 0, 0, 0300, 0303,	/* 0260 */
	0325, 0250, 0264, 0, 0266, 0251, 0256, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0300 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0247, 0, 0,	/* 0320 */
	0, 0, 0, 0, 0, 0, 0265, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0340 */
	0, 0261, 0, 0, 0, 0, 0367, 0, 0260, 0, 0267, 0, 0, 0262, 0263, 0257	/* 0360 */
};

static long getjar()
{
	jar = *(long *)0x5a0L;
	return 0;
}

static long gethz()
{
	hz = *(volatile long *)0x4baL;
	return 0;
}

/* the cookie's value in *v: 1, or 0 if there is none */
static int cookie(id, v)
long id, *v;
{
	long *p;

	Supexec(getjar);
	for (p = (long *)jar; p && p[0]; p += 2)
		if (p[0] == id) {
			*v = p[1];
			return 1;
		}
	return 0;
}

/* a channel's pixel bits for each value 0 to 255: n bits, bit i of
 * the n (the low first) at pixel bit pos[i] */
static void chan(t, n, pos)
unsigned long *t;
int n;
short *pos;
{
	int v, i, k;

	for (v = 0; v < 256; ++v) {
		t[v] = 0;
		for (i = 0; i < n; ++i) {
			k = 8 - n + i;
			if (k >= 0 && (v >> k & 1) && pos[i] >= 0 && pos[i] < 32)
				t[v] |= (unsigned long)1 << pos[i];
		}
	}
}

/* the screen's pixel value of each pen: draw it at x, y (in the
 * window, which is drawn after) and ask v_get_pixel */
static void probe(x, y)
int x, y;
{
	static short pxy[4], pel, idx;
	int i;

	wind_update(BEG_UPDATE);
	graf_mouse(M_OFF, (MFORM *)0);
	vs_clip(vh, 0, pxy);
	vswr_mode(vh, MD_REPLACE);
	vsf_interior(vh, FIS_SOLID);
	vsf_perimeter(vh, 0);
	pxy[0] = pxy[2] = x;
	pxy[1] = pxy[3] = y;
	for (i = 0; i < 256; ++i)
		penof[i] = 1;
	for (i = 0; i < npens; ++i) {
		vsf_color(vh, i);
		vr_recfl(vh, pxy);
		v_get_pixel(vh, x, y, &pel, &idx);
		pixof[i] = (unsigned long)pel & 0xff;
	}
	for (i = npens - 1; i >= 0; --i)
		penof[pixof[i]] = i;
	graf_mouse(M_ON, (MFORM *)0);
	wind_update(END_UPDATE);
}

/* the colors of the pens, as they show */
static void readpal()
{
	static short c[3];
	int i, k;

	for (i = 0; i < npens; ++i) {
		vq_color(vh, i, 0, orig + 3 * i);
		vq_color(vh, i, 1, c);
		for (k = 0; k < 3; ++k)
			prgb[3 * i + k] = (unsigned char)((long)c[k] * 255 / 1000);
	}
}

/* what kind of screen: kind, planes, bpp, swap, clut, ncolors, and the
 * channels' bits (true color) */
static void screen()
{
	static short ex[57], si[273], pos[16];
	long v, vdo = 0;
	int i, edd, fmt;

	vq_extnd(vh, 1, ex);
	planes = ex[4];
	clut = ex[5] != 0;
	kind = B_MONO;
	ncolors = 2;
	for (i = 0; i < 273; ++i)
		si[i] = 0;
	edd = cookie(0x45644449L, &v);	/* 'EdDI' */
	if (edd)
		vq_scrninfo(vh, si);
	cookie(0x5f56444fL, &vdo);	/* '_VDO': 3 the Falcon */
	if (planes >= 4 && planes <= 8 && clut) {
		fmt = edd && si[0] >= 0 && si[0] <= 2 ? si[0] : 0;
		if (fmt == 0 && (planes == 4 || planes == 8))
			kind = B_PLANES;	/* the Atari's own */
		else if (fmt == 2 && planes == 8) {
			kind = B_PACKED;	/* a card: a byte a pixel */
			bpp = 1;
		} else
			return;
		npens = 1 << planes;
		ncolors = npens;
		pen0 = npens == 16 ? 2 : 16;
		pen1 = npens;
		return;
	}
	if (planes != 15 && planes != 16 && planes != 24 && planes != 32)
		return;
	if (edd && si[0] == 2 && si[1] == 2 && si[8] > 0 && si[9] > 0 && si[10] > 0) {
		chan(rt, si[8], si + 16);
		chan(gt, si[9], si + 32);
		chan(bt, si[10], si + 48);
		swap = (si[14] & 0x80) != 0;	/* Intel's byte order */
	} else if (planes == 16 && (vdo >> 16) == 3) {
		for (i = 0; i < 5; ++i)	/* the Falcon: RRRRRGGGGGGBBBBB */
			pos[i] = 11 + i;
		chan(rt, 5, pos);
		for (i = 0; i < 6; ++i)
			pos[i] = 5 + i;
		chan(gt, 6, pos);
		for (i = 0; i < 5; ++i)
			pos[i] = i;
		chan(bt, 5, pos);
		swap = 0;
	} else
		return;
	kind = B_PACKED;
	bpp = planes == 15 ? 2 : planes / 8;
	ncolors = planes >= 24 ? 1L << 24 : 1L << planes;
	clut = 0;
}

/* a new buf for the window, all white: 0, -1 no memory */
static int newbuf(w, h)
int w, h;
{
	unsigned char *p;
	long n, i;

	if (w < 1)
		w = 1;
	if (h < 1)
		h = 1;
	tw = (w + 15) & ~15;
	if (kind == B_MONO)
		bpl = tw / 8;
	else if (kind == B_PLANES)
		bpl = (long)tw / 8 * planes;
	else
		bpl = (long)tw * bpp;
	n = bpl * h;
	p = (unsigned char *)malloc((unsigned long)n);
	if (!p)
		return -1;
	if (buf)
		free((char *)buf);
	buf = p;
	bw = w;
	bh = h;
	bm.fd_addr = (void *)buf;
	bm.fd_w = tw;
	bm.fd_h = h;
	bm.fd_wdwidth = tw / 16;
	bm.fd_stand = 0;
	bm.fd_nplanes = kind == B_MONO ? 1 : planes;
	bm.fd_r1 = bm.fd_r2 = bm.fd_r3 = 0;
	if (kind == B_MONO)
		for (i = 0; i < n; ++i)
			buf[i] = 0;
	else if (kind == B_PLANES)
		for (i = 0; i < n; i += 2) {
			buf[i] = buf[i + 1] = 0;
			if (white >> ((i >> 1) % planes) & 1)
				buf[i] = buf[i + 1] = 0xff;
		}
	else if (white == 0xffffffffL || (bpp == 1 && white == 0xff))
		memset((char *)buf, 0xff, (unsigned long)n);
	else
		g_fill(0, 0, w, h, 0);
	dx0 = dy0 = 0;
	dx1 = w;
	dy1 = h;
	return 0;
}

/* cut x, y, w, h to buf and mark it changed; 0 if nothing is left */
static int clip(x, y, w, h)
int *x, *y, *w, *h;
{
	if (*x < 0) {
		*w += *x;
		*x = 0;
	}
	if (*y < 0) {
		*h += *y;
		*y = 0;
	}
	if (*x + *w > bw)
		*w = bw - *x;
	if (*y + *h > bh)
		*h = bh - *y;
	if (*w <= 0 || *h <= 0)
		return 0;
	if (dx0 >= dx1) {
		dx0 = *x;
		dy0 = *y;
		dx1 = *x + *w;
		dy1 = *y + *h;
	} else {
		if (*x < dx0)
			dx0 = *x;
		if (*y < dy0)
			dy0 = *y;
		if (*x + *w > dx1)
			dx1 = *x + *w;
		if (*y + *h > dy1)
			dy1 = *y + *h;
	}
	return 1;
}

/* 16 bits of the 1-bit row s from bit b (b may be < 0: 0 bits there);
 * the row has n bytes */
static unsigned int get16(s, b, n)
unsigned char *s;
long b, n;
{
	unsigned long v = 0;
	long i, k;
	int sh;

	if (b < 0) {
		if (b <= -16)
			return 0;
		return get16(s, 0L, n) >> (int)-b;
	}
	k = b >> 3;
	sh = (int)(b & 7);
	for (i = 0; i < 3; ++i)
		v = v << 8 | (k + i < n ? s[k + i] : 0);
	return (unsigned int)(v << sh >> 8 & 0xffff);
}

/* the 16-bit word at p (big-endian) */
#define W16(p) ((unsigned int)(p)[0] << 8 | (p)[1])
#define PUT16(p, v) ((p)[0] = (unsigned char)((v) >> 8), (p)[1] = (unsigned char)(v))

/* B_PACKED: pixel value v at p */
static void put(p, v)
unsigned char *p;
unsigned long v;
{
	int n;

	if (swap)
		for (n = 0; n < bpp; ++n, v >>= 8)
			*p++ = (unsigned char)v;
	else
		for (n = bpp - 1; n >= 0; --n)
			*p++ = (unsigned char)(v >> (n * 8));
}

static unsigned long get(p)
unsigned char *p;
{
	unsigned long v = 0;
	int n;

	if (swap)
		for (n = bpp - 1; n >= 0; --n)
			v = v << 8 | p[n];
	else
		for (n = 0; n < bpp; ++n)
			v = v << 8 | p[n];
	return v;
}

/* row y, pixels x up to x1 - 1: op 0 set to v, 1 swap white and black */
static void span(y, x, x1, v, op)
int y, x, x1, op;
unsigned long v;
{
	unsigned char *r = buf + (long)y * bpl, *p;
	unsigned int m, w;
	int g, b, i, a, e;

	if (kind == B_MONO) {
		for (i = x; i < x1 && (i & 7); ++i) {
			m = 0x80 >> (i & 7);
			if (op)
				r[i >> 3] ^= m;
			else if (v)
				r[i >> 3] |= m;
			else
				r[i >> 3] &= ~m;
		}
		for (; i + 8 <= x1; i += 8)
			r[i >> 3] = op ? r[i >> 3] ^ 0xff : v ? 0xff : 0;
		for (; i < x1; ++i) {
			m = 0x80 >> (i & 7);
			if (op)
				r[i >> 3] ^= m;
			else if (v)
				r[i >> 3] |= m;
			else
				r[i >> 3] &= ~m;
		}
		return;
	}
	if (kind == B_PLANES) {
		for (g = x >> 4; g <= (x1 - 1) >> 4; ++g) {
			a = g * 16 < x ? x - g * 16 : 0;
			e = g * 16 + 16 > x1 ? x1 - g * 16 : 16;
			m = (0xffffU >> a) & (0xffffU << (16 - e)) & 0xffff;
			p = r + (long)g * 2 * planes;
			for (b = 0; b < planes; ++b, p += 2) {
				w = W16(p);
				if (op)	/* white and black: all bits */
					w ^= m;
				else if (v >> b & 1)
					w |= m;
				else
					w &= ~m;
				PUT16(p, w);
			}
		}
		return;
	}
	p = r + (long)x * bpp;
	for (i = x; i < x1; ++i, p += bpp)
		if (!op)
			put(p, v);
		else if (get(p) == white)
			put(p, black);
		else if (get(p) == black)
			put(p, white);
		else
			put(p, get(p) ^ white);
}

/* w x h pixels at x, y (clipped) from 1-bit rows s (sbpl bytes a row),
 * the first from bit sx: 1 becomes fg, 0 bg */
static void mono(x, y, w, h, s, sbpl, sx, fg, bg)
int x, y, w, h, sx;
long sbpl;
unsigned char *s;
unsigned long fg, bg;
{
	unsigned char *r, *p, *q;
	unsigned int m, v, nv, sv;
	long n = sbpl;
	int i, j, g, b, a, e;

	for (j = 0; j < h; ++j, s += sbpl) {
		r = buf + (long)(y + j) * bpl;
		if (kind == B_MONO) {
			if (fg == bg) {
				span(y + j, x, x + w, fg, 0);
				continue;
			}
			if (!((x | sx) & 7) && fg) {	/* bytes as they are */
				p = r + (x >> 3);
				q = s + (sx >> 3);
				for (i = 0; i + 8 <= w; i += 8)
					*p++ = *q++;
			} else
				i = 0;
			for (; i < w; i += 16 - ((x + i) & 15)) {
				g = (x + i) >> 4;
				a = (x + i) & 15;
				e = w - i + a < 16 ? w - i + a : 16;
				m = (0xffffU >> a) & (0xffffU << (16 - e)) & 0xffff;
				v = get16(s, (long)sx + i - a, n);
				if (!fg)
					v = ~v;
				p = r + g * 2;
				PUT16(p, (W16(p) & ~m) | (v & m));
			}
			continue;
		}
		if (kind == B_PLANES) {
			for (i = 0; i < w; i += 16 - ((x + i) & 15)) {
				g = (x + i) >> 4;
				a = (x + i) & 15;
				e = w - i + a < 16 ? w - i + a : 16;
				m = (0xffffU >> a) & (0xffffU << (16 - e)) & 0xffff;
				sv = get16(s, (long)sx + i - a, n);
				p = r + (long)g * 2 * planes;
				for (b = 0; b < planes; ++b, p += 2) {
					nv = (fg >> b & 1 ? sv : 0) | (bg >> b & 1 ? ~sv : 0);
					PUT16(p, (W16(p) & ~m) | (nv & m));
				}
			}
			continue;
		}
		p = r + (long)x * bpp;
		for (i = 0; i < w; ++i, p += bpp)
			put(p, s[(sx + i) >> 3] & (0x80 >> ((sx + i) & 7)) ? fg : bg);
	}
}

void g_fill(x, y, w, h, c)
int x, y, w, h, c;
{
	unsigned long v = c ? black : white;
	int j;

	if (!clip(&x, &y, &w, &h))
		return;
	for (j = 0; j < h; ++j)
		span(y + j, x, x + w, v, 0);
}

void g_invert(x, y, w, h)
int x, y, w, h;
{
	int j;

	if (!clip(&x, &y, &w, &h))
		return;
	for (j = 0; j < h; ++j)
		span(y + j, x, x + w, 0L, 1);
}

/* x, y, w, h moved dy rows down (up if dy < 0). Whole rows only; else 0,
 * and Archaic draws it all */
int g_copy(x, y, w, h, dy)
int x, y, w, h, dy;
{
	int a = y, b = y + h, j, cx = 0, cy, cw, ch;

	if (x > 0 || x + w < bw)
		return 0;
	if (a < 0)
		a = 0;
	if (a + dy < 0)
		a = -dy;
	if (b > bh)
		b = bh;
	if (b + dy > bh)
		b = bh - dy;
	if (dy == 0 || b <= a)
		return 1;
	cy = a + dy;
	cw = bw;
	ch = b - a;
	clip(&cx, &cy, &cw, &ch);	/* where they go: changed */
	if (dy < 0)
		memmove((char *)(buf + (long)(a + dy) * bpl), (char *)(buf + (long)a * bpl),
			(unsigned long)(b - a) * bpl);
	else
		for (j = b - 1; j >= a; --j)	/* down: the last row first */
			memcpy((char *)(buf + (long)(j + dy) * bpl),
				(char *)(buf + (long)j * bpl), (unsigned long)bpl);
	return 1;
}

/* each character fills its cell: the text and its background */
void g_text(x, y, s, n, inv)
int x, y, n, inv;
char *s;
{
	static unsigned char cell[FONTH][MAXW / 8 + 4];
	unsigned char *g, *p;
	unsigned int v;
	int k, r, b, x0 = x, y0 = y, w, h = FONTH;
	long nb;

	if (n > (MAXW - 16) / FONTW)
		n = (MAXW - 16) / FONTW;
	w = n * FONTW;
	if (n <= 0 || !clip(&x0, &y0, &w, &h))
		return;
	nb = (n * FONTW + 7) / 8;
	for (r = 0; r < FONTH; ++r)
		for (k = 0; k < nb + 1; ++k)
			cell[r][k] = 0;
	for (k = 0; k < n; ++k) {
		g = font6x13[s[k] & 0377];
		b = k * FONTW;
		for (r = 0; r < FONTH; ++r) {
			v = (unsigned int)(g[r] & (0xff << (8 - FONTW) & 0xff)) << 8 >> (b & 7);
			p = &cell[r][b >> 3];
			p[0] |= (unsigned char)(v >> 8);
			p[1] |= (unsigned char)v;
		}
	}
	mono(x0, y0, w, h, &cell[y0 - y][0], (long)sizeof cell[0], x0 - x,
		inv ? white : black, inv ? black : white);
}

int g_textw(s, n)
char *s;
int n;
{
	return n * FONTW;
}

void g_font(height, base)
int *height, *base;
{
	*height = FONTH;
	*base = FONTA;
}

/* rows of 1-bit pixels, 1 black, high bit left */
void g_bits(x, y, w, h, bits, sbpl)
int x, y, w, h, sbpl;
unsigned char *bits;
{
	int x0 = x, y0 = y, cw = w, ch = h;

	if (!clip(&x0, &y0, &cw, &ch))
		return;
	mono(x0, y0, cw, ch, bits + (long)(y0 - y) * sbpl, (long)sbpl, x0 - x,
		black, white);
}

/* the pen nearest to r, g, b */
static int nearest(r, g, b)
int r, g, b;
{
	long d, best = -1;
	int i, p = 1, dr, dg, db;

	for (i = 0; i < npens; ++i) {
		dr = prgb[3 * i] - r;
		dg = prgb[3 * i + 1] - g;
		db = prgb[3 * i + 2] - b;
		d = (long)dr * dr + (long)dg * dg + (long)db * db;
		if (best < 0 || d < best) {
			best = d;
			p = i;
		}
	}
	return p;
}

/* Archaic's pens that no pixel in buf shows, and that keep[] does not
 * hold, are free again */
static void unused(keep)
char *keep;
{
	static char seen[256];
	unsigned char *r, *p;
	unsigned int w[8];
	long y;
	int i, g, b, k, v;

	for (i = 0; i < 256; ++i)
		seen[i] = 0;
	for (y = 0; y < bh; ++y) {
		r = buf + y * bpl;
		if (kind == B_PACKED) {
			for (i = 0; i < bw; ++i)
				seen[r[i]] = 1;
			continue;
		}
		for (g = 0; g < tw / 16; ++g) {
			p = r + (long)g * 2 * planes;
			for (b = 0; b < planes; ++b)
				w[b] = W16(p + 2 * b);
			for (k = 15; k >= 0; --k) {
				for (v = 0, b = planes - 1; b >= 0; --b)
					v = v << 1 | (w[b] >> k & 1);
				seen[v] = 1;
			}
		}
	}
	for (i = pen0; i < pen1; ++i)
		if (taken[i] && !seen[pixof[i]] && !keep[i])
			taken[i] = 0;
}

/* map: the pixel for each color of pal that the picture uses. With a
 * palette, a pen for each, the most used first */
static void getpens(pix, w, h, sbpl, pal)
unsigned char *pix, *pal;
int w, h;
long sbpl;
{
	static long cnt[256];
	static char keep[256];
	static short c[3];
	unsigned char *s;
	long best;
	int i, j, k, p, r, g, b, gc = 0;

	if (kind == B_MONO) {
		for (i = 0; i < 256; ++i)
			map[i] = pal[3 * i] * 30 + pal[3 * i + 1] * 59 +
				pal[3 * i + 2] * 11 < 12800 ? 1 : 0;
		return;
	}
	if (!npens) {
		for (i = 0; i < 256; ++i)
			map[i] = rt[pal[3 * i]] | gt[pal[3 * i + 1]] | bt[pal[3 * i + 2]];
		return;
	}
	for (i = 0; i < 256; ++i) {
		cnt[i] = 0;
		keep[i] = 0;
	}
	for (j = 0; j < h; ++j)
		for (s = pix + j * sbpl, i = 0; i < w; ++i)
			++cnt[s[i]];
	for (;;) {
		for (best = 0, k = -1, i = 0; i < 256; ++i)	/* the most used */
			if (cnt[i] > best) {
				best = cnt[i];
				k = i;
			}
		if (k < 0)
			break;
		cnt[k] = 0;
		r = pal[3 * k];
		g = pal[3 * k + 1];
		b = pal[3 * k + 2];
		for (p = pen0; p < pen1; ++p)	/* asked for already */
			if (taken[p] && want[3 * p] == r && want[3 * p + 1] == g &&
				want[3 * p + 2] == b)
				break;
		if (p == pen1) {	/* a free pen */
			for (p = pen0; p < pen1 && taken[p]; ++p)
				;
			if (p == pen1 && !gc) {
				gc = 1;
				unused(keep);
				for (p = pen0; p < pen1 && taken[p]; ++p)
					;
			}
			if (p < pen1) {
				c[0] = (short)(r * 1000L / 255);
				c[1] = (short)(g * 1000L / 255);
				c[2] = (short)(b * 1000L / 255);
				vs_color(vh, p, c);
				vq_color(vh, p, 1, c);
				prgb[3 * p] = (unsigned char)((long)c[0] * 255 / 1000);
				prgb[3 * p + 1] = (unsigned char)((long)c[1] * 255 / 1000);
				prgb[3 * p + 2] = (unsigned char)((long)c[2] * 255 / 1000);
				want[3 * p] = r;
				want[3 * p + 1] = g;
				want[3 * p + 2] = b;
				taken[p] = 1;
				setpal = 1;
			} else
				p = nearest(r, g, b);
		}
		keep[p] = 1;
		map[k] = pixof[p];
	}
}

/* rows of 8-bit color numbers; pal: the red, green, blue of each */
void g_pix(x, y, w, h, pix, sbpl, pal)
int x, y, w, h, sbpl;
unsigned char *pix, *pal;
{
	unsigned char *s, *r, *p;
	unsigned long lo, hi, v;
	unsigned int m, pw;
	int x0 = x, y0 = y, cw = w, ch = h, i, j, k, g, a, e, b, sx;

	if (!clip(&x0, &y0, &cw, &ch))
		return;
	sx = x0 - x;
	pix += (long)(y0 - y) * sbpl;
	getpens(pix + sx, cw, ch, (long)sbpl, pal);
	for (j = 0; j < ch; ++j, pix += sbpl) {
		s = pix + sx;
		r = buf + (long)(y0 + j) * bpl;
		if (kind == B_MONO) {
			for (i = 0; i < cw; ++i)
				span(y0 + j, x0 + i, x0 + i + 1, map[s[i]], 0);
			continue;
		}
		if (kind == B_PACKED) {
			p = r + (long)x0 * bpp;
			if (bpp == 1)
				for (i = 0; i < cw; ++i)
					*p++ = (unsigned char)map[s[i]];
			else
				for (i = 0; i < cw; ++i, p += bpp)
					put(p, map[s[i]]);
			continue;
		}
		/* bit planes: 16 pixels at a time, 4 planes in a long (a
		 * byte each), 8 pixels in a byte */
		for (i = 0; i < cw; i += 16 - a) {
			g = (x0 + i) >> 4;
			a = (x0 + i) & 15;
			e = cw - i + a < 16 ? cw - i + a : 16;
			m = (0xffffU >> a) & (0xffffU << (16 - e)) & 0xffff;
			p = r + (long)g * 2 * planes;
			for (b = 0; b < planes; b += 4) {
				lo = hi = 0;
				for (k = 0; k < 16; ++k) {
					v = k >= a && k < e ? map[s[i + k - a]] >> b & 15 : 0;
					if (k < 8)
						hi = hi << 1 | s4[v];
					else
						lo = lo << 1 | s4[v];
				}
				for (k = 0; k < 4 && b + k < planes; ++k) {
					pw = (unsigned int)((hi >> (8 * k) & 0xff) << 8 |
						(lo >> (8 * k) & 0xff));
					PUT16(p + 2 * (b + k), (W16(p + 2 * (b + k)) & ~m) | (pw & m));
				}
			}
		}
	}
}

/* white at x, y, w, h of the window */
static void blank(x, y, w, h)
int x, y, w, h;
{
	static short pxy[4], rx, ry, rw, rh;
	int x0, y0, x1, y1;

	if (wh < 0 || w <= 0 || h <= 0)
		return;
	wind_update(BEG_UPDATE);
	graf_mouse(M_OFF, (MFORM *)0);
	vswr_mode(vh, MD_REPLACE);
	vsf_interior(vh, FIS_SOLID);
	vsf_perimeter(vh, 0);
	vsf_color(vh, 0);
	wind_get(wh, WF_FIRSTXYWH, &rx, &ry, &rw, &rh);
	while (rw > 0 && rh > 0) {
		x0 = rx > wx + x ? rx : wx + x;
		y0 = ry > wy + y ? ry : wy + y;
		x1 = rx + rw < wx + x + w ? rx + rw : wx + x + w;
		y1 = ry + rh < wy + y + h ? ry + rh : wy + y + h;
		if (x0 < x1 && y0 < y1) {
			pxy[0] = x0;
			pxy[1] = y0;
			pxy[2] = x1 - 1;
			pxy[3] = y1 - 1;
			vs_clip(vh, 1, pxy);
			vr_recfl(vh, pxy);
		}
		wind_get(wh, WF_NEXTXYWH, &rx, &ry, &rw, &rh);
	}
	vs_clip(vh, 0, pxy);
	graf_mouse(M_ON, (MFORM *)0);
	wind_update(END_UPDATE);
}

/* buf's part x, y, w, h (in the window) to the screen, in the parts of
 * the window that show */
static void show(x, y, w, h)
int x, y, w, h;
{
	static short pxy[8], col[2];
	static short rx, ry, rw, rh;
	int x0, y0, x1, y1;

	if (x < 0) {
		w += x;
		x = 0;
	}
	if (y < 0) {
		h += y;
		y = 0;
	}
	if (x + w > bw)
		w = bw - x;
	if (y + h > bh)
		h = bh - y;
	if (wh < 0 || !buf || w <= 0 || h <= 0)
		return;
	col[0] = 1;		/* black for the 1 bits */
	col[1] = 0;
	wind_update(BEG_UPDATE);
	graf_mouse(M_OFF, (MFORM *)0);
	wind_get(wh, WF_FIRSTXYWH, &rx, &ry, &rw, &rh);
	while (rw > 0 && rh > 0) {
		x0 = rx > wx + x ? rx : wx + x;
		y0 = ry > wy + y ? ry : wy + y;
		x1 = rx + rw < wx + x + w ? rx + rw : wx + x + w;
		y1 = ry + rh < wy + y + h ? ry + rh : wy + y + h;
		if (x0 < 0)
			x0 = 0;
		if (y0 < 0)
			y0 = 0;
		if (x1 > sw)
			x1 = sw;
		if (y1 > sh)
			y1 = sh;
		if (x0 < x1 && y0 < y1) {
			pxy[0] = x0 - wx;
			pxy[1] = y0 - wy;
			pxy[2] = x1 - 1 - wx;
			pxy[3] = y1 - 1 - wy;
			pxy[4] = x0;
			pxy[5] = y0;
			pxy[6] = x1 - 1;
			pxy[7] = y1 - 1;
			if (kind == B_MONO)
				vrt_cpyfm(vh, MD_REPLACE, pxy, &bm, &scr, col);
			else
				vro_cpyfm(vh, S_ONLY, pxy, &bm, &scr);
		}
		wind_get(wh, WF_NEXTXYWH, &rx, &ry, &rw, &rh);
	}
	graf_mouse(M_ON, (MFORM *)0);
	wind_update(END_UPDATE);
}

/* the work area on the screen */
static void work()
{
	wind_get(wh, WF_WORKXYWH, &wx, &wy, &wkw, &wkh);
}

/* AES 4 and newer (MultiTOS, MagiC, XaAES, N.AES) */
static int aes4()
{
	return (_AESversion & 0xffff) >= 0x400;
}

int g_open(geom, w, h)
char *geom;
int *w, *h;
{
	static short wi[11], wo[57];
	static short d, dx, dy, dw, dh, bx, by, bwd, bht;
	int gw = 800, gh = 600, x = 0, y = 0, f, i, k;
	char *p;

	if (ap < 0)
		ap = appl_init();
	if (ap < 0)
		return -1;
	if (aes4()) {
		shel_write(9, 1, 0, (char *)0, (char *)0);	/* Archaic knows AP_TERM */
		menu_register(ap, "  Archaic");
	}
	vh = graf_handle(&d, &d, &d, &d);
	for (i = 0; i < 10; ++i)
		wi[i] = 1;
	wi[10] = 2;		/* raster coordinates */
	v_opnvwk(wi, &vh, wo);
	if (vh <= 0) {
		appl_exit();
		ap = -1;
		return -1;
	}
	sw = wo[0] + 1;
	sh = wo[1] + 1;
	screen();
	for (i = 0; i < 16; ++i)	/* bit k of v at bit 8 k */
		for (s4[i] = 0, k = 0; k < 4; ++k)
			if (i >> k & 1)
				s4[i] |= 1L << (8 * k);
	wind_get(0, WF_WORKXYWH, &dx, &dy, &dw, &dh);	/* the desktop */
	if (geom) {		/* =WxH+X+Y, or WxH+X+Y */
		p = geom;
		if (*p == '=')
			++p;
		f = 0;
		if (*p >= '0' && *p <= '9') {
			for (gw = 0; *p >= '0' && *p <= '9'; ++p)
				gw = gw * 10 + *p - '0';
			if (*p == 'x' || *p == 'X')
				for (++p, gh = 0; *p >= '0' && *p <= '9'; ++p)
					gh = gh * 10 + *p - '0';
		}
		if (*p == '+' || *p == '-') {
			f |= *p == '-' ? 4 : 0;
			for (++p; *p >= '0' && *p <= '9'; ++p)
				x = x * 10 + *p - '0';
			if (*p == '+' || *p == '-') {
				f |= *p == '-' ? 8 : 0;
				for (++p; *p >= '0' && *p <= '9'; ++p)
					y = y * 10 + *p - '0';
				f |= 2;
			}
		}
	} else
		f = 0;
	if (gw < MINW)
		gw = MINW;
	if (gh < MINH)
		gh = MINH;
	wind_calc(WC_BORDER, KIND, 0, 0, gw, gh, &bx, &by, &bwd, &bht);
	if (bwd > dw) {
		gw -= bwd - dw;
		bwd = dw;
	}
	if (bht > dh) {
		gh -= bht - dh;
		bht = dh;
	}
	if (f & 2) {
		bx = f & 4 ? dx + dw - bwd - x : dx + x;
		by = f & 8 ? dy + dh - bht - y : dy + y;
	} else {
		bx = dx + (dw - bwd) / 2;
		by = dy + (dh - bht) / 2;
	}
	if (bx < dx)
		bx = dx;
	if (by < dy)
		by = dy;
	wh = wind_create(KIND, dx, dy, dw, dh);
	if (wh < 0) {
		v_clsvwk(vh);
		appl_exit();
		ap = -1;
		return -1;
	}
	strcpy(wtitle, "Archaic");
	wind_set_str(wh, WF_NAME, wtitle);
	wind_set(wh, WF_VSLSIZE, 1000, 0, 0, 0);
	wind_set(wh, WF_VSLIDE, 0, 0, 0, 0);
	wind_open(wh, bx, by, bwd, bht);
	work();
	if (npens) {
		probe(wx, wy);
		readpal();
		white = pixof[0];
		black = pixof[1];
	} else if (kind == B_PACKED) {
		white = rt[255] | gt[255] | bt[255];
		black = 0;
	} else {
		white = 0;
		black = 1;
	}
	graf_mouse(ARROW, (MFORM *)0);
	if (newbuf(wkw, wkh) < 0)
		return -1;
	*w = wkw;
	*h = wkh;
	return 0;
}

/* how many colors the screen shows */
long g_colors()
{
	return ncolors;
}

void g_close()
{
	int i;

	if (setpal)		/* the palette as it was */
		for (i = 0; i < npens; ++i)
			vs_color(vh, i, orig + 3 * i);
	setpal = 0;
	if (wh >= 0) {
		wind_close(wh);
		wind_delete(wh);
		wh = -1;
	}
	if (vh > 0)
		v_clsvwk(vh);
	vh = 0;
	if (ap >= 0)
		appl_exit();
	ap = -1;
}

/* the title is ISO 8859-1, GEM's is the Atari's characters */
void g_title(s)
char *s;
{
	int c, i, n = 0;

	for (; *s && n < (int)sizeof wtitle - 1; ++s) {
		c = *s & 0377;
		if (c >= 0200) {
			for (i = 0; i < 128 && latin[i] != c; ++i)
				;
			c = i < 128 ? i + 0200 : '?';
		}
		wtitle[n++] = c;
	}
	wtitle[n] = 0;
	if (wh >= 0)
		wind_set_str(wh, WF_NAME, wtitle);
}

void g_flush()
{
	if (dx0 < dx1 && dy0 < dy1)
		show(dx0, dy0, dx1 - dx0, dy1 - dy0);
	dx0 = dx1 = dy0 = dy1 = 0;
}

int g_fd()
{
	return -1;
}

/* a clock in milliseconds (for the spinner): _hz_200 */
long g_ms()
{
	Supexec(gethz);
	return hz * 5 & 0x3fffffffL;
}

/* the page is at pos (0 to max), vis pixels of it show: the slider,
 * in GEM's 0 to 1000 */
void g_sbar(pos, max, vis)
long pos, max, vis;
{
	long a = vis, b = max + vis;
	int s, p;

	smax = max;
	if (wh < 0 || vis <= 0)
		return;
	while (b > 1000000L) {
		a /= 10;
		b /= 10;
	}
	s = b > 0 ? (int)(a * 1000 / b) : 1000;
	if (s < 1)
		s = 1;
	if (s > 1000)
		s = 1000;
	a = pos;
	b = max;
	while (b > 1000000L) {
		a /= 10;
		b /= 10;
	}
	p = b > 0 ? (int)(a * 1000 / b) : 0;
	if (p < 0)
		p = 0;
	if (p > 1000)
		p = 1000;
	if (s != lsize)
		wind_set(wh, WF_VSLSIZE, s, 0, 0, 0);
	if (p != lpos)
		wind_set(wh, WF_VSLIDE, p, 0, 0, 0);
	lsize = s;
	lpos = p;
}

static Event *put1(type)
int type;
{
	Event *e = &q[nq < NQ ? nq++ : NQ - 1];

	e->type = type;
	e->x = e->y = e->w = e->h = e->key = 0;
	return e;
}

static void scrl(y, unit)
int y, unit;
{
	Event *e = put1(E_SCROLL);

	e->y = y;
	e->key = unit;
}

/* the window has a new size: a new buf, and E_SIZE */
static void sized()
{
	Event *e;

	work();
	if (wkw == bw && wkh == bh)
		return;
	if (newbuf(wkw, wkh) < 0) {
		put1(E_QUIT);
		return;
	}
	e = put1(E_SIZE);
	e->w = wkw;
	e->h = wkh;
}

/* a mouse button is down now */
static int button()
{
	static short x, y, b, k;

	graf_mkstate(&x, &y, &b, &k);
	return b & 3;
}

/* WM_SIZED: the window to x, y, w, h, the AES's own (unless the work
 * area would be smaller than MINW x MINH). MagiC sizes windows in real
 * time: a WM_SIZED as the mouse moves, while the button is down. Then
 * only the window changes, and sized() (a new buf, the page laid out
 * again, WRP's picture at the new width) waits for the release: with
 * sized() at each WM_SIZED, MagiC sent no more WM_SIZED for that drag
 * (MagiC 6.2 on a TT). Until then the window's new parts are white. */
static void resize(x, y, w, h)
int x, y, w, h;
{
	static short a, b, c, d;

	wind_calc(WC_WORK, KIND, x, y, w, h, &a, &b, &c, &d);
	if (c < MINW || d < MINH) {
		if (c < MINW)
			c = MINW;
		if (d < MINH)
			d = MINH;
		wind_calc(WC_BORDER, KIND, a, b, c, d, &a, &b, &c, &d);
		x = a;
		y = b;
		w = c;
		h = d;
	}
	wind_set(wh, WF_CURRXYWH, x, y, w, h);
	if (button()) {		/* MagiC sizes in real time: the rest at the release */
		szpend = 1;
		work();
		return;
	}
	sized();
}

/* WM_FULLED: the full size, or back */
static void full()
{
	static short x, y, w, h, fx, fy, fw, fh;

	wind_get(wh, WF_CURRXYWH, &x, &y, &w, &h);
	wind_get(wh, WF_FULLXYWH, &fx, &fy, &fw, &fh);
	if (x == fx && y == fy && w == fw && h == fh)
		wind_get(wh, WF_PREVXYWH, &fx, &fy, &fw, &fh);
	wind_set(wh, WF_CURRXYWH, fx, fy, fw, fh);
	sized();
}

static void mesg(m)
short *m;
{
	long y;
	int rx, ry;

	switch (m[0]) {
	case WM_REDRAW:
		if (m[3] != wh)
			break;
		show(m[4] - wx, m[5] - wy, m[6], m[7]);
		if (szpend) {	/* bigger than buf until the release: white */
			rx = m[4] - wx;
			ry = m[5] - wy;
			if (rx + m[6] > bw)
				blank(rx > bw ? rx : bw, ry, rx + m[6] - (rx > bw ? rx : bw), m[7]);
			if (ry + m[7] > bh)
				blank(rx, ry > bh ? ry : bh, m[6], ry + m[7] - (ry > bh ? ry : bh));
		}
		break;
	case WM_TOPPED:
		if (m[3] == wh)
			wind_set(wh, WF_TOP, 0, 0, 0, 0);
		break;
	case WM_CLOSED:
	case AP_TERM:
		put1(E_QUIT);
		break;
	case WM_MOVED:
		if (m[3] == wh) {
			wind_set(wh, WF_CURRXYWH, m[4], m[5], m[6], m[7]);
			work();
		}
		break;
	case WM_SIZED:
		if (m[3] == wh)
			resize(m[4], m[5], m[6], m[7]);
		break;
	case WM_FULLED:
		if (m[3] == wh)
			full();
		break;
	case WM_ARROWED:
		if (m[3] != wh)
			break;
		switch (m[4]) {
		case WA_UPPAGE:
			scrl(-1, S_PAGE);
			break;
		case WA_DNPAGE:
			scrl(1, S_PAGE);
			break;
		case WA_UPLINE:
			scrl(-1, S_STEP);
			break;
		case WA_DNLINE:
			scrl(1, S_STEP);
			break;
		}
		break;
	case WM_VSLID:
		if (m[3] != wh || smax < 0)
			break;
		y = smax > 1000000L ? smax / 1000 * m[4] : smax * m[4] / 1000;
		scrl((int)y, S_TO);
		break;
	}
}

/* a key: k is the scan code << 8 | the character, ks the shift keys */
static void keyb(k, ks)
int k, ks;
{
	int sc = k >> 8 & 0377, c = k & 0377, sh = ks & (K_LSHIFT | K_RSHIFT);
	Event *e;

	switch (sc) {
	case 0x48:		/* up */
		scrl(-1, sh ? S_PAGE : S_STEP);
		return;
	case 0x50:		/* down */
		scrl(1, sh ? S_PAGE : S_STEP);
		return;
	case 0x49:		/* Page Up, Page Down (a PC keyboard) */
		scrl(-1, S_PAGE);
		return;
	case 0x51:
		scrl(1, S_PAGE);
		return;
	case 0x47:		/* ClrHome */
		scrl(sh ? (int)BIG : 0, S_TO);
		return;
	case 0x61:		/* Undo */
		c = 033;
		break;
	}
	if (c == 0)
		return;
	if (c >= 0200 && (c = latin[c - 0200]) == 0)
		return;
	e = put1(E_KEY);
	e->key = c;
}

/* a mouse event at x, y on the screen */
static void at(type, x, y, key)
int type, x, y, key;
{
	Event *e = put1(type);

	e->x = x - wx;
	e->y = y - wy;
	e->key = key;
}

/* the buttons are b now, the mouse at x, y on the screen */
static void mouse(b, x, y)
int b, x, y;
{
	if (!held) {
		if (!(b & 3))
			return;
		held = 1;
		if (x < wx || x >= wx + wkw || y < wy || y >= wy + wkh ||
			wind_find(x, y) != wh)
			return;	/* not in the window: no E_DOWN, no E_UP */
		down = b & 1 ? 1 : 3;
		at(E_DOWN, x, y, down);
	} else if (!(b & 3)) {
		held = 0;
		if (down)
			at(E_UP, x, y, down);
		down = 0;
	}
}

int g_event(e, wait)
Event *e;
int wait;
{
	static short m[16], x, y, b, ks, k, n;
	Event *p;
	int fl, ev;

	for (;;) {
		if (iq < nq) {
			p = &q[iq++];
			e->type = p->type;
			e->x = p->x;
			e->y = p->y;
			e->w = p->w;
			e->h = p->h;
			e->key = p->key;
			return 1;
		}
		nq = iq = 0;
		fl = MU_MESAG | MU_KEYBD | MU_BUTTON;
		if (down)
			fl |= MU_M1;	/* the mouse leaves where it was */
		if (wait != 1 || szpend)
			fl |= MU_TIMER;
		/* no button held: wait for one down (0x101: not state 0);
		 * held: wait for all up */
		ev = evnt_multi(fl, held ? 1 : 0x101, 3, 0,
			1, mx, my, 1, 1, 0, 0, 0, 0, 0,
			m, szpend ? 50L : wait == 2 ? 100L : 0L, &x, &y, &b, &ks, &k, &n);
		if (ev & MU_MESAG)
			mesg(m);
		if (szpend && !button()) {
			szpend = 0;
			sized();
		}
		if (ev & MU_KEYBD)
			keyb(k & 0xffff, ks);
		if (ev & MU_BUTTON)
			mouse(b, x, y);
		else if ((ev & MU_M1) && down)
			at(E_MOVE, x, y, 0);
		mx = x;
		my = y;
		if (iq < nq)
			continue;
		if (wait != 1 && (ev & MU_TIMER))
			return 0;
	}
}

int archaic_main P((int argc, char **argv));

/* The options: the command line's, else the words of ARCHAIC.INF in
 * Archaic's folder (the desktop gives a .PRG no command line). The
 * program's path (shel_read) goes in argv[0], with / (for the updates).
 * With -v, the messages go to ARCHAIC.LOG there, not over GEM's
 * screen. */
int main(argc, argv)
int argc;
char **argv;
{
	static char *av[34], cmd[256], tail[256], ob[512], fn[300];
	FILE *f;
	char *p;
	int ac = 0, i, n, v = 0;

	ap = appl_init();
	cmd[0] = 0;
	if (ap >= 0)
		shel_read(cmd, tail);
	for (p = cmd; *p; ++p)
		if (*p == '\\')
			*p = '/';
	av[ac++] = cmd[0] && cmd[1] == ':' ? cmd : argc > 0 ? argv[0] : "archaic";
	for (i = 1; i < argc && ac < 32; ++i)
		av[ac++] = argv[i];
	/* the folder of the program */
	scopy(fn, av[0], 250);
	for (n = (int)strlen(fn); n > 0 && fn[n - 1] != '/' && fn[n - 1] != '\\' &&
		fn[n - 1] != ':'; --n)
		;
	fn[n] = 0;
	if (argc < 2) {
		scopy(fn + n, "ARCHAIC.INF", 20);
		f = fopen(fn, "r");
		if (!f)
			f = fopen("ARCHAIC.INF", "r");
		if (f) {
			n = (int)fread(ob, 1, sizeof ob - 1, f);
			fclose(f);
			ob[n > 0 ? n : 0] = 0;
			for (p = ob; *p && ac < 32;) {
				while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
					*p++ = 0;
				if (!*p)
					break;
				av[ac++] = p;
				while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
					++p;
			}
		}
		fn[(int)strlen(fn) - 11] = 0;
	}
	av[ac] = 0;
	for (i = 1; i < ac; ++i)
		if (!strcmp(av[i], "-v"))
			v = 1;
	if (v) {
		scopy(fn + (int)strlen(fn), "ARCHAIC.LOG", 20);
		freopen(fn, "w", stderr);
		freopen(fn, "a", stdout);
		setvbuf(stderr, (char *)0, _IONBF, 0);
	}
	i = archaic_main(ac, av);
	if (ap >= 0) {
		appl_exit();
		ap = -1;
	}
	return i;
}
