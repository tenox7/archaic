/* dmdterm.c - Archaic's part in the terminal: the AT&T DMD 5620 (the
 * Blit), in a layer of layers. Archaic itself runs on the UNIX host (a
 * 3B2) and draws through this program: see dmd.c, the host's side, for
 * the commands. This file is built with the DMD's compiler, not the
 * host's. mk3b2 then puts the program into the host's archaic (dmdm.h),
 * which loads it into its layer with 32ld:
 *	dmdcc -O -o archaic.m dmdterm.c
 *
 * The line from the host carries about 450 bytes a second. So:
 * - The page's picture (its slices) lives here, not on the host. The host
 *   passes WRP's ABM or ABM3 file on as it comes, and this program
 *   decodes it into its own memory (the decoders of pic.c). It draws the
 *   rows as they come, and it draws the frame again with no pixel from
 *   the host.
 * - A decoded slice is the layer's own bitmap format: rows of whole
 *   32-bit words, a lit pixel is 1. White is lit here, so the decoders
 *   store the pictures' bytes inverted (ABM: 1 is black). The picture's
 *   width must be a multiple of 32 (dmd.c asks WRP for one).
 * - The host sends only what changed: it sets a clip (K), and its
 *   commands then change only that part of the layer.
 * - The spinner turns here, with the terminal's clock, and the bar that
 *   shows how much of a file has come is drawn here, as its bytes come.
 * - Flow: for each 64 bytes it has read, the program sends 'a'. The host
 *   keeps at most 128 bytes on the way (the xt driver of UNIX System V
 *   Release 3.2 stops for good with more).
 *
 * The DMD's cc is the 3B2's K&R compiler. dmd.h makes macros of many
 * common names (min, max, sw, add, point, string ...): the names here
 * keep away from them. Tables with no values are not static, so that
 * they are in bss and do not come down the line with the program.
 */
#include <dmd.h>
#include <font.h>

#define NSL 5			/* slots: 4 slices of the page, and the test's */
#define TEST 4			/* the picture test's slot: never drawn */
#define MAXW 2048
#define FW 9			/* the font: defont, 9 x 14 */
#define FH 14
#define NTXT 255		/* letters in a text command, at most */

typedef struct slot {
	char *bits;		/* h rows of bpl bytes, from alloc: 0 is black */
	int w, h, bpl;
	int rows;		/* rows that are decoded */
	long y0;		/* the page row of its first row */
} Slot;

Slot slots[NSL];
Slot *sl;			/* the slot that a file comes into */
int vx, vy, vw, vh;		/* the frame, in the layer */
long voy;			/* the page row at its top */
int kx0, ky0, kx1, ky1;		/* the clip, for the host's commands */
int hx0, hy0, hx1, hy1;		/* the hole: no rows that come show here */
int nrcv;			/* bytes read since the last 'a' */
int npoll;			/* bytes read since events() */
int lastb = -1;			/* the mouse buttons as the host has them */
int lastx, lasty;		/* and the pointer's place */
long lastmt;			/* when it was sent: realtime() */
int spon, spx, spy, spf;	/* the spinner: on, where, its step */
long spt;			/* when it stepped: realtime(), 1/60 s */
int gx, gy, gw, gh;		/* the bar for the file that comes */
long gtot, ggot;		/* the file's bytes: all, and here */
int inslice;			/* a file comes: no new one inside it */
unsigned char fbuf[256];	/* a block of the file */
unsigned char *fp, *fe;		/* its bytes at hand */
int fend;			/* the file's end has come: 1 whole, 2 cut */
char tbuf[NTXT + 5];
Bitmap *tbm;			/* a text is drawn here first, then clipped */

#define OX (Drect.origin.x)
#define OY (Drect.origin.y)

void command();

/* white (lit) or black on the layer, with no clip: this program's own */
void box(x, y, w, h, black)
int x, y, w, h, black;
{
	if (w > 0 && h > 0)
		rectf(&display, Rect(OX + x, OY + y, OX + x + w, OY + y + h),
			black ? F_CLR : F_STORE);
}

