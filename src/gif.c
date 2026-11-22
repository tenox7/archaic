/* gif.c - GIF decoder for Archaic: GIF87a and GIF89a, the first image,
 * interlaced or not. The bytes come from the body as they arrive, and
 * finished rows go to the screen at once (pic_rows), so the page shows
 * from the top down while it loads.
 *
 * im->depth says what the pixels become. 1: 1 bit, each color's
 * brightness against a 4x4 ordered dither; WRP's 2-color GIFs (black
 * and white only) come out exact. 8: the color numbers themselves, a
 * byte each, and the colors in im->pal, for a display that shows them.
 *
 * WRP's black and white GIF on a 1-bit display takes the fast way (lzw1):
 * no pixel is handled alone. It is a GIF whose colors are all black or
 * white, that fills its screen and is not interlaced. For 551 x 310
 * pixels a 10 MHz 68010 needs 7.5 seconds the slow way (44 microseconds
 * a pixel), and 1.3 seconds the fast way. (WRP's ABM is faster still:
 * see pic.c.)
 *
 * With 8 bits, a picture as wide as its screen and not interlaced (all
 * of WRP's) takes the 8-bit way (lzw8): the picture is its own
 * dictionary, so each code is one copy of pixels that came before.
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
/* lzw1: a code's pixels as bits, 16 at a time. tail has its last ntail
 * pixels (1 to 16; the first one is the high bit), prefix the code with
 * the pixels before them (-1: none), suffix its first pixel. */
static unsigned short tail[4096];
static unsigned char ntail[4096];
static short chain[260];		/* a long string's codes, last first */
/* lzw8: where each code's pixels are in im->bits (prefix has how many).
 * From malloc, the first time a picture needs it. */
static unsigned char **where;
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

	a = HGETC();
	b = HGETC();
	return b < 0 ? -1 : a | b << 8;
}

