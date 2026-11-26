/* dmd.c - Archaic's display on an AT&T DMD 5620 terminal (the Blit), in a
 * layer of layers, from a UNIX host: the AT&T 3B2 with UNIX System V
 * Release 3.2.
 *
 * Archaic runs on the host. The terminal is on a serial line, and has a
 * CPU and memory of its own. g_open() loads Archaic's program for the
 * terminal into the layer with 32ld, and then draws by commands to it.
 * That program is made from dmdterm.c, and a copy of it is in this one
 * (dmdm.h, which mk3b2 makes), so that archaic is one file. The line
 * carries about 450 bytes a second (19200 baud, less the xt protocol of
 * layers), so:
 *
 * - The page's picture is kept in the terminal, not here (REMOTE in
 *   main.c). g_slice() passes WRP's ABM or ABM3 file on as it comes, and
 *   the terminal decodes it, draws its rows, and scrolls by itself.
 *   g_view() tells it which rows of the page the frame shows. Archaic
 *   asks WRP for a picture whose width is a multiple of 32: the decoded
 *   rows are then the terminal's own bitmap.
 *
 * - Only what changes on the screen goes down the line. The g_ functions
 *   draw into a copy of the screen in memory here (nxt), and keep a list
 *   of what they drew. flush() compares nxt with the screen as the
 *   terminal has it (cur), and sends the list again only for the parts
 *   that are not the same: the terminal clips to such a part (K). So a
 *   new page with the same form costs a few bytes, and a typed letter
 *   about 30.
 *
 *   The copy has not the terminal's real pixels: it only has to be
 *   different where they are. A letter is a pattern of its own in its
 *   9 x 14 cell (the host does not have the terminal's font): each of
 *   its rows is not the same as that row of any other letter. A row of
 *   the frame is a number made from its page row. So a part that
 *   changed is made wider by a cell on each side, and a row of the
 *   frame changes as a whole.
 *
 *   While the line is busy, nothing new is put in its queue: what
 *   Archaic draws meanwhile goes as one change, when there is room. So
 *   keys typed fast do not wait for each other's pictures.
 *
 * - The spinner turns in the terminal (g_spin), and the terminal draws
 *   the bar that shows how much of a slice has come (g_gauge).
 *
 * - The scroll bar is Archaic's own (g_bar 1), drawn as all the rest.
 *   Its dotted shaft is rows that repeat: such a picture goes as a
 *   pattern (P), a few bytes. While a button is down, the terminal says
 *   where the pointer is when it moves (E_MOVE; only the last place
 *   waits in the queue): the elevator follows it. An arrow or the shaft
 *   held down repeats (g_event waits 1/10 s). While a part of the bar
 *   is held, main.c gets no slice (REMOTE): one takes 10 s and more.
 *
 * - Flow: at most FLY bytes are on the way. The terminal sends 'a' for
 *   each 64 bytes it has read. Without that, the xt driver stops for
 *   good after about 130 bytes.
 *
 * Commands to the terminal. A number is 2 bytes, the high one first; a
 * page row is 4 bytes. x and y are from the layer's top left corner.
 *	Z		all of the layer is white; no clip, hole or frame
 *	K x y w h	the clip: the commands f to R change only this part
 *	f c		fill the clip: c 0 white, 1 black
 *	F x y w h c	fill
 *	i		invert the clip
 *	I x y w h	invert
 *	T x y i n text	n characters at x, y; i 1: white on black
 *	B x y w h rows	a small picture: h rows of (w + 7) / 8 bytes, 1 black
 *	P x y w h n rows	the same, but only its first n rows come (1, 2 or
 *			4): the rows after them repeat them
 *	R		the frame (the part in the clip) again, from the slots
 *	C x y w h dy	move that part dy rows down (up if dy < 0); no clip
 *	V x y w h row	the frame is at x, y, w x h; its top row is this page
 *			row. Nothing is drawn
 *	H x y w h	the hole: rows of a slice that comes do not show here
 *			(an open list is there). w 0: no hole
 *	N s row		a file comes for slot s: the slice at this page row
 *	D n bytes	n bytes of the file (1 to 255)
 *	E ok		the file's end; ok 0: it was cut short
 *	X s		slot s is free
 *	W on x y	the spinner at x, y turns (1) or is gone (0)
 *	G x y w h n	a bar at x, y, w x h shows how much of the next file
 *			has come: the file has n bytes (4 bytes)
 *	Q		quit: the layer is a terminal again
 * From the terminal:
 *	a		64 more bytes are read
 *	k c		a key. 0360 to 0363: the arrows up, down, right, left
 *	m b x y		the mouse: b 1 = button 1 is down, 2 = button 2. With
 *			the same b as before: the pointer moved, a button down
 *	r w h		the layer's size: at the start, and after a reshape
 *	d s ok		the file for slot s is decoded; ok 1: all of it,
 *			2: the terminal has no memory for it
 *	q		the terminal's program has ended
 */
#include "archaic.h"
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <termio.h>
#include <poll.h>
#include <sys/types.h>
#undef VERSION			/* sys/sccsid.h has one of its own */
#include <sys/jioctl.h>

#include "dmdm.h"		/* termm, TERMN: the terminal's program */

char *getenv(), *malloc(), *memcpy(), *memset();
int memcmp();