/* the host's: inside the clip */
void cbox(x, y, w, h, f)
int x, y, w, h, f;
{
	int x1 = x + w, y1 = y + h;

	if (x < kx0)
		x = kx0;
	if (y < ky0)
		y = ky0;
	if (x1 > kx1)
		x1 = kx1;
	if (y1 > ky1)
		y1 = ky1;
	if (x1 > x && y1 > y)
		rectf(&display, Rect(OX + x, OY + y, OX + x1, OY + y1), f);
}

/* the bitmap b (w x h, from its 0, 0) to x, y of the layer, in the clip */
void cblt(b, x, y, w, h)
Bitmap *b;
int x, y, w, h;
{
	int x0 = x, y0 = y, x1 = x + w, y1 = y + h;

	if (x0 < kx0)
		x0 = kx0;
	if (y0 < ky0)
		y0 = ky0;
	if (x1 > kx1)
		x1 = kx1;
	if (y1 > ky1)
		y1 = ky1;
	if (x1 > x0 && y1 > y0)
		bitblt(b, Rect(x0 - x, y0 - y, x1 - x, y1 - y), &display,
			Pt(OX + x0, OY + y0), F_STORE);
}

void put2(v)
int v;
{
	sendchar(v >> 8);
	sendchar(v);
}

/* the layer's size to the host: at the start and after a reshape */
void size()
{
	sendchar('r');
	put2(Drect.corner.x - Drect.origin.x);
	put2(Drect.corner.y - Drect.origin.y);
}

/* all of the layer is white: the start, a new shape, and Z */
void clear()
{
	rectf(&display, Drect, F_STORE);
	kx0 = ky0 = 0;
	kx1 = ky1 = 32000;
	hx0 = hx1 = hy0 = hy1 = 0;
	vw = vh = 0;
	spon = 0;
}

/* the spinner: 8 dots in a circle, as main.c has them, and a larger
 * one that goes around */
int sdx[8] = { 6, 9, 10, 9, 6, 3, 2, 3 };
int sdy[8] = { 2, 3, 6, 9, 10, 9, 6, 3 };

void spin(on)
int on;
{
	int i;

	if (on && !spon)		/* it starts: the 8 dots */
		for (i = 0; i < 8; ++i)
			box(spx + sdx[i], spy + sdy[i], 1, 1, 1);
	if (spon) {			/* the large one is a dot again */
		box(spx + sdx[spf] - 1, spy + sdy[spf] - 1, 3, 3, 0);
		box(spx + sdx[spf], spy + sdy[spf], 1, 1, 1);
	}
	if (spon && !on)		/* it stops: all of it goes */
		box(spx, spy, 13, 13, 0);
	spon = on;
	if (!on)
		return;
	spf = (spf + 1) & 7;
	box(spx + sdx[spf] - 1, spy + sdy[spf] - 1, 3, 3, 1);
}

/* the bar: so much of the file is here. All of it is drawn each time:
 * the host does not know it, and may have drawn over a part of it. */
void gauge()
{
	int c;

	if (gtot <= 0 || gw <= 4)
		return;
	c = (int)((gw - 4) * ggot / gtot);
	if (c > gw - 4)
		c = gw - 4;
	box(gx, gy, gw, gh, 1);
	box(gx + 1, gy + 1, gw - 2, gh - 2, 0);
	box(gx + 2, gy + 2, c, gh - 4, 1);
}

void key(c)
int c;
{
	sendchar('k');
	sendchar(c);
}

