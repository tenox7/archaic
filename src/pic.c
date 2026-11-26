/* pic.c - Archaic's pictures: what comes in the body, into an Image.
 *
 * pic_load() looks at the first bytes:
 *	"GI"	a GIF (gif.c)
 *	"ABM"	ABM: WRP's black and white picture, 1 bit a pixel
 *	"AB8"	AB8: WRP's picture in colors, a byte a pixel
 *
 * A GIF has a code for every few pixels, and an old CPU needs a long time
 * for that. ABM and AB8 are the display's own bytes, packed with a byte
 * LZ: no pixel is handled alone, so a 68000 decodes them many times
 * faster than a GIF. An ABM can also be packed for a slow line (ABM3):
 * much smaller, and slow to decode. Seconds for one picture of 632 x 351:
 *
 *			size	3B1 (10 MHz 68010)	Mac IIcx (16 MHz 68030)
 *	GIF, 2 colors	8.2 KB	1.55			0.53
 *	ABM		6.7 KB	0.20			0.05
 *	ABM3		4.1 KB	2.75			1.02
 *	GIF, 256 colors	40 KB	3			1.30
 *	AB8		36 KB				0.43
 *
 * Archaic's requests name what it takes (http_accept), and WRP sends the
 * smallest picture of the types that Archaic likes best. Archaic times
 * its pictures and asks for the kind that is faster here: see choice()
 * in main.c.
 *
 * ABM, the file:
 *	"ABM"		3 bytes
 *	how packed	1 byte: '1', or '3' (ABM3, see below)
 *	width, height	2 bytes each, the high byte first
 *	depth		1 byte: bits a pixel, 1
 *	flags		1 byte: 0
 *	the rows	packed. Each row is whole bytes; the high bit is the
 *			left pixel; 1 is black (as Image's bits for depth 1)
 * Packed as '1', the rows are tokens. A control byte, then:
 *	00nnnnnn	n + 1 bytes follow as they are
 *	01nnnnnn	n + 1 white bytes (0)
 *	10nnnnnn	n + 1 bytes as in the row above
 *	110nnnnn d	n + 2 bytes as d + 1 bytes back
 *	111nnnnn d e	n + 3 bytes as 256 d + e + 1 bytes back
 *
 * AB8, the file:
 *	"AB8"		3 bytes
 *	how packed	1 byte: '1'
 *	width, height	2 bytes each, the high byte first
 *	colors		1 byte: how many, less 1
 *	the colors	3 bytes each: red, green, blue
 *	the rows	packed. A byte for each pixel: its color's number
 * The rows are tokens. A control byte, then:
 *	00nnnnnn	n + 1 bytes follow as they are
 *	01nnnnnn	n + 1 times the byte before. n = 63: 2 bytes c
 *			follow (the high one first), 64 + c times
 *	10nnnnnn	n + 1 bytes as in the row above; n = 63 as for 01
 *	110nnnnn d	n + 2 bytes as d + 1 bytes back
 *	111nnnnn d e	n + 3 bytes as 256 d + e + 1 bytes back. n = 31: a
 *			byte b follows, 34 + b bytes
 *
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

/* n bytes at p to c, a long at a time (from the first place where a long
 * fits) */