#define FW 9			/* the terminal's font: 9 x 14 */
#define FH 14
#define FLY 128			/* bytes on the way, at most */
#define OBSZ 512
#define LOW 48			/* bytes in the queue: room for a change */
#define DBLK 96			/* bytes of a slice in a block */
#define ROWGAP 4		/* changed rows this near: one band */
#define NEV 128
#define NCMD 512		/* what was drawn since the last flush */
#define POOL 8192		/* its texts and small pictures */
#define BIGB 2048		/* a picture of more bytes goes in strips */
#define LAYW 2040		/* the layer's width, at most */
#define GAPB 8			/* changed parts this many bytes apart: one */

typedef struct cmd {
	char type;		/* F fill, I invert, T text, B bits, R frame */
	char arg;		/* F: 1 black. T: 1 white on black */
	short x, y, w, h;
	short n;		/* T: characters. B: bytes a row */
	short at;		/* T, B: where its bytes are in the pool */
} Cmd;

int g_bar = 1;			/* Archaic draws its own scroll bar */
static struct termio tsave;
static int israw;
static unsigned char ob[OBSZ];	/* for the terminal: oh up to on */
static int oh, on, unacked;
static Event evq[NEV];		/* what came from it */
static int evh, evn;
static int winw, winh;		/* the layer's size, as the terminal said */
static int rsz;			/* it is new: resync() makes the copies */
static int pst, pn, pv[5];	/* the parser: the event that comes, its bytes */
static int sdone = -1, sok;	/* 'd': the slot that is decoded, and how */
static int gone;		/* the terminal's program has ended */
static int spst = -1, spx, spy;	/* the spinner as the terminal has it */
static int gx, gy, gw, gh;	/* the bar for the next file (gw 0: none) */
static unsigned char *cur;	/* the screen as the terminal has it */
static unsigned char *nxt;	/* the screen as Archaic drew it */
static short *lo, *hi;		/* a row's first and last changed byte */
static int sw, sh, sbpl;	/* their size: 1 bit a pixel, 1 is black */
static unsigned short pat[96][FH];	/* a letter's pattern */
static Cmd cmds[NCMD];
static int ncmd, npool;
static unsigned char pool[POOL];
static int dy0, dy1;		/* the rows that the commands touched */
static int vx, vy, vw, vh;	/* the frame as the terminal has it */
static long voy = -1;
static long gen;		/* a new number when a slot went */
static int hx, hy, hw, hh;	/* the hole as the terminal has it */
static int logfd = -1;		/* $ARCHAICLOG: all bytes for the terminal */
static int errfd = -1;		/* standard error, while it is /dev/null */

static void nap(ms)
int ms;
{
	struct pollfd p;

	p.fd = -1;			/* no file: poll() only waits */
	p.events = 0;
	poll(&p, 1L, ms);
}

static void ev(type, x, y, w, h, key)
int type, x, y, w, h, key;
{
	Event *e;

	if (evn >= NEV)
		return;
	e = &evq[(evh + evn++) % NEV];
	e->type = type;
	e->x = x;
	e->y = y;
	e->w = w;
	e->h = h;
	e->key = key;
}

/* a byte from the terminal */
static void parse(c)
int c;
{
	static int lastb;
	int b, x, y;

	if (!pst) {
		if (c == 'a') {
			unacked -= 64;
			if (unacked < 0)
				unacked = 0;
		} else if (c == 'k' || c == 'm' || c == 'r' || c == 'd') {
			pst = c;
			pn = 0;
		} else if (c == 'q')
			gone = 1;
		return;
	}
	pv[pn++] = c;
	if (pn < (pst == 'k' ? 1 : pst == 'd' ? 2 : pst == 'r' ? 4 : 5))
		return;
	switch (pst) {
	case 'k':
		c = pv[0];
		if (c == 0360 || c == 0361)	/* the arrows up and down */
			ev(E_SCROLL, 0, c == 0360 ? -1 : 1, 0, 0, S_STEP);
		if (c == 0177)
			c = '\b';
		if (c < 0200)
			ev(E_KEY, 0, 0, 0, 0, c);
		break;
	case 'm':
		b = pv[0];
		x = pv[1] << 8 | pv[2];
		y = pv[3] << 8 | pv[4];
		if ((b & 1) != (lastb & 1))	/* button 1: the left one */
			ev(b & 1 ? E_DOWN : E_UP, x, y, 0, 0, 1);
		if ((b & 2) != (lastb & 2))	/* button 2 scrolls, as the right one */
			ev(b & 2 ? E_DOWN : E_UP, x, y, 0, 0, 3);
		if (b == lastb && b) {		/* it moved: only the last place counts */
			if (evn > 0 && evq[(evh + evn - 1) % NEV].type == E_MOVE)
				--evn;
			ev(E_MOVE, x, y, 0, 0, 0);
		}
		lastb = b;
		break;
	case 'r':
		/* a new shape: the layer is empty. resync() makes the copies
		 * for it; Archaic then draws all again */
		x = pv[0] << 8 | pv[1];
		y = pv[2] << 8 | pv[3];
		if (x > LAYW)
			x = LAYW;
		if (winw && (x != winw || y != winh))
			ev(E_SIZE, 0, 0, x, y, 0);
		else if (winw)
			ev(E_EXPOSE, 0, 0, x, y, 0);
		winw = x;
		winh = y;
		rsz = 1;
		break;
	case 'd':
		sdone = pv[0];
		sok = pv[1];
		break;
	}
	pst = 0;
}