/* what the user did goes to the host; the spinner's next step */
void events()
{
	int c, b, x, y;

	if (P->state & RESHAPED) {
		/* a new shape: the layer is empty, and the host draws it
		 * again. A layer that only moved has kept its pixels. */
		if (!(P->state & MOVED)) {
			clear();
			size();
		}
		P->state &= ~(RESHAPED | MOVED);
	}
	while ((c = kbdchar()) != -1) {
		/* an arrow key is ESC [ A (to D), all at once: 0360 to 0363 */
		if (c == 033 && (x = kbdchar()) != -1) {
			y = x == '[' ? kbdchar() : -1;
			if (y >= 'A' && y <= 'D')
				c = 0360 + y - 'A';
			else {
				key(033);
				if (y != -1) {
					key(x);
					c = y;
				} else
					c = x;
			}
		}
		key(c);
	}
	/* The buttons. A press outside the layer is not ours; one that
	 * began inside it stays ours until the button is let go. While a
	 * button is down, the host also gets the pointer's place when it
	 * moves (10 times a second at most): the scroll bar's elevator
	 * follows it. */
	b = 0;
	if (own() & MOUSE) {
		b = (button1() ? 1 : 0) | (button2() ? 2 : 0);
		if (lastb <= 0 && !ptinrect(mouse.xy, Drect))
			b = 0;
	}
	x = mouse.xy.x - OX;
	y = mouse.xy.y - OY;
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	c = b != lastb && (b || lastb > 0);
	if (!c && b && realtime() - lastmt >= 6 &&
		(y - lasty > 2 || lasty - y > 2 || x - lastx > 15 || lastx - x > 15))
		c = 1;
	if (c) {
		sendchar('m');
		sendchar(b);
		put2(x);
		put2(y);
		lastx = x;
		lasty = y;
		lastmt = realtime();
	}
	lastb = b;
	if (spon && realtime() - spt >= 8) {	/* 8 steps a second */
		spt = realtime();
		spin(1);
	}
}

/* the next byte from the host: waits for it, and looks at the keyboard
 * and the mouse meanwhile. While bytes keep coming (a slow decoder), it
 * still looks at them after each 32 bytes, and lets the terminal's other
 * programs run: a program that never waits locks them out. */
int rawc()
{
	int c;

	if (++npoll >= 32) {
		npoll = 0;
		events();
		wait(CPU);
	}
	while ((c = rcvchar()) == -1) {
		npoll = 0;
		events();
		alarm(2);
		wait(RCV | KBD | ALARM);
	}
	if (++nrcv >= 64) {
		nrcv = 0;
		sendchar('a');
	}
	return c & 0377;
}

int get2()
{
	int v = rawc();

	return v << 8 | rawc();
}

/* a place: it can be left of the layer or above it */
int get2s()
{
	int v = get2();

	return v & 0x8000 ? v - 0x10000 : v;
}

long get4()
{
	long v = get2();

	return v << 16 | (long)get2();
}

/* The frame's part x0 up to x1, rows a up to b - 1 (in the layer), from
 * the slots: white where no slice is. */
void fdraw(a, b, x0, x1)
int a, b, x0, x1;
{
	Slot *s, *f;
	Bitmap bm;
	long pr, e;
	int q, n, xe, y;

	if (a < vy)
		a = vy;
	if (b > vy + vh)
		b = vy + vh;
	if (x0 < vx)
		x0 = vx;
	if (x1 > vx + vw)
		x1 = vx + vw;
	if (x1 <= x0)
		return;
	while (a < b) {
		pr = voy + (a - vy);	/* the page row at a */
		q = b;
		f = 0;
		for (s = slots; s < slots + TEST; ++s) {
			if (!s->bits)
				continue;
			if (s->y0 <= pr && pr < s->y0 + s->h) {
				f = s;
				break;
			}
			e = vy + (s->y0 - voy);		/* where it starts */
			if (s->y0 > pr && e < q)
				q = (int)e;
		}
		if (!f) {
			box(x0, a, x1 - x0, q - a, 0);
			a = q;
			continue;
		}
		n = b - a;
		if (n > f->y0 + f->h - pr)
			n = (int)(f->y0 + f->h - pr);
		y = (int)(pr - f->y0);
		xe = vx + f->w < x1 ? vx + f->w : x1;
		if (xe > x0) {
			bm.base = (Word *)f->bits;
			bm.width = f->bpl / sizeof (Word);
			bm.rect = Rect(0, 0, f->w, f->h);
			bm._null = (char *)0;
			bitblt(&bm, Rect(x0 - vx, y, xe - vx, y + n), &display,
				Pt(OX + x0, OY + a), F_STORE);
		} else
			xe = x0;
		box(xe, a, x1 - xe, n, 0);
		a += n;
	}
}

