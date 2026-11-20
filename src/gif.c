/* gif.c - GIF decoder for Archaic: GIF87a and GIF89a, the first image,
 * interlaced or not. The bytes come from http_getc() as they arrive,
 * and finished rows go to the screen at once (img_rows), so the page
 * shows from the top down while it loads.
 *
 * im->depth says what the pixels become. 1: 1 bit, each color's
 * brightness against a 4x4 ordered dither; WRP's 2-color GIFs (black
 * and white only) come out exact. 8: the color numbers themselves, a
 * byte each, and the colors in im->pal, for a display that shows them.
 */
#include "archaic.h"

#ifdef NOPROTO
char *malloc();
void free();
#else
#include <stdlib.h>
#endif

static int gray[256];			/* brightness 0..255 of each color */
static unsigned char rgb[768];		/* red, green, blue of each color */
static short prefix[4096];		/* LZW: code = prefix code + suffix */
static unsigned char suffix[4096];
static unsigned char stk[4097];
static unsigned char row[MAXW];		/* color numbers of one row */
static int dither[4][4] = {		/* 4x4 Bayer thresholds, 8..248 */
	{ 8, 136, 40, 168 },
	{ 200, 72, 232, 104 },
	{ 56, 184, 24, 152 },
	{ 248, 120, 216, 88 }
};
static unsigned char bit[8] = { 0200, 0100, 040, 020, 010, 04, 02, 01 };
static int blk;				/* bytes left in this data block */
static int dend;			/* the last data block has been read */
static unsigned long acc;		/* bits not used yet */
static int nacc;
static Image *im;
static int left, top, iw, ih, ilace, pass, ry;
static int pend0, npend;		/* rows done, not yet shown */

static int get2()
{
	int a, b;

	a = http_getc();
	b = http_getc();
	return b < 0 ? -1 : a | b << 8;
}

/* n colors: their brightness */
static int colors(n)
int n;
{
	int i, r, g, b;

	for (i = 0; i < n; ++i) {
		r = http_getc();
		g = http_getc();
		b = http_getc();
		if (b < 0)
			return -1;
		rgb[3 * i] = r;
		rgb[3 * i + 1] = g;
		rgb[3 * i + 2] = b;
		gray[i] = (int)(((long)r * 77 + (long)g * 150 + (long)b * 29) >> 8);
	}
	return 0;
}

/* 8 bits: the colors into im->pal, and all pixels the lightest color
 * (white, mostly) until their rows come */
static void setpal()
{
	long i, n;
	int c, k;

	for (i = 0; i < 768; ++i)
		im->pal[i] = rgb[i];
	for (c = k = 0; k < 256; ++k)
		if (gray[k] > gray[c])
			c = k;
	n = (long)im->bpl * im->h;
	for (i = 0; i < n; ++i)
		im->bits[i] = c;
}

/* skip data blocks up to the empty one */
static int skip()
{
	int n;

	while ((n = http_getc()) > 0)
		while (n-- > 0)
			if (http_getc() < 0)
				return -1;
	return n;
}

/* the next byte of the image data, -1 at its end */
static int dbyte()
{
	if (dend)
		return -1;
	if (blk == 0) {
		blk = http_getc();
		if (blk <= 0) {
			blk = 0;
			dend = 1;
			return -1;
		}
	}
	--blk;
	return http_getc();
}

/* the next code of size bits, -1 at the end */
static int code(size)
int size;
{
	int c;

	while (nacc < size) {
		c = dbyte();
		if (c < 0)
			return -1;
		acc |= (unsigned long)c << nacc;
		nacc += 8;
	}
	c = (int)(acc & ((1L << size) - 1));
	acc >>= size;
	nacc -= size;
	return c;
}

static void flush()
{
	if (npend > 0)
		img_rows(im, pend0, npend);
	npend = 0;
}

/* row[] has all of image row ry: into the bitmap, then the next row */
static void putrow()
{
	static int step[4] = { 8, 8, 4, 2 };
	static int start[4] = { 0, 4, 2, 1 };
	unsigned char *b;
	int *t;
	int x, y, n;

	y = top + ry;
	if (y < im->h) {
		n = iw;
		if (left + n > im->w)
			n = im->w - left;
		if (im->depth == 8) {
			b = im->bits + (long)y * im->bpl + left;
			for (x = 0; x < n; ++x)
				b[x] = row[x];
		} else {
			b = im->bits + (long)y * im->bpl;
			t = dither[y & 3];
			for (x = 0; x < n; ++x)
				if (gray[row[x]] < t[(left + x) & 3])
					b[(left + x) >> 3] |= bit[(left + x) & 7];
		}
		if (npend > 0 && y == pend0 + npend)
			++npend;
		else {
			flush();
			pend0 = y;
			npend = 1;
		}
		if (npend >= 8)
			flush();
	}
	if (!ilace)
		++ry;
	else {
		ry += step[pass];
		while (ry >= ih && pass < 3) {
			++pass;
			ry = start[pass];
		}
	}
}