/* send what may go now, and take what came */
static void pump()
{
	static unsigned char ib[64];
	int n, k;

	while (on > oh && unacked < FLY) {
		k = on - oh;
		if (k > FLY - unacked)
			k = FLY - unacked;
		n = write(1, (char *)ob + oh, (unsigned)k);
		if (n <= 0)
			break;
		if (logfd >= 0)
			write(logfd, (char *)ob + oh, (unsigned)n);
		oh += n;
		unacked += n;
	}
	if (oh == on)
		oh = on = 0;
	while ((n = read(0, (char *)ib, sizeof ib)) > 0)
		for (k = 0; k < n; ++k)
			parse(ib[k]);
}

static void put(c)
int c;
{
	int i;

	while (on >= OBSZ) {		/* full: wait for the line */
		if (oh > 0) {
			for (i = oh; i < on; ++i)
				ob[i - oh] = ob[i];
			on -= oh;
			oh = 0;
			continue;
		}
		pump();
		if (gone)
			return;
		if (on >= OBSZ && !oh)
			nap(20);
	}
	ob[on++] = c;
}

static void put2(v)
int v;
{
	put(v >> 8 & 255);
	put(v & 255);
}

static void put4(v)
long v;
{
	put2((int)(v >> 16 & 0xffff));
	put2((int)(v & 0xffff));
}

static void cmd4(c, x, y, w, h)
int c, x, y, w, h;
{
	put(c);
	put2(x);
	put2(y);
	put2(w);
	put2(h);
}

static void cook()
{
	if (!israw)
		return;
	israw = 0;
	ioctl(0, TCFLSH, 2);		/* never TCSETAW on an xt channel */
	ioctl(0, TCSETA, &tsave);
	if (errfd >= 0) {		/* standard error is the layer again */
		close(2);
		dup(errfd);
		close(errfd);
		errfd = -1;
	}
}

static void onsig(sig)
int sig;
{
	cook();
	_exit(1);
}

/* The copies of the screen: what the g_ functions draw into nxt. */

static void touch(y0, y1)
int y0, y1;
{
	if (y0 < dy0)
		dy0 = y0;
	if (y1 > dy1)
		dy1 = y1;
}

/* a part of nxt: op 0 white, 1 black, 2 inverted */
static void sfill(x, y, w, h, op)
int x, y, w, h, op;
{
	register unsigned char *p;
	register int i;
	int b0, b1, lm, rm;

	if (x < 0) {
		w += x;
		x = 0;
	}
	if (y < 0) {
		h += y;
		y = 0;
	}
	if (w > sw - x)
		w = sw - x;
	if (h > sh - y)
		h = sh - y;
	if (w <= 0 || h <= 0)
		return;
	touch(y, y + h);
	b0 = x >> 3;
	b1 = (x + w - 1) >> 3;
	lm = 0377 >> (x & 7);
	rm = 0377 << (7 - ((x + w - 1) & 7)) & 0377;
	if (b0 == b1)
		lm &= rm;
	for (; h > 0; --h, ++y) {
		p = nxt + (long)y * sbpl + b0;
		if (op == 0)
			*p &= ~lm;
		else if (op == 1)
			*p |= lm;
		else
			*p ^= lm;
		if (b1 == b0)
			continue;
		++p;
		i = b1 - b0 - 1;
		if (i > 0 && op < 2) {
			memset((char *)p, op ? 0377 : 0, i);
			p += i;
		} else
			for (; i > 0; --i, ++p)
				*p ^= 0377;
		if (op == 0)
			*p &= ~rm;
		else if (op == 1)
			*p |= rm;
		else
			*p ^= rm;
	}
}

/* The letters' patterns: 9 bits a row. The space is white, as it is on
 * the terminal. Each row of another letter is 1 to 509: not white, not
 * black, and not that row of any other letter (509 is a prime). So when
 * a letter changes, all 14 rows of its cell change. */
static void patinit()
{
	int c, j;

	for (c = 1; c < 96; ++c)
		for (j = 0; j < FH; ++j)
			pat[c][j] = (unsigned short)((c * (37 + 2 * j) + 101 * j) % 509 + 1);
}

/* n letters at x, y. A letter that is not all inside the layer's width
 * is left out, here and in replay() */
static void stext(x, y, s, n, inv)
int x, y, n, inv;
unsigned char *s;
{
	register unsigned char *p;
	register unsigned v, m;
	int i, j, c, sft;

	touch(y < 0 ? 0 : y, y + FH > sh ? sh : y + FH);
	for (i = 0; i < n; ++i, x += FW) {
		if (x < 0 || x + FW > sw)
			continue;
		c = s[i] > ' ' && s[i] < 0177 ? s[i] - ' ' : 0;
		sft = 7 - (x & 7);	/* 9 pixels in 2 bytes */
		m = 0777 << sft;
		for (j = 0; j < FH; ++j) {
			if (y + j < 0 || y + j >= sh)
				continue;
			v = pat[c][j];
			if (inv)
				v ^= 0777;
			v <<= sft;
			p = nxt + (long)(y + j) * sbpl + (x >> 3);
			p[0] = (p[0] & ~(m >> 8)) | (v >> 8);
			p[1] = (p[1] & ~(m & 0377)) | (v & 0377);
		}
	}
}