/* rows a up to b - 1 of the frame, but not in the hole */
void fshow(a, b)
int a, b;
{
	int m0, m1;

	if (a < vy)
		a = vy;
	if (b > vy + vh)
		b = vy + vh;
	if (b <= a)
		return;
	if (hx1 <= hx0 || hy1 <= a || hy0 >= b) {
		fdraw(a, b, vx, vx + vw);
		return;
	}
	m0 = hy0 > a ? hy0 : a;
	m1 = hy1 < b ? hy1 : b;
	fdraw(a, m0, vx, vx + vw);
	fdraw(m1, b, vx, vx + vw);
	fdraw(m0, m1, vx, hx0);
	fdraw(m0, m1, hx1, vx + vw);
}

/* rows y up to y + n - 1 of the slice that comes are decoded: show
 * the ones in the frame */
void rowsdone(y, n)
int y, n;
{
	long a;

	sl->rows = y + n;
	if (sl == slots + TEST || vh <= 0)
		return;
	a = vy + (sl->y0 + y - voy);
	if (a + n <= vy || a >= vy + vh)
		return;
	fshow((int)a, (int)a + n);
}

/* the file's next byte, -1 at its end. Between the file's blocks other
 * commands can come (the frame moves, a status line): they are done. */
int tmore()
{
	int c, n;

	for (;;) {
		if (fend)
			return -1;
		c = rawc();
		if (c == 'D') {
			n = rawc();
			for (c = 0; c < n; ++c)
				fbuf[c] = rawc();
			fp = fbuf;
			fe = fbuf + n;
			ggot += n;
			gauge();
			if (n)
				return *fp++;
		} else if (c == 'E')
			fend = rawc() ? 1 : 2;
		else
			command(c);
	}
}

#define TGET() (fp < fe ? *fp++ : tmore())

/* n bytes at p to c */
void tfill(p, n, c)
register char *p;
register long n;
int c;
{
	while (--n >= 0)
		*p++ = c;
}

/* ABM packed as '1': tokens (see pic.c). The bytes are stored inverted. */
void abm1()
{
	register unsigned char *o, *s, *oe, *mark;
	register int c, n;
	long d;
	int shown = 0, done;

	o = (unsigned char *)sl->bits;
	oe = o + (long)sl->bpl * sl->h;
	mark = o + 8L * sl->bpl;
	while (o < oe) {
		c = TGET();
		if (c < 0)
			break;
		n = (c & 63) + 1;
		if (c < 0100) {			/* as they are */
			if (n > oe - o)
				break;
			do {
				c = TGET();
				if (c < 0)
					goto out;
				*o++ = c ^ 0377;
			} while (--n);
		} else if (c < 0200) {		/* white */
			if (n > oe - o)
				break;
			do
				*o++ = 0377;
			while (--n);
		} else {			/* as d bytes back */
			if (c < 0300)
				d = sl->bpl;
			else if (c < 0340) {
				n = (c & 31) + 2;
				d = (long)TGET() + 1;
			} else {
				n = (c & 31) + 3;
				d = TGET();
				d = (d << 8 | TGET()) + 1;
			}
			if (fend || n > oe - o || d > o - (unsigned char *)sl->bits)
				break;
			s = o - d;
			do
				*o++ = *s++;
			while (--n);
		}
		if (o >= mark) {
			done = (o - (unsigned char *)sl->bits) / sl->bpl;
			rowsdone(shown, done - shown);
			shown = done;
			mark = (unsigned char *)sl->bits + (long)(shown + 8) * sl->bpl;
		}
	}
out:
	done = (o - (unsigned char *)sl->bits) / sl->bpl;
	if (done > shown)
		rowsdone(shown, done - shown);
}

/* ABM packed as '3' (ABM3): the arithmetic coder and the model of pic.c,
 * where the comments are. The rows are stored inverted here. */