/* n colors: their brightness */
static int colors(n)
int n;
{
	int i, r, g, b;

	for (i = 0; i < n; ++i) {
		r = HGETC();
		g = HGETC();
		b = HGETC();
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
	int c, i;

	for (i = 0; i < 768; ++i)
		im->pal[i] = rgb[i];
	for (c = i = 0; i < 256; ++i)
		if (gray[i] > gray[c])
			c = i;
	pic_fill(im->bits, (long)im->bpl * im->h, c);
}

/* skip data blocks up to the empty one */
static int skip()
{
	int n;

	while ((n = HGETC()) > 0)
		while (n-- > 0)
			if (HGETC() < 0)
				return -1;
	return n;
}

/* the next byte of the image data, at the start of a data block; -1 at
 * the end of the data */
static int dblock()
{
	if (dend)
		return -1;
	blk = HGETC();
	if (blk <= 0) {
		blk = 0;
		dend = 1;
		return -1;
	}
	--blk;
	return HGETC();
}

/* the next byte of the image data, -1 at its end */
#define DBYTE() (blk > 0 ? (--blk, HGETC()) : dblock())

/* The fast way keeps the body's bytes in registers: the image data's
 * bytes at hand are ip up to lim (the end of the data block, or of what
 * has come). dmore() gives the next byte when they are used up, and
 * sets http_p and dlim for the ones after it; -1 at the end of the data. */
static unsigned char *dlim;

static int dmore()
{
	long n;
	int c;

	dlim = http_p;
	if (dend)
		return -1;
	if (blk == 0) {
		blk = HGETC();
		if (blk <= 0) {
			blk = 0;
			dend = 1;
			dlim = http_p;
			return -1;
		}
	}
	c = HGETC();
	if (c < 0) {
		dend = 1;
		dlim = http_p;
		return -1;
	}
	--blk;
	n = http_e - http_p;
	if (n > blk)
		n = blk;
	blk -= (int)n;
	dlim = http_p + n;
	return c;
}

static void flush()
{
	if (npend > 0)
		pic_rows(im, pend0, npend);
	npend = 0;
}

/* image row y is done: show the rows in eights */
static void rowdone(y)
int y;
{
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
		rowdone(y);
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
static int lzw(mcs)
int mcs;
{
	int clear, eoi, next, size, old, first, in, c, sp, x;

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
	x = 0;
	while (ry < ih) {
		while (nacc < size) {
			c = DBYTE();
			if (c < 0)
				return 0;
			acc |= (unsigned long)c << nacc;
			nacc += 8;
		}
		c = (int)(acc & ((1L << size) - 1));
		acc >>= size;
		nacc -= size;
		if (c == eoi)
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

/* LZW, the fast way: black and white pixels straight to the 1-bit rows.
 * Each code's string is kept as bits (see tail), so a code gives up to
 * 16 pixels at a time. For a picture that fills its screen, and is not
 * interlaced. */
static int lzw1(mcs)
int mcs;
{
	register unsigned long acc;	/* code bits not used yet, nacc of them */
	register unsigned long oacc;	/* the row's bits not stored yet, */
	register int on;		/*  on of them, from the high bit down */
	register unsigned char *ip, *lim, *op;
	register int c, k, nacc;
	unsigned int v;
	int clear, eoi, next, size, mask, top, old, n, m, f, rleft, y, w, h;

	clear = 1 << mcs;
	eoi = clear + 1;
	for (c = 0; c < clear; ++c) {
		f = gray[c] < 128;
		prefix[c] = -1;
		suffix[c] = f;
		tail[c] = f ? 0x8000 : 0;
		ntail[c] = 1;
	}
	size = mcs + 1;
	mask = (1 << size) - 1;
	top = 1 << size;
	next = clear + 2;
	old = -1;
	acc = 0;
	nacc = 0;
	oacc = 0;
	on = 0;
	op = im->bits;
	w = iw;
	h = ih;
	rleft = w;
	y = 0;
	ip = lim = http_p;
	for (;;) {
		while (nacc < size) {
			if (ip < lim)
				c = *ip++;
			else {
				http_p = ip;
				c = dmore();
				ip = http_p;
				lim = dlim;
				if (c < 0)
					goto out;
			}
			acc |= (unsigned long)c << nacc;
			nacc += 8;
		}
		c = (int)acc & mask;
		acc >>= size;
		nacc -= size;
		if (c == eoi)
			break;
		if (c == clear) {
			size = mcs + 1;
			mask = (1 << size) - 1;
			top = 1 << size;
			next = clear + 2;
			old = -1;
			continue;
		}
		if (old < 0) {
			if (c >= clear)
				goto bad;
		} else {
			if (c > next || (c == next && next >= 4096))
				goto bad;
			if (next < 4096) {	/* the last string and one pixel */
				f = suffix[c == next ? old : c];
				k = ntail[old];
				if (k < 16) {
					tail[next] = tail[old] | (f ? 0x8000 >> k : 0);
					ntail[next] = k + 1;
					prefix[next] = prefix[old];
				} else {
					tail[next] = f ? 0x8000 : 0;
					ntail[next] = 1;
					prefix[next] = old;
				}
				suffix[next] = suffix[old];
				if (++next >= top && size < 12) {
					++size;
					mask = (1 << size) - 1;
					top = 1 << size;
				}
			}
		}
		old = c;
		/* the string of c: its parts, the first one first */
		n = 0;
		for (k = c; prefix[k] >= 0; k = prefix[k]) {
			if (n >= 258)
				goto bad;
			chain[n++] = k;
		}
		for (;;) {
			v = tail[k];
			m = ntail[k];
			while (m >= rleft) {	/* the row ends in this part */
				oacc |= (unsigned long)(v & (0xffff0000L >> rleft)) << (16 - on);
				on += rleft;
				while (on > 0) {
					*op++ = (unsigned char)(oacc >> 24);
					oacc <<= 8;
					on -= 8;
				}
				oacc = 0;
				on = 0;
				v = (v << rleft) & 0xffff;
				m -= rleft;
				rleft = w;
				http_p = ip;
				rowdone(y);
				if (++y >= h)
					goto out;
			}
			if (m > 0) {
				oacc |= (unsigned long)v << (16 - on);
				on += m;
				rleft -= m;
				if (on >= 16) {
					c = (int)(oacc >> 16);
					*op++ = (unsigned char)(c >> 8);
					*op++ = (unsigned char)c;
					oacc <<= 16;
					on -= 16;
				}
			}
			if (n == 0)
				break;
			k = chain[--n];
		}
	}
out:
	http_p = ip;
	ry = y;
	/* the data bytes after lim are not taken yet: give them back to blk */
	blk += (int)(lim - ip);
	return 0;
bad:
	http_p = ip;
	ry = y;
	blk += (int)(lim - ip);
	return -1;
}

/* LZW, the 8-bit way: the picture is its own dictionary (as Larry Bank's
 * AnimatedGIF does it). A code's pixels are already in the picture, where
 * they came the first time: where[] has the place, prefix[] how many. So
 * a code is one copy, and each pixel is written once. (The slow way puts
 * each pixel on a stack, then into row[], then into the picture.) For an
 * 8-bit picture as wide as its screen, not interlaced: its rows lie
 * together in im->bits. */
static int lzw8(mcs)
int mcs;
{
	register unsigned long acc;	/* code bits not used yet, nacc of them */
	register unsigned char *ip, *lim;	/* the image data at hand */
	register unsigned char *op, *sp;	/* the next pixel; a copy's start */
	register int c, n, nacc;
	unsigned char *end, *rowe;
	unsigned char *pp;	/* where the last code's pixels are, */
	int plen;		/*  how many */
	int clear, eoi, next, size, mask, cmax, w, y, yend;

	clear = 1 << mcs;
	eoi = clear + 1;
	size = mcs + 1;
	mask = (1 << size) - 1;
	cmax = 1 << size;
	/* After a clear, the first code makes no new code. So that no test
	 * is needed for it, it makes one at eoi, which is never looked up. */
	next = eoi;
	acc = 0;
	nacc = 0;
	w = im->w;
	y = top;
	yend = top + ih < im->h ? top + ih : im->h;
	op = im->bits + (long)y * w;
	end = im->bits + (long)yend * w;
	rowe = op + w;
	pp = op;
	plen = 0;
	ip = lim = http_p;
	for (;;) {
		while (nacc < size) {
			if (ip < lim)
				c = *ip++;
			else {
				http_p = ip;
				c = dmore();
				ip = http_p;
				lim = dlim;
				if (c < 0)
					goto out;
			}
			acc |= (unsigned long)c << nacc;
			nacc += 8;
		}
		c = (int)acc & mask;
		acc >>= size;
		nacc -= size;
		if (c < clear)			/* one pixel */
			n = 0;
		else if (c < next && c > eoi) {	/* an earlier code's pixels */
			n = prefix[c];
			sp = where[c];
		} else if (c == next && c > eoi) {
			n = plen + 1;		/* the last pixels and their first */
			sp = pp;
		} else if (c == clear) {
			size = mcs + 1;
			mask = (1 << size) - 1;
			cmax = 1 << size;
			next = eoi;
			continue;
		} else if (c == eoi)
			break;
		else
			goto bad;
		if (next < 4096) {
			/* a new code: the last code's pixels, and the first
			 * pixel of these, which comes next to them */
			where[next] = pp;
			prefix[next] = plen + 1;
			if (++next >= cmax && size < 12) {
				++size;
				mask = (1 << size) - 1;
				cmax = 1 << size;
			}
		}
		pp = op;
		if (n == 0) {
			plen = 1;
			if (op < end)
				*op++ = c;
		} else {
			plen = n;
			if ((long)n > end - op)		/* below the screen */
				n = (int)(end - op);
			/* forward: at c == next, the last pixel is the first */
			while (n >= 4) {
				*op++ = *sp++;
				*op++ = *sp++;
				*op++ = *sp++;
				*op++ = *sp++;
				n -= 4;
			}
			while (--n >= 0)
				*op++ = *sp++;
		}
		if (op >= rowe) {		/* rows done */
			http_p = ip;
			do {
				rowdone(y);
				rowe += w;
				if (++y >= yend)
					goto out;
			} while (op >= rowe);
		}
	}
out:
	http_p = ip;
	ry = y - top;
	blk += (int)(lim - ip);	/* the bytes after ip are not taken yet */
	return 0;
bad:
	http_p = ip;
	ry = y - top;
	blk += (int)(lim - ip);
	return -1;
}

/* read a GIF into img (bits and pal from malloc), after its first two
 * bytes "GI": 0 ok, -1 not a GIF or broken */
int gif_load(img)
Image *img;
{
	int c, w, h, flags, i, r, mcs, pure;
	long n;

	im = img;
	npend = 0;
	if (im->depth != 8)
		im->depth = 1;
	im->pal = 0;
	if (HGETC() != 'F')
		return -1;
	for (i = 0; i < 3; ++i)
		HGETC();
	w = get2();
	h = get2();
	flags = HGETC();
	HGETC();
	if (HGETC() < 0 || w <= 0 || h <= 0 || w > MAXW || h > 8192)
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
		pic_wipe(im->bits, n);
	r = -1;
	for (;;) {
		c = HGETC();
		if (c == '!') {			/* extension: skip it */
			HGETC();
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
		flags = HGETC();
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
		mcs = HGETC();
		if (mcs < 2 || mcs > 8)
			break;
		blk = dend = 0;
		acc = 0;
		nacc = 0;
		ry = pass = 0;
		/* pure black and white: the dither changes no pixel */
		pure = im->depth == 1 && !ilace && left == 0 && top == 0 &&
			iw == w && ih == h;
		for (i = 0; pure && i < 1 << mcs; ++i)
			if (gray[i] > 7 && gray[i] < 248)
				pure = 0;
		if (pure)
			r = lzw1(mcs);
		else if (im->depth == 8 && !ilace && left == 0 && iw == w &&
			(where || (where = (unsigned char **)malloc((unsigned)
			(4096 * sizeof *where))) != 0))
			r = lzw8(mcs);
		else
			r = lzw(mcs);
		if (r == 0) {			/* read to the end: all of it counts */
			while (!dend && blk > 0 && HGETC() >= 0)
				--blk;
			if (!dend)
				skip();
			HGETC();		/* ';' */
		}
		break;				/* only the first image */
	}
	flush();
	return r;
}