/* 8 pixels at x, y: the bits m of v */
static void sbyte(x, y, v, m)
int x, y, v, m;
{
	register unsigned char *p;
	int o;

	v &= m;
	if (x < 0 || x + 8 > sw) {	/* at the edge: a pixel at a time */
		for (o = 0; o < 8; ++o) {
			if (!(m & 0200 >> o) || x + o < 0 || x + o >= sw)
				continue;
			p = nxt + (long)y * sbpl + ((x + o) >> 3);
			if (v & 0200 >> o)
				*p |= 0200 >> ((x + o) & 7);
			else
				*p &= ~(0200 >> ((x + o) & 7));
		}
		return;
	}
	p = nxt + (long)y * sbpl + (x >> 3);
	o = x & 7;
	p[0] = (p[0] & ~(m >> o)) | (v >> o);
	if (o)
		p[1] = (p[1] & ~(m << (8 - o) & 0377)) | (v << (8 - o) & 0377);
}

/* a picture of w x h pixels at x, y: rows of bpl bytes, 1 is black */
static void sbits(x, y, w, h, bits, bpl)
int x, y, w, h, bpl;
unsigned char *bits;
{
	int i, k, n = (w + 7) / 8;
	int last = w & 7 ? 0377 << (8 - (w & 7)) & 0377 : 0377;

	touch(y < 0 ? 0 : y, y + h > sh ? sh : y + h);
	for (i = 0; i < h; ++i) {
		if (y + i < 0 || y + i >= sh)
			continue;
		for (k = 0; k < n; ++k)
			sbyte(x + 8 * k, y + i, bits[(long)i * bpl + k],
				k == n - 1 ? last : 0377);
	}
}

/* rows a up to b - 1 of the frame: each gets a number of its own, made
 * from its page row, in all of its bytes */
static void sview(a, b)
int a, b;
{
	static unsigned char id[4];
	register unsigned char *p;
	unsigned long v;
	int y, x0 = vx, x1 = vx + vw, b0, b1, lm, rm, i, n, k;

	if (x0 < 0)
		x0 = 0;
	if (x1 > sw)
		x1 = sw;
	if (a < vy)
		a = vy;
	if (b > vy + vh)
		b = vy + vh;
	if (a < 0)
		a = 0;
	if (b > sh)
		b = sh;
	if (x1 <= x0 || b <= a)
		return;
	touch(a, b);
	b0 = x0 >> 3;
	b1 = (x1 - 1) >> 3;
	lm = 0377 >> (x0 & 7);
	rm = 0377 << (7 - ((x1 - 1) & 7)) & 0377;
	if (b0 == b1)
		lm &= rm;
	for (y = a; y < b; ++y) {
		v = (unsigned long)(voy + (y - vy) + 1) * (unsigned long)0x9e3779b1L +
			(unsigned long)gen * (unsigned long)0x01000193L;
		id[0] = (unsigned char)(v >> 24);
		id[1] = (unsigned char)(v >> 16);
		id[2] = (unsigned char)(v >> 8);
		id[3] = (unsigned char)v;
		p = nxt + (long)y * sbpl;
		p[b0] = (p[b0] & ~lm) | (id[b0 & 3] & lm);
		if (b1 == b0)
			continue;
		n = b1 - b0 - 1;	/* the whole bytes between them */
		for (i = 0; i < n && i < 4; ++i)
			p[b0 + 1 + i] = id[(b0 + 1 + i) & 3];
		for (k = 4; k < n; k += i) {	/* then twice as many each time */
			i = k < n - k ? k : n - k;
			memcpy((char *)p + b0 + 1 + k, (char *)p + b0 + 1, i);
		}
		p[b1] = (p[b1] & ~rm) | (id[b1 & 3] & rm);
	}
}

/* The list of what was drawn goes again to the part x, y, w x h of the
 * terminal's screen: the terminal clips to it. */