void pic_fill(p, n, c)
unsigned char *p;
register long n;
int c;
{
	register long *q;
	register long v;
	register unsigned char *b;
	unsigned long u;
	int i;

	c &= 255;
	while (n > 0 && ((long)p & ((long)sizeof (long) - 1))) {
		*p++ = (unsigned char)c;
		--n;
	}
	q = (long *)p;
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

/* n bytes from s to d, the first one first: d comes after s, and the two
 * may lie over each other (a copy of bytes that came before). A long at
 * a time, when the two lie on a long's boundary together. */
void pic_copy(d, s, n)
register unsigned char *d, *s;
register long n;
{
	register long *p, *q;

	if (n >= 16 && d - s >= (long)sizeof (long) &&
		((d - s) & ((long)sizeof (long) - 1)) == 0) {
		while ((long)d & ((long)sizeof (long) - 1)) {
			*d++ = *s++;
			--n;
		}
		p = (long *)d;
		q = (long *)s;
		while (n >= 4 * (long)sizeof (long)) {
			*p++ = *q++;
			*p++ = *q++;
			*p++ = *q++;
			*p++ = *q++;
			n -= 4 * (long)sizeof (long);
		}
		while (n >= (long)sizeof (long)) {
			*p++ = *q++;
			n -= (long)sizeof (long);
		}
		d = (unsigned char *)p;
		s = (unsigned char *)q;
	}
	while (n >= 4) {
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		n -= 4;
	}
	while (--n >= 0)
		*d++ = *s++;
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

/* the rows of w x ht pixels, bpl bytes each, from malloc: 0, or -1 if
 * there is no room for them */
static int rowsnew(w, ht, bpl)
long w, ht;
int bpl;
{
	long n = (long)bpl * ht;

	if (n != (long)(unsigned)n)
		return -1;	/* too big for this machine's malloc */
	im->w = (int)w;
	im->h = (int)ht;
	im->bpl = bpl;
	im->bits = (unsigned char *)malloc((unsigned)n);
	if (!im->bits) {
		im->w = im->h = 0;
		return -1;
	}
	op = im->bits;
	oend = op + n;
	oshown = 0;
	omark = op;
	return 0;
}

/* ABM's header, after "ABM": how it is packed ('1' or '3'), or -1.
 * White rows from malloc */
static int abmhead()
{
	static int h[7];
	long w, ht;
	int i;

	for (i = 0; i < 7; ++i)
		h[i] = HGETC();
	if ((h[0] != '1' && h[0] != '3') || h[5] != 1 || h[6] != 0)
		return -1;
	w = (long)h[1] << 8 | h[2];
	ht = (long)h[3] << 8 | h[4];
	if (w <= 0 || ht <= 0 || w > MAXW || ht > 8192)
		return -1;
	im->depth = 1;			/* also on a display with colors */
	im->pal = 0;
	if (rowsnew(w, ht, (int)((w + 7) / 8)) < 0)
		return -1;
	pic_wipe(im->bits, (long)(oend - op));
	rowsout();
	return h[0];
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

/* ABM3: an ABM packed as '3', the dense black and white picture for a
 * line that is slower than the CPU. About 39% smaller than ABM packed as
 * '1'; the decoder works for each coded byte and pixel, so it is slow.
 *
 * The rows are bits from an adaptive binary arithmetic coder: the MQ
 * coder's 47 states (as in JBIG2 and JPEG 2000), with plain bytes (no
 * byte stuffing). A bit is coded in a context: a state of its own, which
 * learns how likely the bit is there. A context starts in state 0; the
 * flags' contexts start with 1 as the likely bit, the pixels' with 0.
 * The bits, for each row from the top:
 *	same row	1: the row is as the row above (the first row:
 *			white). Context: this flag of the row before (the
 *			first row: 1).
 *	white row	if not the same: 1: the row is all white.
 *	same bytes	if neither: for each byte of the row, 1: the byte is
 *			as the byte above. All of a row's flags come before
 *			its pixels. Context, 5 bits: this flag of the byte
 *			before (the first byte: 1), 2 if the byte above is
 *			white, and at 4, 8 and 16 this flag of the three
 *			bytes above: left of it, at it, right of it
 *			(outside the row: 1; after a same row all 1, after
 *			a white row all 0).
 *	pixels		then each pixel of the bytes that are not the same,
 *			from the left. Context, 14 bits: bits 0 to 3 are the
 *			4 pixels before it in its row (bit 0: the nearest).
 *			Then for the places x - 2 to x + 2: the pixel of the
 *			row above (bits 4, 6 ... 12) and of the row two up
 *			(bits 5, 7 ... 13). Outside the picture: white.
 * The file has all of the coder's bytes; the decoder reads at most 1 byte
 * more (as 0). When it wants more, the file was cut: what came is shown.
 *
 * The functions are small on purpose: an old compiler (gcc 2.5.8 on the
 * 3B1) then keeps what a loop needs in registers.
 */
#define ABYTE 16384		/* the contexts: 16384 of the pixels, then */
#define AROW 16416		/*  32 of the bytes, 2 of the rows, */
#define AROWW 16418		/*  the white row */
#define ANCTX 16419

static unsigned short mqq[47] = {	/* the states' LPS sizes */
	0x5601, 0x3401, 0x1801, 0x0ac1, 0x0521, 0x0221, 0x5601, 0x5401,
	0x4801, 0x3801, 0x3001, 0x2401, 0x1c01, 0x1601, 0x5601, 0x5401,
	0x5101, 0x4801, 0x3801, 0x3401, 0x3001, 0x2801, 0x2401, 0x2201,
	0x1c01, 0x1801, 0x1601, 0x1401, 0x1201, 0x1101, 0x0ac1, 0x09c1,
	0x08a1, 0x0521, 0x0441, 0x02a1, 0x0221, 0x0141, 0x0111, 0x0085,
	0x0049, 0x0025, 0x0015, 0x0009, 0x0005, 0x0001, 0x5601
};
static unsigned char mqm[47] = {	/* the next state after an MPS */
	1, 2, 3, 4, 5, 38, 7, 8, 9, 10, 11, 12, 13, 29, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
	37, 38, 39, 40, 41, 42, 43, 44, 45, 45, 46
};
static unsigned char mql[47] = {	/* and after an LPS */
	1, 6, 9, 12, 29, 33, 6, 14, 14, 14, 17, 18, 20, 21, 14, 14, 15, 16,
	17, 18, 19, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
	33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 46
};
/* a context's state is a byte: 2 * state + MPS. For such a byte: */
static unsigned long aql[94];		/* the LPS size, in the high half */
static unsigned char anm[94], anl[94];	/* the next byte after an MPS, an LPS */
static unsigned char ash[94];		/* the shifts that make the LPS size an interval */
static unsigned char ast[ANCTX];	/* the contexts */
/* A pixel's context is 14 bits. Bits 0 to 3: this row, x-1 first. Then
 * for x-2 to x+2 a bit of the row above (bits 4, 6 ... 12) and a bit of
 * the row two up (bits 5, 7 ... 13). For a byte of the row above, its
 * bits for the context of the first pixel of the byte after it (alA), of
 * its own place (alB, and the pixels to come above them), and of the
 * byte before it (alC); the next pixel's context is 2 bits down. */
static unsigned char alA[256];
static unsigned long alB[256], alC[256];
static unsigned char azero[MAXW / 8 + 2];	/* a white row */
static unsigned char afa[MAXW / 8 + 2], afb[MAXW / 8 + 2];	/* same flags: 16 or 0 */
static unsigned short alist[MAXW / 8 + 2];	/* the bytes of a row that are not the same */

/* the decoder between its functions: the high half of aa is the
 * interval (0x8000 and up), the high half of ac the code's place in it,
 * act the shifts until the next byte of the body goes into ac */
static unsigned long aa, ac;
static int act, aend;

static void abm3init()
{
	register int i, k;
	register unsigned long m;

	if (!aql[0])
		for (i = 0; i < 47; ++i)
			for (k = 0; k < 2; ++k) {
				aql[2 * i + k] = (unsigned long)mqq[i] << 16;
				anm[2 * i + k] = 2 * mqm[i] + k;
				anl[2 * i + k] = 2 * mql[i] +
					(i == 0 || i == 6 || i == 14 ? 1 - k : k);
				for (m = mqq[i]; !(m & 0x8000); m <<= 1)
					++ash[2 * i + k];
			}
	if (!alB[128])
		for (i = 0; i < 256; ++i) {
			for (m = 0, k = 0; k < 8; ++k)	/* pixel k at bit 2k */
				if (i & 0x80 >> k)
					m |= 1L << 2 * k;
			alA[i] = (unsigned char)((m >> 12 & 5) << 4);
			alB[i] = m << 8;
			alC[i] = (m & 5) << 24;
		}
	pic_fill(ast, (long)ABYTE, 0);
	for (i = ABYTE; i < ANCTX; ++i)
		ast[i] = 1;		/* the flags start as likely */
}

/* the body's next byte when http.c's window is empty: 0 after its end */
static int amore()
{
	int b = http_more();

	if (b < 0) {
		b = 0;
		++aend;
	}
	return b;
}

/* the next byte of the body goes into c's low half */
#define ABYTEIN() { \
	c |= (unsigned long)(http_p < http_e ? *http_p++ : amore()) << 8; \
	act = 8; \
}

/* the interval is too small: shift, a bit at a time (1 or 2 after an MPS) */
#define ARENORM() do { \
	a <<= 1; \
	c <<= 1; \
	if (--act == 0) \
		ABYTEIN() \
} while (!(a & 0x80000000L))

/* the same after an LPS: the interval is q, and ash has its shifts */
#define ALPSNORM() { \
	a = q; \
	s = ash[s]; \
	while ((int)s >= act) { \
		a <<= act; \
		c <<= act; \
		s -= act; \
		ABYTEIN() \
	} \
	a <<= s; \
	c <<= s; \
	act -= s; \
}