#define ABYTE 16384
#define AROW 16416
#define AROWW 16418
#define ANCTX 16419

unsigned short mqq[47] = {
	0x5601, 0x3401, 0x1801, 0x0ac1, 0x0521, 0x0221, 0x5601, 0x5401,
	0x4801, 0x3801, 0x3001, 0x2401, 0x1c01, 0x1601, 0x5601, 0x5401,
	0x5101, 0x4801, 0x3801, 0x3401, 0x3001, 0x2801, 0x2401, 0x2201,
	0x1c01, 0x1801, 0x1601, 0x1401, 0x1201, 0x1101, 0x0ac1, 0x09c1,
	0x08a1, 0x0521, 0x0441, 0x02a1, 0x0221, 0x0141, 0x0111, 0x0085,
	0x0049, 0x0025, 0x0015, 0x0009, 0x0005, 0x0001, 0x5601
};
unsigned char mqm[47] = {
	1, 2, 3, 4, 5, 38, 7, 8, 9, 10, 11, 12, 13, 29, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
	37, 38, 39, 40, 41, 42, 43, 44, 45, 45, 46
};
unsigned char mql[47] = {
	1, 6, 9, 12, 29, 33, 6, 14, 14, 14, 17, 18, 20, 21, 14, 14, 15, 16,
	17, 18, 19, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
	33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 46
};
unsigned long aql[94];
unsigned char anm[94], anl[94], ash[94], ast[ANCTX];
unsigned char alA[256];
unsigned long alB[256], alC[256];
unsigned char awhite[MAXW / 8 + 2], afa[MAXW / 8 + 2], afb[MAXW / 8 + 2];
unsigned short alist[MAXW / 8 + 2];
unsigned long aa, ac;
int act, aend, ainit;

void a3init()
{
	register int i, k;
	register unsigned long m;

	if (!ainit) {
		ainit = 1;
		for (i = 0; i < 47; ++i)
			for (k = 0; k < 2; ++k) {
				aql[2 * i + k] = (unsigned long)mqq[i] << 16;
				anm[2 * i + k] = 2 * mqm[i] + k;
				anl[2 * i + k] = 2 * mql[i] +
					(i == 0 || i == 6 || i == 14 ? 1 - k : k);
				for (m = mqq[i]; !(m & 0x8000); m <<= 1)
					++ash[2 * i + k];
			}
		/* the tables are for a byte as it is stored: inverted */
		for (i = 0; i < 256; ++i) {
			for (m = 0, k = 0; k < 8; ++k)
				if (!(i & 0x80 >> k))
					m |= 1L << 2 * k;
			alA[i] = (unsigned char)((m >> 12 & 5) << 4);
			alB[i] = m << 8;
			alC[i] = (m & 5) << 24;
		}
		tfill((char *)awhite, (long)sizeof awhite, 0377);
	}
	tfill((char *)ast, (long)ABYTE, 0);
	for (i = ABYTE; i < ANCTX; ++i)
		ast[i] = 1;
}

int amore()
{
	int b = tmore();

	if (b < 0) {
		b = 0;
		++aend;
	}
	return b;
}

#define ABYTEIN() { \
	c |= (unsigned long)(fp < fe ? *fp++ : amore()) << 8; \
	act = 8; \
}
#define ARENORM() do { \
	a <<= 1; \
	c <<= 1; \
	if (--act == 0) \
		ABYTEIN() \
} while (!(a & 0x80000000L))
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

int abit(sp)
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

int a3flags(cur, up, fa, fc, n)
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
		if (*up == 0377)
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

unsigned a3eight(r, o)
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

void a3pix(cur, up, up2, k, bpl, w)
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
			o |= ~cur[xb - 1] & 15;
		}
		if (xb + 1 < bpl)
			r |= alC[u[1]] | alC[v[1]] << 1;
		else if (w & 7) {		/* the row's last pixels */
			o = a3eight(r, o - 0x10 + (0x1000 >> (w & 7)));
			cur[xb] = ~((o & ((1 << (w & 7)) - 1)) << (8 - (w & 7)));
			continue;
		}
		cur[xb] = ~a3eight(r, o);
	} while (--k);
}