static void replay(rx, ry, rw, rh)
int rx, ry, rw, rh;
{
	Cmd *c;
	unsigned char *d;
	int x0, y0, x1, y1, a, b, i, k, n, per;

	cmd4('K', rx, ry, rw, rh);
	for (c = cmds; c < cmds + ncmd; ++c) {
		x0 = c->x > rx ? c->x : rx;
		y0 = c->y > ry ? c->y : ry;
		x1 = c->x + c->w < rx + rw ? c->x + c->w : rx + rw;
		y1 = c->y + c->h < ry + rh ? c->y + c->h : ry + rh;
		if (x1 <= x0 || y1 <= y0)
			continue;
		switch (c->type) {
		case 'F':
		case 'I':
			if (x0 == rx && y0 == ry && x1 == rx + rw && y1 == ry + rh)
				put(c->type == 'F' ? 'f' : 'i');
			else
				cmd4(c->type, x0, y0, x1 - x0, y1 - y0);
			if (c->type == 'F')
				put(c->arg);
			break;
		case 'T':
			a = (x0 - c->x) / FW;
			b = (x1 - c->x + FW - 1) / FW;
			while (a < b && c->x + a * FW < 0)
				++a;
			while (b > a && c->x + b * FW > sw)
				--b;
			if (b <= a)
				break;
			put('T');
			put2(c->x + a * FW);
			put2(c->y);
			put(c->arg);
			put(b - a);
			for (d = pool + c->at + a; a < b; ++a)
				put(*d++);
			break;
		case 'B':
			a = (x0 - c->x) / 8;	/* its bytes a up to b - 1 */
			b = (x1 - c->x + 7) / 8;
			if (b > c->n)
				b = c->n;
			k = c->w - 8 * a < 8 * (b - a) ? c->w - 8 * a : 8 * (b - a);
			/* rows that repeat after 1, 2 or 4 rows (the scroll
			 * bar's shaft): only the first ones go */
			n = y1 - y0;
			d = pool + c->at + (long)(y0 - c->y) * c->n + a;
			for (per = 1; per <= 4; per *= 2) {
				for (i = per; i < n && memcmp((char *)d + (long)i * c->n,
					(char *)d + (long)(i - per) * c->n, b - a) == 0; ++i)
					;
				if (i >= n)
					break;
			}
			if (per > 4 || n < 2 * per + 2)
				per = 0;
			cmd4(per ? 'P' : 'B', c->x + 8 * a, y0, k, n);
			if (per)
				put(per);
			for (i = 0; i < (per ? per : n); ++i)
				for (k = 0; k < b - a; ++k)
					put(d[(long)i * c->n + k]);
			break;
		case 'R':
			put('R');
			break;
		}
	}
}

/* Send what changed. The rows that are not the same in cur and nxt make
 * bands; a band's changed bytes make parts; each part is a cell wider
 * on both sides, because a letter's pattern can be the same as another
 * one's in some of its columns (not in a row: see patinit). The parts
 * do not overlap (an invert must come once). */
static void flush()
{
	static unsigned char colm[LAYW / 8 + 2];
	register unsigned char *p, *q;
	register int i;
	int y, k, ys, ye, b0, b1, ry, rh, x0, x1;

	if (!ncmd || !cur) {
		ncmd = npool = 0;
		dy0 = sh;
		dy1 = 0;
		return;
	}
	if (dy0 < 0)
		dy0 = 0;
	if (dy1 > sh)
		dy1 = sh;
	for (y = dy0; y < dy1; ++y) {
		p = cur + (long)y * sbpl;
		q = nxt + (long)y * sbpl;
		if (memcmp((char *)p, (char *)q, sbpl) == 0) {
			lo[y] = -1;
			continue;
		}
		for (i = 0; p[i] == q[i]; ++i)
			;
		lo[y] = i;
		for (i = sbpl - 1; p[i] == q[i]; --i)
			;
		hi[y] = i;
	}
	for (y = dy0; y < dy1;) {
		if (lo[y] < 0) {
			++y;
			continue;
		}
		ys = y;			/* a band: changed rows near each other */
		ye = y + 1;
		for (k = y + 1; k < dy1 && k <= ye + ROWGAP; ++k)
			if (lo[k] >= 0)
				ye = k + 1;
		memset((char *)colm, 0, sbpl);
		for (k = ys; k < ye; ++k) {
			if (lo[k] < 0)
				continue;
			p = cur + (long)k * sbpl;
			q = nxt + (long)k * sbpl;
			for (i = lo[k]; i <= hi[k]; ++i)
				if (p[i] != q[i])
					colm[i] = 1;
		}
		/* a row of the frame changes as a whole */
		if (vw > 0 && ye > vy && ys < vy + vh) {
			b0 = vx < 0 ? 0 : vx >> 3;
			b1 = vx + vw > sw ? sbpl - 1 : (vx + vw - 1) >> 3;
			for (i = b0; i <= b1 && !colm[i]; ++i)
				;
			if (i <= b1)
				memset((char *)colm + b0, 1, b1 - b0 + 1);
		}
		ry = ys;
		rh = ye - ys;
		for (i = 0; i < sbpl;) {
			if (!colm[i]) {
				++i;
				continue;
			}
			b0 = b1 = i;
			for (k = i + 1; k < sbpl && k <= b1 + GAPB; ++k)
				if (colm[k])
					b1 = k;
			i = b1 + 1;
			x0 = b0 > 0 ? (b0 - 1) * 8 : 0;
			x1 = (b1 + 2) * 8 > sw ? sw : (b1 + 2) * 8;
			replay(x0, ry, x1 - x0, rh);
		}
		y = ye;
	}
	for (y = dy0; y < dy1; ++y)	/* now the terminal has it */
		if (lo[y] >= 0)
			memcpy((char *)cur + (long)y * sbpl + lo[y],
				(char *)nxt + (long)y * sbpl + lo[y], hi[y] - lo[y] + 1);
	ncmd = npool = 0;
	dy0 = sh;
	dy1 = 0;
}

/* the layer has a new shape (or the first one): new copies, all white,
 * as the layer is after Z */