/* ADEC(ONE, ZERO): the next bit, in the context at sp: does ONE if it is
 * 1, ZERO if it is 0. s and q are for it; a and c are aa and ac. */
#define AMPS(ONE, ZERO) if (s & 1) { ONE; } else { ZERO; }
#define ALPS(ONE, ZERO) if (s & 1) { ZERO; } else { ONE; }
#define ADEC(ONE, ZERO) { \
	s = *sp; \
	q = aql[s]; \
	a -= q; \
	if (c < q) { \
		if (a < q) { \
			*sp = anm[s]; \
			AMPS(ONE, ZERO) \
		} else { \
			*sp = anl[s]; \
			ALPS(ONE, ZERO) \
		} \
		ALPSNORM() \
	} else { \
		c -= q; \
		if (a & 0x80000000L) { \
			AMPS(ONE, ZERO) \
		} else { \
			if (a < q) { \
				*sp = anl[s]; \
				ALPS(ONE, ZERO) \
			} else { \
				*sp = anm[s]; \
				AMPS(ONE, ZERO) \
			} \
			ARENORM(); \
		} \
	} }

/* one bit in the context at sp (the rows' flags) */
static int abit(sp)
register unsigned char *sp;
{
	register unsigned long a = aa, c = ac, q;
	register unsigned s;
	int bit;

	ADEC(bit = 1, bit = 0)
	aa = a;
	ac = c;
	return bit;
}