void abm3()
{
	register unsigned char *s, *d;
	unsigned char *cur, *up, *up2, *fa = afa, *fc = afb, *t;
	int y, k, bpl = sl->bpl, prev = 1, h = sl->h, shown = 0;

	a3init();
	tfill((char *)afa, (long)bpl + 2, 16);
	tfill((char *)afb, (long)bpl + 2, 16);
	aend = 0;
	for (ac = 0, k = 0; k < 3; ++k) {
		y = TGET();
		ac = ac << 8 | (y < 0 ? 0 : y);
	}
	ac <<= 8;
	aa = 0x80000000L;
	act = 8;
	for (y = 0; y < h && aend < 2; ++y) {
		cur = (unsigned char *)sl->bits + (long)y * bpl;
		up = y > 0 ? cur - bpl : awhite;
		up2 = y > 1 ? up - bpl : awhite;
		prev = abit(ast + AROW + prev);
		if (prev) {			/* as the row above */
			for (s = up, d = cur, k = bpl; --k >= 0;)
				*d++ = *s++;
			tfill((char *)fa + 1, (long)bpl, 16);
		} else if (abit(ast + AROWW))	/* white, as it is */
			tfill((char *)fa + 1, (long)bpl, 0);
		else {
			k = a3flags(cur, up, fa, fc, bpl);
			if (k)
				a3pix(cur, up, up2, k, bpl, sl->w);
			t = fa;
			fa = fc;
			fc = t;
		}
		if (y + 1 - shown >= 8 || y + 1 == h) {
			rowsdone(shown, y + 1 - shown);
			shown = y + 1;
		}
	}
}

void drop(s)
Slot *s;
{
	if (s->bits)
		free(s->bits);
	s->bits = (char *)0;
}

/* a file comes for slot i, the slice at page row y0: decode it, and tell
 * the host how it ended ('d', the slot, and 1 if all its rows are here,
 * 2 if there is no memory for them) */
void slice(i, y0)
int i;
long y0;
{
	static int h[10];
	long n;
	int k, pack = 0, ok = 0;

	fend = 0;
	fp = fe = fbuf;
	inslice = 1;
	ggot = 0;
	gauge();
	if (i >= NSL)
		i = TEST;
	sl = &slots[i];
	drop(sl);
	for (k = 0; k < 10; ++k)
		h[k] = TGET();
	sl->w = h[4] << 8 | h[5];
	sl->h = h[6] << 8 | h[7];
	sl->bpl = (sl->w + 7) / 8;
	sl->y0 = y0;
	sl->rows = 0;
	n = (long)sl->bpl * sl->h;
	if (h[0] == 'A' && h[1] == 'B' && h[2] == 'M' && h[8] == 1 && h[9] == 0 &&
		sl->w > 0 && sl->w <= MAXW && sl->h > 0 && !(sl->bpl & 3) && n < 400000L)
		pack = h[3];
	if (pack == '1' || pack == '3') {
		sl->bits = alloc((unsigned)n);
		if (!sl->bits)
			ok = 2;
		else {
			tfill(sl->bits, n, 0377);
			if (pack == '1')
				abm1();
			else
				abm3();
			ok = sl->rows == sl->h;
		}
	}
	while (!fend)			/* what is left of the file */
		tmore();
	if (fend == 2 && ok == 1)
		ok = 0;
	inslice = 0;
	if (gtot > 0)			/* the bar goes */
		box(gx, gy, gw, gh, 0);
	gtot = 0;
	sendchar('d');
	sendchar(i);
	sendchar(ok);
}

/* a small picture from the host: h rows of (w + 7) / 8 bytes, 1 is black.
 * per 0: all its rows come. Else only the first per rows come, and the
 * rows after them repeat them (a pattern). */