static void resync()
{
	long n;

	if (!rsz)
		return;
	rsz = 0;
	if (cur)
		free((char *)cur);
	if (nxt)
		free((char *)nxt);
	if (lo)
		free((char *)lo);
	if (hi)
		free((char *)hi);
	sw = winw;
	sh = winh;
	sbpl = (sw + 7) / 8 + 1;	/* a byte more: stext writes 2 */
	n = (long)sbpl * sh;
	cur = (unsigned char *)malloc((unsigned)n);
	nxt = (unsigned char *)malloc((unsigned)n);
	lo = (short *)malloc((unsigned)(sh * sizeof (short)));
	hi = (short *)malloc((unsigned)(sh * sizeof (short)));
	if (!cur || !nxt || !lo || !hi) {
		cook();
		fprintf(stderr, "archaic: no memory for a layer of %d x %d\n", sw, sh);
		exit(1);
	}
	memset((char *)cur, 0, (int)n);
	memset((char *)nxt, 0, (int)n);
	ncmd = npool = 0;
	dy0 = sh;
	dy1 = 0;
	spst = -1;
	vw = hw = 0;
	voy = -1;
	put('Z');
}

/* A fill, a picture or the frame covers x, y, w x h: what the list has
 * inside it does not show any more, and goes, with its bytes in the
 * pool. So when the line is busy and Archaic draws a field or the scroll
 * bar ten times, the list has only the last one. */
static void cover(x, y, w, h)
int x, y, w, h;
{
	Cmd *c, *d;
	int n, i, at = 0;

	if (w < 8 || h < 8)
		return;
	for (c = d = cmds; c < cmds + ncmd; ++c) {
		if (c->x >= x && c->y >= y && c->x + c->w <= x + w &&
			c->y + c->h <= y + h)
			continue;
		n = c->type == 'T' ? c->n : c->type == 'B' ? c->n * c->h : 0;
		if (n && c->at != at)		/* its bytes move down */
			for (i = 0; i < n; ++i)
				pool[at + i] = pool[c->at + i];
		if (d != c)
			memcpy((char *)d, (char *)c, sizeof (Cmd));
		d->at = at;
		at += n;
		++d;
	}
	ncmd = d - cmds;
	npool = at;
}

/* a place in the list for a command, and n bytes in the pool */
static Cmd *newcmd(type, x, y, w, h, n)
int type, x, y, w, h, n;
{
	Cmd *c;

	if (ncmd >= NCMD || npool + n > POOL)
		flush();
	c = &cmds[ncmd++];
	c->type = type;
	c->arg = 0;
	c->x = x;
	c->y = y;
	c->w = w;
	c->h = h;
	c->n = 0;
	c->at = npool;
	npool += n;
	return c;
}

/* Run 32ld with the terminal's program: 0 if it loaded. The program is
 * the copy in this one, in a file for 32ld; or the file that $ARCHAICM
 * names (to try another dmdterm.c). */
static int load()
{
	static char ld[200], tmp[40];
	char *dmd = getenv("DMD"), *path = getenv("ARCHAICM");
	int pid, w, fd, st = -1;

	if (!path || !path[0]) {
		sprintf(tmp, "/tmp/arcm%d", getpid());
		path = tmp;
		fd = creat(tmp, 0600);
		if (fd < 0 || write(fd, (char *)termm, (unsigned)TERMN) != TERMN) {
			if (fd >= 0)
				close(fd);
			unlink(tmp);
			return -1;
		}
		close(fd);
	}
	sprintf(ld, "%.150s/bin/32ld", dmd && dmd[0] ? dmd : "/usr/dmd");
	pid = fork();
	if (pid == 0) {
		execl(ld, "32ld", path, (char *)0);
		_exit(127);
	}
	while (pid > 0 && (w = wait(&st)) != pid && w >= 0)
		;
	if (tmp[0])
		unlink(tmp);
	return pid > 0 ? st : -1;
}

int g_open(geom, w, h)
char *geom;
int *w, *h;
{
	struct termio t;
	char *p;
	int i;

	if (ioctl(0, JMPX, 0) == -1) {
		fprintf(stderr, "archaic: this is no window of layers on a DMD 5620 terminal.\n");
		fprintf(stderr, "archaic: start layers, make a window, and start archaic in it\n");
		return -1;
	}
	patinit();
	if ((p = getenv("ARCHAICLOG")) != 0 && p[0])
		logfd = creat(p, 0644);
	/* raw before 32ld: the terminal's bytes must not meet erase, kill
	 * or the signal keys. 32ld keeps the modes it finds. */
	ioctl(0, TCGETA, &tsave);
	t = tsave;
	t.c_iflag = IGNBRK;
	t.c_oflag = 0;
	t.c_lflag = 0;
	t.c_cflag = (tsave.c_cflag & (CBAUD | CLOCAL)) | CS8 | CREAD;
	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = 0;
	ioctl(0, TCSETAW, &t);
	israw = 1;
	/* Standard error is the layer too: a message on it would be
	 * commands for the terminal. It goes to /dev/null while the layer
	 * is Archaic's (not if it is a file: archaic -v 2>log). */
	if (isatty(2) && (errfd = dup(2)) >= 0) {
		close(2);
		if (open("/dev/null", 1) != 2) {
			close(2);
			dup(errfd);
			close(errfd);
			errfd = -1;
		}
	}
	signal(SIGHUP, onsig);
	signal(SIGINT, onsig);
	signal(SIGQUIT, onsig);
	signal(SIGTERM, onsig);
	if (load() != 0) {
		cook();
		fprintf(stderr, "archaic: 32ld could not load Archaic's part into the terminal\n");
		return -1;
	}
	for (i = 0; i < 300 && !winw; ++i) {	/* its first word: the size */
		pump();
		if (!winw)
			nap(100);
	}
	if (!winw) {
		cook();
		fprintf(stderr, "archaic: no answer from the terminal\n");
		return -1;
	}
	resync();
	*w = winw;
	*h = winh;
	return 0;
}