/* The same flags of a row's n bytes: into fc (fa has the row above's),
 * and the same bytes from up into cur. For each of the others, how many
 * bytes the row had left goes to alist: returns how many they are. */
static int abm3flags(cur, up, fa, fc, n)
register unsigned char *cur, *up, *fa, *fc;
register int n;
{
	register unsigned long a = aa, c = ac, q;
	register unsigned s;
	register int fw;
	register unsigned char *sp;
	unsigned char *pb = ast + ABYTE + 1;
	unsigned short *lp = alist;

	fw = fa[0] >> 1 | fa[1];
	fa += 2;
	do {
		fw = (fw >> 1 & 0x0c) | *fa++;
		sp = pb + fw;
		if (!*up)
			sp += 2;
		ADEC({ *++fc = 16; *cur = *up; pb = ast + ABYTE + 1; },
			{ *++fc = 0; *lp++ = n; pb = ast + ABYTE; })
		++cur;
		++up;
	} while (--n);
	aa = a;
	ac = c;
	return (int)(lp - alist);
}

/* The pixels of a byte. r has the two rows above: the context's bits 4
 * to 13, and above them the pixels to come. o has this row's 4 pixels
 * before, and a 1 that is at bit 12 when all the pixels are done.
 * Returns o: the pixels are its low bits. */
static unsigned abm3eight(r, o)
register unsigned long r;
register unsigned o;
{
	register unsigned long a = aa, c = ac, q;
	register unsigned s;
	register unsigned char *sp, *st = ast;

	do {
		sp = st + (((unsigned)r & 0x3ff0) | (o & 15));
		o <<= 1;
		ADEC(++o, ;)
		r >>= 2;
	} while (!(o & 0x1000));
	aa = a;
	ac = c;
	return o;
}

/* The pixels of a row's k bytes that are not the same (alist), into cur.
 * up is the row above, up2 the row two up; w pixels in bpl bytes */
static void abm3pix(cur, up, up2, k, bpl, w)
unsigned char *cur, *up, *up2;
int k, bpl, w;
{
	register unsigned char *u, *v;
	register unsigned short *lp = alist;
	register unsigned long r;
	register unsigned o;
	register int xb;

	do {
		xb = bpl - *lp++;
		u = up + xb;
		v = up2 + xb;
		r = alB[*u] | alB[*v] << 1;
		o = 0x10;
		if (xb > 0) {
			r |= (unsigned long)(alA[u[-1]] | alA[v[-1]] << 1);
			o |= cur[xb - 1] & 15;
		}
		if (xb + 1 < bpl)
			r |= alC[u[1]] | alC[v[1]] << 1;
		else if (w & 7) {		/* the row's last pixels */
			o = abm3eight(r, o - 0x10 + (0x1000 >> (w & 7)));
			cur[xb] = (o & ((1 << (w & 7)) - 1)) << (8 - (w & 7));
			continue;
		}
		cur[xb] = abm3eight(r, o);
	} while (--k);
}

/* ABM3's rows. 0 also when the body ends early: what came is shown. */
static int abm3rows()
{
	unsigned char *cur, *up, *up2, *fa = afa, *fc = afb, *t;
	int y, k, bpl = im->bpl, prev = 1, h = im->h;

	abm3init();
	pic_fill(afa, (long)bpl + 2, 16);
	pic_fill(afb, (long)bpl + 2, 16);
	aend = 0;
	for (ac = 0, k = 0; k < 3; ++k) {
		y = HGETC();
		ac = ac << 8 | (y < 0 ? 0 : y);
	}
	ac <<= 8;
	aa = 0x80000000L;
	act = 8;
	for (y = 0; y < h && aend < 2; ++y) {
		cur = im->bits + (long)y * bpl;
		up = y > 0 ? cur - bpl : azero;
		up2 = y > 1 ? up - bpl : azero;
		prev = abit(ast + AROW + prev);
		if (prev) {			/* as the row above */
			if (y > 0)
				pic_copy(cur, up, (long)bpl);
			pic_fill(fa + 1, (long)bpl, 16);
		} else if (abit(ast + AROWW))	/* white, as it is */
			pic_fill(fa + 1, (long)bpl, 0);
		else {
			k = abm3flags(cur, up, fa, fc, bpl);
			if (k)
				abm3pix(cur, up, up2, k, bpl, im->w);
			t = fa;
			fa = fc;
			fc = t;
		}
		op = cur + bpl;
		if (op >= omark)
			rowsout();
	}
	return 0;
}