/* LZW: the pixels of the image, row by row */
static int lzw()
{
	int mcs, clear, eoi, next, size, old, first, in, c, sp, x;

	mcs = http_getc();
	if (mcs < 2 || mcs > 8)
		return -1;
	clear = 1 << mcs;
	eoi = clear + 1;
	for (c = 0; c < clear; ++c) {
		prefix[c] = -1;
		suffix[c] = c;
	}
	size = mcs + 1;
	next = clear + 2;
	old = -1;
	first = 0;
	blk = dend = 0;
	acc = 0;
	nacc = 0;
	x = 0;
	ry = pass = 0;
	while (ry < ih) {
		c = code(size);
		if (c < 0 || c == eoi)
			break;
		if (c == clear) {
			size = mcs + 1;
			next = clear + 2;
			old = -1;
			continue;
		}
		if (old < 0) {
			if (c >= clear)
				return -1;
			first = old = c;
			row[x++] = c;
			if (x >= iw) {
				putrow();
				x = 0;
			}
			continue;
		}
		in = c;
		sp = 0;
		if (c >= next) {
			if (c > next)
				return -1;
			stk[sp++] = first;
			c = old;
		}
		while (c >= clear) {
			if (sp >= 4096)
				return -1;
			stk[sp++] = suffix[c];
			c = prefix[c];
		}
		first = c;
		stk[sp++] = c;
		while (sp > 0) {
			row[x++] = stk[--sp];
			if (x >= iw) {
				putrow();
				x = 0;
				if (ry >= ih)
					break;
			}
		}
		if (next < 4096) {
			prefix[next] = old;
			suffix[next] = first;
			++next;
			if (next >= 1 << size && size < 12)
				++size;
		}
		old = in;
	}
	return 0;
}

/* read a GIF into img (bits and pal from malloc): 0 ok, -1 not a GIF or
 * broken */
int gif_load(img)
Image *img;
{
	int c, w, h, flags, i, r;
	long n;

	im = img;
	npend = 0;
	if (im->depth != 8)
		im->depth = 1;
	im->pal = 0;
	if (http_getc() != 'G' || http_getc() != 'I' || http_getc() != 'F')
		return -1;
	for (i = 0; i < 3; ++i)
		http_getc();
	w = get2();
	h = get2();
	flags = http_getc();
	http_getc();
	if (http_getc() < 0 || w <= 0 || h <= 0 || w > MAXW || h > 8192)
		return -1;
	for (i = 0; i < 256; ++i) {
		gray[i] = i & 1 ? 0 : 255;
		rgb[3 * i] = rgb[3 * i + 1] = rgb[3 * i + 2] = gray[i];
	}
	if ((flags & 0200) && colors(2 << (flags & 7)) < 0)
		return -1;
	im->w = w;
	im->h = h;
	im->bpl = im->depth == 8 ? w : (w + 7) / 8;
	n = (long)im->bpl * h;
	if (n != (long)(unsigned)n)
		return -1;	/* too big for this machine's malloc */
	im->bits = (unsigned char *)malloc((unsigned)n);
	if (!im->bits)
		return -1;
	if (im->depth == 8) {
		im->pal = (unsigned char *)malloc(768);
		if (!im->pal) {
			free((char *)im->bits);
			im->bits = 0;
			return -1;
		}
		setpal();
	} else
		while (--n >= 0)
			im->bits[n] = 0;
	r = -1;
	for (;;) {
		c = http_getc();
		if (c == '!') {			/* extension: skip it */
			http_getc();
			if (skip() < 0)
				break;
			continue;
		}
		if (c != ',')			/* ';' the end, or broken */
			break;
		left = get2();
		top = get2();
		iw = get2();
		ih = get2();
		flags = http_getc();
		if (flags < 0 || iw <= 0 || ih <= 0 || iw > MAXW || left >= w ||
			top >= h)
			break;
		if (flags & 0200) {		/* its own colors */
			if (colors(2 << (flags & 7)) < 0)
				break;
			if (im->depth == 8)
				setpal();
		}
		ilace = flags & 0100;
		r = lzw();
		if (r == 0) {			/* read to the end: all of it counts */
			while (!dend && blk > 0 && http_getc() >= 0)
				--blk;
			if (!dend)
				skip();
			http_getc();		/* ';' */
		}
		break;				/* only the first image */
	}
	flush();
	return r;
}