void bits(x, y, w, h, per)
int x, y, w, h, per;
{
	Bitmap *b;
	register char *p, *q;
	int i, k, bpl = (w + 7) / 8, c;

	b = w > 0 && h > 0 ? balloc(Rect(0, 0, w, h)) : (Bitmap *)0;
	for (i = 0; i < h; ++i) {
		p = b ? (char *)(b->base + (long)i * b->width) : tbuf;
		if (per && i >= per) {
			if (b) {
				q = (char *)(b->base + (long)(i - per) * b->width);
				for (k = 0; k < bpl; ++k)
					p[k] = q[k];
			}
			continue;
		}
		for (k = 0; k < bpl; ++k) {
			c = rawc();
			if (b)
				p[k] = c ^ 0377;
		}
	}
	if (!b)
		return;
	cblt(b, x, y, w, h);
	bfree(b);
}

/* a command from the host: see dmd.c */
void command(c)
int c;
{
	int x, y, w, h, n, k;

	switch (c) {
	case 'Z':
		clear();
		break;
	case 'K':
		kx0 = get2();
		ky0 = get2();
		kx1 = kx0 + get2();
		ky1 = ky0 + get2();
		break;
	case 'f':
		cbox(kx0, ky0, kx1 - kx0, ky1 - ky0, rawc() ? F_CLR : F_STORE);
		break;
	case 'i':
		cbox(kx0, ky0, kx1 - kx0, ky1 - ky0, F_XOR);
		break;
	case 'F':
	case 'I':
		x = get2();
		y = get2();
		w = get2();
		h = get2();
		cbox(x, y, w, h, c == 'I' ? F_XOR : rawc() ? F_CLR : F_STORE);
		break;
	case 'T':
		x = get2s();
		y = get2s();
		k = rawc();
		n = rawc();
		for (w = 0; w < n; ++w)
			tbuf[w] = rawc();
		tbuf[n] = 0;
		if (!tbm)
			break;
		rectf(tbm, Rect(0, 0, n * FW, FH), k ? F_CLR : F_STORE);
		string(&defont, tbuf, tbm, Pt(0, 0), k ? F_OR : F_CLR);
		cblt(tbm, x, y, n * FW, FH);
		break;
	case 'B':
	case 'P':
		x = get2s();
		y = get2s();
		w = get2();
		h = get2();
		bits(x, y, w, h, c == 'P' ? rawc() : 0);
		break;
	case 'R':
		fdraw(ky0, ky1, kx0, kx1);
		break;
	case 'C':
		x = get2();
		y = get2();
		w = get2();
		h = get2();
		n = get2s();
		bitblt(&display, Rect(OX + x, OY + y, OX + x + w, OY + y + h),
			&display, Pt(OX + x, OY + y + n), F_STORE);
		break;
	case 'V':
		vx = get2s();
		vy = get2s();
		vw = get2();
		vh = get2();
		voy = get4();
		break;
	case 'H':
		hx0 = get2();
		hy0 = get2();
		hx1 = hx0 + get2();
		hy1 = hy0 + get2();
		break;
	case 'N':
		x = rawc();
		if (inslice) {		/* not inside another one */
			get4();
			break;
		}
		slice(x, get4());
		break;
	case 'X':
		x = rawc();
		if (x < NSL && !(inslice && sl == &slots[x]))
			drop(&slots[x]);
		break;
	case 'W':
		k = rawc();
		x = get2();
		y = get2();
		spin(0);
		spx = x;
		spy = y;
		if (k)
			spin(1);
		break;
	case 'G':
		gx = get2();
		gy = get2();
		gw = get2();
		gh = get2();
		gtot = get4();
		ggot = 0;
		break;
	case 'D':			/* a block with no file: skip it */
		for (n = rawc(); n > 0; --n)
			rawc();
		break;
	case 'E':
		rawc();
		break;
	case 'Q':
		for (k = 0; k < NSL; ++k)
			drop(&slots[k]);
		sendchar('q');
		exit();
	}
}

main()
{
	request(RCV | SEND | KBD | MOUSE | ALARM);
	P->state &= ~(RESHAPED | MOVED);
	tbm = balloc(Rect(0, 0, NTXT * FW, FH));
	clear();
	size();
	for (;;)
		command(rawc());
}