/* AB8's tokens into the rows. 0 also when the body ends early: what
 * came is shown. -1: a token that can't be */
static int ab8rows()
{
	register unsigned char *ip, *ie, *o, *s;
	register int c;
	register long n;
	long d, bpl = im->bpl;
	unsigned char *base = im->bits;
	int r = 0;

	INON();
	o = op;
	while (o < oend) {
		IN(c);
		if (c < 0)
			break;
		if (c < 0100) {			/* as they are */
			n = c + 1;
			if (n > oend - o) {
				r = -1;
				break;
			}
			if (n <= ie - ip) {	/* all at hand */
				do
					*o++ = *ip++;
				while (--n);
			} else
				do {
					IN(c);
					if (c < 0)
						goto out;
					*o++ = c;
				} while (--n);
		} else {
			if (c < 0300) {		/* the byte before, the row above */
				n = (c & 63) + 1;
				d = c < 0200 ? 1 : bpl;
				if (n == 64) {
					IN(c);
					if (c < 0)
						break;
					n = c;
					IN(c);
					if (c < 0)
						break;
					n = (n << 8 | c) + 64;
				}
			} else if (c < 0340) {
				n = (c & 31) + 2;
				IN(c);
				if (c < 0)
					break;
				d = (long)c + 1;
			} else {
				n = (c & 31) + 3;
				IN(c);
				if (c < 0)
					break;
				d = c;
				IN(c);
				if (c < 0)
					break;
				d = (d << 8 | c) + 1;
				if (n == 34) {
					IN(c);
					if (c < 0)
						break;
					n = 34 + c;
				}
			}
			if (n > oend - o || d > o - base) {
				r = -1;
				break;
			}
			s = o - d;
			if (n >= 16) {		/* a long one */
				if (d == 1)
					pic_fill(o, n, *s);
				else
					pic_copy(o, s, n);
				o += n;
			} else {
				while (n >= 4) {
					*o++ = *s++;
					*o++ = *s++;
					*o++ = *s++;
					*o++ = *s++;
					n -= 4;
				}
				while (--n >= 0)
					*o++ = *s++;
			}
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

/* AB8's header, after "AB8": the colors, and rows from malloc */
static int ab8load()
{
	static int h[6];
	long w, ht;
	int i, k, nc, r, light = 0, lsum = -1;
	unsigned char *p;

	im->pal = 0;
	for (i = 0; i < 6; ++i)
		h[i] = HGETC();
	if (h[0] != '1' || h[5] < 0 || im->depth != 8)
		return -1;
	w = (long)h[1] << 8 | h[2];
	ht = (long)h[3] << 8 | h[4];
	nc = h[5] + 1;
	if (w <= 0 || ht <= 0 || w > MAXW || ht > 8192)
		return -1;
	im->pal = (unsigned char *)malloc(768);
	if (!im->pal || rowsnew(w, ht, (int)w) < 0)
		return -1;
	pic_wipe(im->pal, 768L);
	for (i = 0, p = im->pal; i < nc; ++i, p += 3) {
		for (k = 0; k < 3; ++k)
			p[k] = (unsigned char)HGETC();
		if (p[0] + p[1] + p[2] > lsum) {
			lsum = p[0] + p[1] + p[2];
			light = i;
		}
	}
	rowsout();
	r = ab8rows();
	if (op < oend)			/* the body ended early */
		pic_fill(op, (long)(oend - op), light);
	omark = op;
	rowsout();
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
	if (a != 'A' || b != 'B')
		return -1;
	a = HGETC();
	if (a == '8') {
		im->kind = P_AB8;
		return ab8load();
	}
	if (a != 'M' || (b = abmhead()) < 0)
		return -1;
	im->kind = b == '3' ? P_AB3 : P_ABM;
	r = b == '3' ? abm3rows() : abmrows();
	omark = op;			/* the last rows, fewer than 8 */
	rowsout();
	return r;
}
