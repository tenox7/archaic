/* fb.c - drawing into a screen in memory, for the displays that show
 * the pixels themselves (sdl.c, cocoa.m, headless.c). Black and white,
 * and one font: the X11 6x13 font (font6x13.h), so the page looks the
 * same as with x11.c. fb_shot writes the screen to a file.
 */
#include "archaic.h"
#include "fb.h"
#include "font6x13.h"

#ifdef NOPROTO
char *malloc();
void free();
#else
#include <stdlib.h>
#endif

unsigned int *fb;
int fbw, fbh;
int fbx0, fby0, fbx1, fby1;

int fb_size(w, h)
int w, h;
{
	unsigned int *p;
	long i, n;

	if (w < 1)
		w = 1;
	if (h < 1)
		h = 1;
	n = (long)w * h;
	p = (unsigned int *)malloc(n * sizeof *p);
	if (!p)
		return -1;
	for (i = 0; i < n; ++i)
		p[i] = FB_WHITE;
	if (fb)
		free((char *)fb);
	fb = p;
	fbw = w;
	fbh = h;
	fbx0 = fby0 = 0;
	fbx1 = w;
	fby1 = h;
	return 0;
}

void fb_done()
{
	fbx0 = fbx1 = fby0 = fby1 = 0;
}

/* cut x, y, w, h to the screen and mark it changed; 0 if nothing is left */
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
	if (*x + *w > fbw)
		*w = fbw - *x;
	if (*y + *h > fbh)
		*h = fbh - *y;
	if (*w <= 0 || *h <= 0)
		return 0;
	if (fbx0 >= fbx1) {
		fbx0 = *x;
		fby0 = *y;
		fbx1 = *x + *w;
		fby1 = *y + *h;
	} else {
		if (*x < fbx0)
			fbx0 = *x;
		if (*y < fby0)
			fby0 = *y;
		if (*x + *w > fbx1)
			fbx1 = *x + *w;
		if (*y + *h > fby1)
			fby1 = *y + *h;
	}
	return 1;
}

void g_fill(x, y, w, h, c)
int x, y, w, h, c;
{
	unsigned int *p, v = c ? FB_BLACK : FB_WHITE;
	int i, j;

	if (!clip(&x, &y, &w, &h))
		return;
	for (j = 0; j < h; ++j) {
		p = fb + (long)(y + j) * fbw + x;
		for (i = 0; i < w; ++i)
			p[i] = v;
	}
}

void g_invert(x, y, w, h)
int x, y, w, h;
{
	unsigned int *p;
	int i, j;

	if (!clip(&x, &y, &w, &h))
		return;
	for (j = 0; j < h; ++j) {
		p = fb + (long)(y + j) * fbw + x;
		for (i = 0; i < w; ++i)
			p[i] ^= FB_WHITE;
	}
}