void g_close()
{
	int i;

	if (!israw)
		return;
	put('Q');
	for (i = 0; i < 100 && !gone; ++i) {
		pump();
		nap(50);
	}
	cook();
}

void g_title(s)
char *s;
{
}

/* what was drawn goes to the terminal, if the line has room for it now.
 * If not, it waits: more can come, and it then goes as one change. */
static void later()
{
	pump();
	if (on - oh < LOW)
		flush();
}

void g_flush()
{
	resync();
	later();
	pump();
}

void g_fill(x, y, w, h, c)
int x, y, w, h, c;
{
	Cmd *p;

	if (w <= 0 || h <= 0 || !nxt)
		return;
	cover(x, y, w, h);
	p = newcmd('F', x, y, w, h, 0);
	p->arg = c ? 1 : 0;
	sfill(x, y, w, h, c ? 1 : 0);
}

void g_invert(x, y, w, h)
int x, y, w, h;
{
	if (w <= 0 || h <= 0 || !nxt)
		return;
	newcmd('I', x, y, w, h, 0);
	sfill(x, y, w, h, 2);
}

void g_text(x, y, s, n, inv)
int x, y, n, inv;
char *s;
{
	Cmd *p;
	unsigned char *d;
	int i, k;

	for (; n > 0 && nxt; n -= k, s += k, x += k * FW) {
		k = n > 250 ? 250 : n;
		p = newcmd('T', x, y, k * FW, FH, k);
		p->arg = inv ? 1 : 0;
		p->n = k;
		d = pool + p->at;
		for (i = 0; i < k; ++i)
			d[i] = s[i] >= ' ' && s[i] < 0177 ? s[i] : '?';
		stext(x, y, d, k, inv);
	}
}

int g_textw(s, n)
char *s;
int n;
{
	return n * FW;
}

void g_font(height, base)
int *height, *base;
{
	*height = FH;
	*base = 11;
}

void g_bits(x, y, w, h, bits, bpl)
int x, y, w, h, bpl;
unsigned char *bits;
{
	Cmd *p;
	unsigned char *d;
	int i, k, n = (w + 7) / 8;

	if (w <= 0 || h <= 0 || !nxt)
		return;
	cover(x, y, w, h);
	if ((long)n * h > BIGB) {
		/* a large picture goes in strips: the list has no room for
		 * all of it */
		k = BIGB / n > 0 ? BIGB / n : 1;
		for (i = 0; i < h; i += k)
			g_bits(x, y + i, w, h - i < k ? h - i : k,
				bits + (long)i * bpl, bpl);
		return;
	}
	p = newcmd('B', x, y, w, h, n * h);
	p->n = n;
	d = pool + p->at;
	for (i = 0; i < h; ++i)
		for (k = 0; k < n; ++k)
			*d++ = bits[(long)i * bpl + k];
	sbits(x, y, w, h, pool + p->at, n);
}

long g_colors()
{
	return 2;
}

long g_ms()
{
	return -1;
}

void g_pix(x, y, w, h, pix, bpl, pal)
int x, y, w, h, bpl;
unsigned char *pix, *pal;
{
}

/* a row's bytes b0 up to b1 from row ys to row yd of the copy s */
static void rowcp(s, ys, yd, b0, b1, lm, rm)
unsigned char *s;
int ys, yd, b0, b1, lm, rm;
{
	unsigned char *a = s + (long)ys * sbpl, *d = s + (long)yd * sbpl;

	d[b0] = (d[b0] & ~lm) | (a[b0] & lm);
	if (b1 == b0)
		return;
	if (b1 - b0 > 1)
		memcpy((char *)d + b0 + 1, (char *)a + b0 + 1, b1 - b0 - 1);
	d[b1] = (d[b1] & ~rm) | (a[b1] & rm);
}

/* The terminal moves its own pixels: what was drawn goes there first,
 * then the two copies move the same way. */
int g_copy(x, y, w, h, dy)
int x, y, w, h, dy;
{
	int i, b0, b1, lm, rm;

	if (!nxt || x < 0 || y < 0 || w <= 0 || h <= 0 || x + w > sw ||
		y + h > sh || y + dy < 0 || y + dy + h > sh)
		return 0;
	flush();
	cmd4('C', x, y, w, h);
	put2(dy & 0xffff);
	b0 = x >> 3;
	b1 = (x + w - 1) >> 3;
	lm = 0377 >> (x & 7);
	rm = 0377 << (7 - ((x + w - 1) & 7)) & 0377;
	if (b0 == b1)
		lm &= rm;
	if (dy < 0)
		for (i = 0; i < h; ++i) {
			rowcp(cur, y + i, y + i + dy, b0, b1, lm, rm);
			rowcp(nxt, y + i, y + i + dy, b0, b1, lm, rm);
		}
	else
		for (i = h - 1; i >= 0; --i) {
			rowcp(cur, y + i, y + i + dy, b0, b1, lm, rm);
			rowcp(nxt, y + i, y + i + dy, b0, b1, lm, rm);
		}
	return 1;
}

