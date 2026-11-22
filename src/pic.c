/* pic.c - Archaic's pictures: what comes in the body, into an Image.
 *
 * pic_load() looks at the first two bytes:
 *	"GI"	a GIF (gif.c)
 *	"AB"	ABM: WRP's black and white picture for the 1-bit displays
 *
 * ABM is the display's own bytes, packed with a byte LZ. No pixel is
 * handled alone, so a 68000 decodes it many times faster than a GIF. For
 * one picture of 551 x 310 pixels, a 10 MHz 68010 needs 7.5 seconds as a
 * GIF (1.3 seconds with gif.c's fast way) and 0.15 seconds as ABM. ABM is
 * also about 11% smaller. Archaic's requests say "Accept: image/x-abm"
 * (http_accept): a WRP that knows ABM then sends its black and white
 * pictures (2 colors) as ABM, and all others as GIFs.
 *
 * The file:
 *	"ABM"		3 bytes
 *	how packed	1 byte: '1'
 *	width, height	2 bytes each, the high byte first
 *	depth		1 byte: bits a pixel, 1
 *	flags		1 byte: 0
 *	the rows	packed. Each row is whole bytes; the high bit is the
 *			left pixel; 1 is black (as Image's bits for depth 1)
 * The packed rows are tokens. A control byte, then:
 *	00nnnnnn	n + 1 bytes follow as they are
 *	01nnnnnn	n + 1 white bytes (0)
 *	10nnnnnn	n + 1 bytes as in the row above
 *	110nnnnn d	n + 2 bytes as d + 1 bytes back
 *	111nnnnn d e	n + 3 bytes as 256 d + e + 1 bytes back
 * Finished rows go to the screen at once (pic_rows), 8 or more at a time.
 */
#include "archaic.h"

#ifdef NOPROTO
char *malloc();
#else
#include <stdlib.h>
#endif

static Image *im;
static unsigned char *op, *oend;	/* the next byte of the rows, their end */
static unsigned char *omark;		/* at op >= omark, new rows are whole */
static int oshown;			/* rows shown so far */

/* rows y up to y + n - 1 are done: count the bytes used, then show them */
void pic_rows(img, y, n)
Image *img;
int y, n;
{
	http_sync();
	img_rows(img, y, n);
}

/* n bytes at p to c, a long at a time. p is from malloc: longs fit there */
void pic_fill(p, n, c)
unsigned char *p;
register long n;
int c;
{
	register long *q = (long *)p;
	register long v;
	register unsigned char *b;
	unsigned long u;
	int i;

	c &= 255;
	for (u = (unsigned long)c, i = 1; i < (int)sizeof u; ++i)
		u = u << 8 | (unsigned long)c;
	v = (long)u;
	while (n >= 4 * (long)sizeof (long)) {
		*q++ = v;
		*q++ = v;
		*q++ = v;
		*q++ = v;
		n -= 4 * (long)sizeof (long);
	}
	for (b = (unsigned char *)q; n > 0; --n)
		*b++ = (unsigned char)c;
}

/* show the rows that are whole now; the next ones when 8 more are */
static void rowsout()
{
	int done = (int)((long)(op - im->bits) / im->bpl);

	if (done > oshown) {
		pic_rows(im, oshown, done - oshown);
		oshown = done;
	}
	omark = im->bits + (long)(oshown + 8) * im->bpl;
	if (omark > oend)
		omark = oend;
}

/* ABM's header, after "AB": white rows from malloc */
static int abmhead()
{
	static int h[8];
	long w, ht, n;
	int i;

	for (i = 0; i < 8; ++i)
		h[i] = HGETC();
	if (h[0] != 'M' || h[1] != '1' || h[6] != 1 || h[7] != 0)
		return -1;
	w = (long)h[2] << 8 | h[3];
	ht = (long)h[4] << 8 | h[5];
	if (w <= 0 || ht <= 0 || w > MAXW || ht > 8192)
		return -1;
	im->depth = 1;			/* also on a display with colors */
	im->pal = 0;
	im->w = (int)w;
	im->h = (int)ht;
	im->bpl = (int)((w + 7) / 8);
	n = (long)im->bpl * ht;
	if (n != (long)(unsigned)n)
		return -1;	/* too big for this machine's malloc */
	im->bits = (unsigned char *)malloc((unsigned)n);
	if (!im->bits)
		return -1;
	pic_wipe(im->bits, n);
	op = im->bits;
	oend = op + n;
	oshown = 0;
	omark = op;
	rowsout();
	return 0;
}

/* The body's bytes in registers: ip up to ie. IN(c) takes the next one
 * (-1 at the end of the body). INON() starts that, INOFF() ends it
 * (before a call that reads the body, or counts how much of it is
 * used). o is op in a register. */
#define INON() (ip = http_p, ie = http_e)
#define INOFF() (http_p = ip)
#define IN(c) { if (ip < ie) c = *ip++; else { http_p = ip; \
	c = http_more(); ip = http_p; ie = http_e; } }

/* ABM's tokens into the rows. 0 also when the body ends early: what
 * came is shown. -1: a token that can't be */
static int abmrows()
{
	register unsigned char *ip, *ie, *o, *s;
	register int c, n;
	long d, bpl = im->bpl;
	unsigned char *base = im->bits;
	int r = 0;

	INON();
	o = op;
	while (o < oend) {
		IN(c);
		if (c < 0)
			break;
		n = (c & 63) + 1;
		if (c < 0100) {			/* as they are */
			if (n > oend - o) {
				r = -1;
				break;
			}
			do {
				IN(c);
				if (c < 0)
					goto out;
				*o++ = c;
			} while (--n);
		} else if (c < 0200) {		/* white */
			if (n > oend - o) {
				r = -1;
				break;
			}
			do
				*o++ = 0;
			while (--n);
		} else {			/* as d bytes back */
			if (c < 0300)
				d = bpl;
			else if (c < 0340) {
				n = (c & 31) + 2;
				IN(c);
				d = (long)c + 1;
			} else {
				n = (c & 31) + 3;
				IN(c);
				if (c < 0)
					break;
				d = c;
				IN(c);
				d = (d << 8 | c) + 1;
			}
			if (c < 0)
				break;
			if (n > oend - o || d > o - base) {
				r = -1;
				break;
			}
			s = o - d;
			do
				*o++ = *s++;
			while (--n);
		}
		if (o >= omark) {
			INOFF();
			op = o;
			rowsout();
			INON();
		}
	}
out:
	INOFF();
	op = o;
	return r;
}

/* read a picture into img (bits and pal from malloc): 0 ok, -1 not a
 * picture Archaic knows, or broken. img->kind says what it was. */
int pic_load(img)
Image *img;
{
	int a, b, r;

	img->kind = 0;
	a = HGETC();
	b = HGETC();
	if (a == 'G' && b == 'I') {
		img->kind = P_GIF;
		return gif_load(img);
	}
	im = img;
	if (a != 'A' || b != 'B' || abmhead() < 0)
		return -1;
	im->kind = P_ABM;
	r = abmrows();
	omark = op;			/* the last rows, fewer than 8 */
	rowsout();
	return r;
}