/* each character fills its cell: the text and its background */
void g_text(x, y, s, n, inv)
int x, y, n, inv;
char *s;
{
	unsigned int fg = inv ? FB_WHITE : FB_BLACK, bg = inv ? FB_BLACK : FB_WHITE;
	unsigned int *p;
	unsigned char *g;
	int k, r, i, cx, cy, x0 = x, y0 = y, w = n * FONTW, h = FONTH;

	if (!clip(&x0, &y0, &w, &h))
		return;
	for (k = 0; k < n; ++k) {
		g = font6x13[s[k] & 0377];
		for (r = 0; r < FONTH; ++r) {
			cy = y + r;
			if (cy < 0 || cy >= fbh)
				continue;
			p = fb + (long)cy * fbw;
			for (i = 0; i < FONTW; ++i) {
				cx = x + k * FONTW + i;
				if (cx >= 0 && cx < fbw)
					p[cx] = g[r] & (0x80 >> i) ? fg : bg;
			}
		}
	}
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
void g_bits(x, y, w, h, bits, bpl)
int x, y, w, h, bpl;
unsigned char *bits;
{
	unsigned char *b;
	unsigned int *p;
	int i, j, x0 = x, y0 = y, cw = w, ch = h;

	if (!clip(&x0, &y0, &cw, &ch))
		return;
	for (j = y0 - y; j < y0 - y + ch; ++j) {
		b = bits + (long)j * bpl;
		p = fb + (long)(y + j) * fbw + x;
		for (i = x0 - x; i < x0 - x + cw; ++i)
			p[i] = b[i >> 3] & (0x80 >> (i & 7)) ? FB_BLACK : FB_WHITE;
	}
}

static char *num(s, v)
char *s;
int *v;
{
	for (*v = 0; *s >= '0' && *s <= '9'; ++s)
		*v = *v * 10 + *s - '0';
	return s;
}

/* X11's geometry, =WxH+X+Y, each part may be missing. Returns 1 if it
 * gave the size, + 2 if it gave the place, + 4 if X counts from the
 * right, + 8 if Y counts from the bottom. */
int fb_geom(g, w, h, x, y)
char *g;
int *w, *h, *x, *y;
{
	int f = 0, neg;

	if (!g)
		return 0;
	if (*g == '=')
		++g;
	if (*g >= '0' && *g <= '9') {
		g = num(g, w);
		if (*g == 'x' || *g == 'X') {
			g = num(g + 1, h);
			f |= 1;
		}
	}
	if (*g == '+' || *g == '-') {
		neg = *g == '-';
		g = num(g + 1, x);
		if (neg)
			f |= 4;
		if (*g == '+' || *g == '-') {
			if (*g == '-')
				f |= 8;
			g = num(g + 1, y);
			f |= 2;
		}
	}
	return f;
}

/* PNG: crc32 of each chunk, adler32 of the zlib data */
static unsigned long crctab[256];
static unsigned long crc, adla, adlb;
static FILE *shot;

static void put4(v)
unsigned long v;
{
	putc((int)(v >> 24 & 0377), shot);
	putc((int)(v >> 16 & 0377), shot);
	putc((int)(v >> 8 & 0377), shot);
	putc((int)(v & 0377), shot);
}

/* a byte of a chunk's type or data */
static void cput(c)
int c;
{
	crc = crctab[(crc ^ c) & 0377] ^ crc >> 8;
	putc(c, shot);
}

/* a byte of the picture: in the chunk, and counted by adler32 */
static void zput(c)
int c;
{
	adla = (adla + c) % 65521L;
	adlb = (adlb + adla) % 65521L;
	cput(c);
}

static void chunk(type, len)
char *type;
unsigned long len;
{
	put4(len);
	crc = 0xffffffffL;
	while (*type)
		cput(*type++);
}

/* the picture: 1 bit a pixel (0 black, 1 white), each row after a 0
 * (no filter), in zlib blocks that are not compressed */
static void pngdata()
{
	long rb = (fbw + 7) / 8 + 1, n = rb * fbh, left, i;
	unsigned int *p;
	int b, c, x, y, k;

	chunk("IDAT", 2 + n + 5 * ((n + 65534) / 65535) + 4);
	cput(0x78);
	cput(0x01);
	adla = 1;
	adlb = 0;
	left = n;
	x = y = 0;
	while (left > 0) {
		k = left > 65535L ? 65535 : (int)left;
		left -= k;
		cput(left == 0);
		cput(k & 0377);
		cput(k >> 8);
		cput(~k & 0377);
		cput(~k >> 8 & 0377);
		for (i = 0; i < k; ++i) {
			if (x == 0)
				c = 0;
			else {
				p = fb + (long)y * fbw + (x - 1) * 8;
				for (c = 0, b = 0; b < 8; ++b)
					if ((x - 1) * 8 + b < fbw && p[b] != FB_BLACK)
						c |= 0x80 >> b;
			}
			zput(c);
			if (++x == rb) {
				x = 0;
				++y;
			}
		}
	}
	cput((int)(adlb >> 8 & 0377));
	cput((int)(adlb & 0377));
	cput((int)(adla >> 8 & 0377));
	cput((int)(adla & 0377));
	put4(crc ^ 0xffffffffL);
}

int fb_shot(file)
char *file;
{
	unsigned long c;
	unsigned int *p;
	int n, k, i, x, y, b;

	shot = fopen(file, "wb");
	if (!shot)
		return -1;
	n = 0;
	while (file[n])
		++n;
	if (n > 4 && strieq(file + n - 4, ".pbm")) {
		fprintf(shot, "P4\n%d %d\n", fbw, fbh);
		for (y = 0; y < fbh; ++y)
			for (x = 0; x < fbw; x += 8) {
				p = fb + (long)y * fbw + x;
				for (b = k = 0; k < 8; ++k)
					if (x + k < fbw && p[k] == FB_BLACK)
						b |= 0x80 >> k;
				putc(b, shot);
			}
		return fclose(shot) == 0 ? 0 : -1;
	}
	for (i = 0; i < 256; ++i) {
		c = i;
		for (k = 0; k < 8; ++k)
			c = c & 1 ? 0xedb88320L ^ c >> 1 : c >> 1;
		crctab[i] = c;
	}
	fwrite("\211PNG\r\n\032\n", 1, 8, shot);
	chunk("IHDR", 13L);
	cput(fbw >> 24 & 0377);
	cput(fbw >> 16 & 0377);
	cput(fbw >> 8 & 0377);
	cput(fbw & 0377);
	cput(fbh >> 24 & 0377);
	cput(fbh >> 16 & 0377);
	cput(fbh >> 8 & 0377);
	cput(fbh & 0377);
	cput(1);	/* 1 bit */
	cput(0);	/* gray */
	cput(0);
	cput(0);
	cput(0);
	put4(crc ^ 0xffffffffL);
	pngdata();
	chunk("IEND", 0L);
	put4(crc ^ 0xffffffffL);
	return fclose(shot) == 0 ? 0 : -1;
}