void g_sbar(pos, max, vis)
long pos, max, vis;
{
}

int g_fd()
{
	return -1;
}

int g_event(e, wait)
Event *e;
int wait;
{
	int t = 0;

	resync();
	for (;;) {
		later();
		resync();	/* a new shape: the copies first, then its event */
		if (gone && !evn) {
			e->type = E_QUIT;
			return 1;
		}
		if (evn) {
			*e = evq[evh];
			evh = (evh + 1) % NEV;
			--evn;
			return 1;
		}
		if (!wait || (wait == 2 && t >= 100))
			return 0;
		nap(30);
		t += 30;
	}
}

/* the spinner turns at x, y (on), or is gone */
void g_spin(on, x, y)
int on, x, y;
{
	on = on ? 1 : 0;
	if (on == spst && (!on || (x == spx && y == spy)))
		return;
	spst = on;
	spx = x;
	spy = y;
	put('W');
	put(on);
	put2(x);
	put2(y);
}

/* the next g_slice: the terminal shows how much of it has come, in a
 * bar at x, y, w x h */
void g_gauge(x, y, w, h)
int x, y, w, h;
{
	gx = x;
	gy = y;
	gw = w;
	gh = h;
}

/* rows of a slice that comes do not show at x, y, w x h; w 0: no hole */
void g_hole(x, y, w, h)
int x, y, w, h;
{
	if (w <= 0 || h <= 0)
		x = y = w = h = 0;
	if (x == hx && y == hy && w == hw && h == hh)
		return;
	hx = x;
	hy = y;
	hw = w;
	hh = h;
	cmd4('H', x, y, w, h);
}

/* the frame is at x, y, w x h, and its top row is the page's row oy:
 * its rows a up to b - 1 are drawn again, from the terminal's slots */
void g_view(x, y, w, h, oy, a, b)
int x, y, w, h, a, b;
long oy;
{
	if (!nxt)
		return;
	if (x != vx || y != vy || w != vw || h != vh || oy != voy) {
		vx = x;
		vy = y;
		vw = w;
		vh = h;
		voy = oy;
		cmd4('V', x, y, w, h);
		put4(oy);
	}
	if (a < y)
		a = y;
	if (b > y + h)
		b = y + h;
	if (b <= a)
		return;
	cover(x, a, w, b - a);
	newcmd('R', x, a, w, b - a, 0);
	sview(a, b);
}

/* the terminal forgets slot s: what the frame shows of it is old */
void g_drop(s)
int s;
{
	put('X');
	put(s);
	++gen;
}

/* The body that comes now is the slice at page row y0: pass it on to
 * the terminal, for its slot s. im gets the picture's size and kind (it
 * has no bits here). 0 when the terminal has decoded all of it; -1 if
 * it was stopped, or it came short; -2 if it is no ABM that the terminal
 * can keep; -3 if the terminal has no memory for it. */
int g_slice(s, y0, im)
int s;
long y0;
Image *im;
{
	static unsigned char hd[10];
	long t;
	int i, c, n, ok = 1;

	im->bits = im->pal = 0;
	im->depth = 1;
	im->kind = 0;
	for (i = 0; i < 10; ++i) {
		c = HGETC();
		if (c < 0)
			return -1;
		hd[i] = c;
	}
	if (hd[0] != 'A' || hd[1] != 'B' || hd[2] != 'M' ||
		(hd[3] != '1' && hd[3] != '3') || hd[8] != 1)
		return -2;
	im->w = hd[4] << 8 | hd[5];
	im->h = hd[6] << 8 | hd[7];
	im->bpl = (im->w + 7) / 8;
	im->kind = hd[3] == '3' ? P_AB3 : P_ABM;
	if (im->w & 31)			/* not the terminal's bitmap */
		return -2;
	resync();
	flush();			/* the screen first, then its rows */
	sdone = -1;
	if (gw > 4 && gh > 4 && http_len > 0) {
		cmd4('G', gx, gy, gw, gh);
		put4(http_len);
	}
	gw = 0;
	put('N');
	put(s);
	put4(y0);
	put('D');
	put(10);
	for (i = 0; i < 10; ++i)
		put(hd[i]);
	for (;;) {
		c = HGETC();
		if (c < 0)
			break;
		flush();		/* what Archaic drew meanwhile */
		n = (int)(http_e - http_p);
		if (n > DBLK - 1)
			n = DBLK - 1;
		put('D');
		put(n + 1);
		put(c);
		for (i = 0; i < n; ++i)
			put(*http_p++);
		http_sync();
		pump();
		if (ui_poll() || gone) {
			ok = 0;
			break;
		}
	}
	if (http_len > 0 && http_got < http_len)
		ok = 0;
	put('E');
	put(ok);
	/* the terminal says when it has decoded it: that is its time */
	for (t = 0; sdone != s && !gone && t < 120000L; t += 20) {
		pump();
		if (sdone == s)
			break;
		if (ok && ui_poll()) {
			ok = 0;			/* stopped: it stays in the slot */
			break;
		}
		nap(20);
	}
	if (sdone == s && sok == 2)
		return -3;
	return ok && sdone == s && sok == 1 ? 0 : -1;
}
