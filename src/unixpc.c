/* unixpc.c - Archaic's display for the window system of the AT&T UNIX PC
 * (3B1, PC 7300): 720x348, black and white. It uses tam, the window
 * library, which is in the shared library: link with -shlib (gcc).
 *
 * What is special there:
 * - Archaic gets a window of its own (/dev/window), as large as the
 *   window area allows: 686x248 inside the frame. -g WxH+X+Y gives
 *   another size. A size that is too large for a frame (wider than 686 or
 *   taller than 248) gets a window without a frame, up to 720x288.
 * - wrastop() draws. 1 is a lit pixel, and the low bit of each 16-bit
 *   word is the leftmost pixel. White is lit and black is dark, so the
 *   page and its pictures look as they should.
 * - Text is in the system font, /usr/lib/wfont/system.8.ft (9x12),
 *   drawn with wrastop() too.
 * - Keys come from wgetc() in keypad mode. When Archaic waits, a read
 *   waits at most 0.1 s (VMIN 0, VTIME 1); a poll does not wait (VTIME 0).
 *   Exit and Cancl (also the frame's close box) quit, Rfrsh draws again.
 *   Page, Shift+Page, Next, Prev and the Roll keys scroll. The scroll
 *   arrows in the frame send the Roll keys (so does FreeBee's wheel).
 * - A mouse click comes as the Mouse key, then wreadmouse() says where.
 *   The left button is 4, the middle one 2, the right one 1.
 * - The frame's resize box sends SIGWIND: then the new size is an E_SIZE.
 * - The help goes on the prompt line at the bottom of the screen.
 */
#include "archaic.h"
#include <fcntl.h>
#include <signal.h>
#include <termio.h>
#include <sys/types.h>
#include <sys/window.h>
#include <sys/font.h>

long lseek();

/* key codes from wgetc() in keypad mode (kcodes.h) */
#define KC_EXIT		0350
#define KC_SEXIT	0270
#define KC_CANCL	0370
#define KC_SCANCL	0310
#define KC_RFRSH	0362
#define KC_CLEAR	0302
#define KC_UP		0364
#define KC_DOWN		0365
#define KC_ROLLUP	0304
#define KC_ROLLDN	0305
#define KC_PAGE		0371
#define KC_SPAGE	0311
#define KC_NEXT		0373
#define KC_PREV		0355
#define KC_MOUSE	0375

#define FRAMEW 686		/* the largest inside of a window with a frame */
#define FRAMEH 248
#define SCRW 720		/* the window area: y 12 to 300 */
#define SCRY 12
#define SCRH 288

#define HELP "Click a link. Tab: next box. Page, Shift+Page: scroll. Exit: quit"

static int wn = -1;		/* the window */
static int ww, wh;		/* its inside */
static int fh;			/* text line height */
static int waiting = -1;	/* VTIME is 1 (1), 0 (0), not set yet (-1) */
static struct fntdef *font;
static struct termio tsave;
static int tsaved;
static int resized;		/* SIGWIND came */
static long fbuf[2048];		/* the font file (long: it must be even) */
static unsigned short lit[16] = {
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff
};
static unsigned short dark[16];
static unsigned short band[4096];	/* g_bits: 8192 bytes at a time */
static unsigned char rev[256];		/* a byte's bits the other way round */
static char lbuf[WTXTLEN];

/* wrastop() on the window, clipped to the inside */
static void rop(src, bpr, sx, sy, x, y, w, h, sop, dop, pat)
unsigned short *src, *pat;
int bpr, sx, sy, x, y, w, h, sop, dop;
{
	if (x < 0) {
		sx -= x;
		w += x;
		x = 0;
	}
	if (y < 0) {
		sy -= y;
		h += y;
		y = 0;
	}
	if (x + w > ww)
		w = ww - x;
	if (y + h > wh)
		h = wh - y;
	if (w <= 0 || h <= 0 || wn < 0)
		return;
	wrastop(wn, src, bpr, (unsigned short *)0, 0, sx, sy, x, y, w, h,
		sop, dop, pat);
}

static void bye(sig)
int sig;
{
	g_close();
	exit(1);
}

static void onwind(sig)
int sig;
{
	signal(SIGWIND, onwind);
	resized = 1;
}

/* the system font, read into fbuf */
static int loadfont()
{
	long size;
	int fd;

	fd = open("/usr/lib/wfont/system.8.ft", O_RDONLY);
	if (fd < 0)
		return -1;
	size = lseek(fd, 0L, 2);
	lseek(fd, 0L, 0);
	if (size < (long)sizeof (struct fntdef) || size > (long)sizeof fbuf ||
		read(fd, (char *)fbuf, (int)size) != size) {
		close(fd);
		return -1;
	}
	close(fd);
	font = (struct fntdef *)fbuf;
	return font->ff_magic == FMAGIC ? 0 : -1;
}

/* the font's picture of c: a space for a no-break space, ? for the
 * letters it does not have */
static struct fcdef *glyph(c)
int c;
{
	c &= 0377;
	if (c == 0240)
		c = ' ';
	if (c < FNTBASE || c >= FNTBASE + FNTSIZE)
		c = '?';
	return &font->ff_fc[c - FNTBASE];
}

/* a number in a geometry; 0 if there is none */
static int num(s, v)
char **s;
int *v;
{
	int n = 0, d = 0;

	while (**s >= '0' && **s <= '9') {
		n = n * 10 + *(*s)++ - '0';
		d = 1;
	}
	*v = n;
	return d;
}

/* a position for a window that is room pixels smaller than the window
 * area: in the middle if there is none (p < 0); neg counts from the far
 * edge */
static int place(p, neg, room)
int p, neg, room;
{
	if (p < 0)
		return room / 2;
	if (neg)
		p = room - p;
	if (p > room)
		p = room;
	if (p < 0)
		p = 0;
	return p;
}

int g_open(geom, w, h)
char *geom;
int *w, *h;
{
	struct uwdata uw;
	struct umdata um;
	struct termio t;
	int gw = FRAMEW, gh = FRAMEH, x = -1, y = -1, xneg = 0, yneg = 0;
	int frame, fw, fhh, i, j, b;
	char *s;

	if (geom) {
		s = geom;
		if (*s == '=')
			++s;
		if (num(&s, &i) && *s == 'x') {
			++s;
			if (num(&s, &j)) {
				gw = i;
				gh = j;
			}
		}
		if (*s == '+' || *s == '-') {
			xneg = *s++ == '-';
			if (!num(&s, &x))
				x = -1;
			if (*s == '+' || *s == '-') {
				yneg = *s++ == '-';
				if (!num(&s, &y))
					y = -1;
			}
		}
	}
	if (gw < 200)
		gw = 200;
	if (gh < 100)
		gh = 100;
	frame = gw <= FRAMEW && gh <= FRAMEH;
	if (gw > SCRW)
		gw = SCRW;
	if (gh > SCRH)
		gh = SCRH;
	/* the frame: 6 pixels at the left, 28 at the right (the scroll
	 * arrows), 20 at the top and at the bottom */
	fw = frame ? gw + 34 : gw;
	fhh = frame ? gh + 40 : gh;
	x = place(x, xneg, SCRW - fw);
	if (y >= 0 && !yneg)		/* +Y counts from the top of the screen */
		y = y < SCRY ? 0 : y - SCRY;
	y = SCRY + place(y, yneg, SCRH - fhh);

	/* a new window of our own, whatever started Archaic. A process
	 * group of our own too: the window sends SIGWIND to its group. */
	setpgrp();
	close(0);
	close(1);
	close(2);
	if (open("/dev/window", O_RDWR) != 0)
		return -1;
	dup(0);
	dup(0);
	winit();
	if (!iswind())
		return -1;
	wn = wcreate(1, 0, 5, 20, frame ?
		BORDCANCEL | BORDRESIZE | BORDVSCROLL : NBORDER);
	if (wn < 0)
		return -1;
	ioctl(wn, WIOCPGRP, 0);
	if (loadfont() < 0) {
		wprompt(wn, "archaic: no /usr/lib/wfont/system.8.ft");
		sleep(3);
		g_close();
		return -1;
	}
	ioctl(wn, WIOCGETD, &uw);
	uw.uw_x = x;
	uw.uw_y = y;
	uw.uw_width = gw;
	uw.uw_height = gh;
	ioctl(wn, WIOCSETD, &uw);
	ioctl(wn, WIOCGETD, &uw);
	ww = uw.uw_width;
	wh = uw.uw_height;
	wlabel(wn, "Archaic");
	wuser(wn, "Archaic");
	wprompt(wn, HELP);
	wprintf(wn, "\033[=1C");	/* no text cursor */
	keypad(wn, 1);
	ioctl(wn, TCGETA, &tsave);
	tsaved = 1;
	t = tsave;
	t.c_lflag &= ~(ICANON | ECHO);
	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = 1;
	ioctl(wn, TCSETA, &t);
	waiting = 1;
	um.um_flags = MSDOWN;		/* a click comes as the Mouse key */
	um.um_x = um.um_y = um.um_w = um.um_h = 0;
	um.um_icon = 0;
	wsetmouse(wn, &um);
	signal(SIGHUP, bye);
	signal(SIGINT, bye);
	signal(SIGQUIT, bye);
	signal(SIGTERM, bye);
	signal(SIGWIND, onwind);
	for (i = 0; i < 256; ++i) {
		for (b = j = 0; j < 8; ++j)
			if (i & (1 << j))
				b |= 0x80 >> j;
		rev[i] = b;
	}
	fh = font->ff_vs + 1;
	g_fill(0, 0, ww, wh, 0);
	*w = ww;
	*h = wh;
	return 0;
}

void g_close()
{
	if (wn < 0)
		return;
	if (tsaved)
		ioctl(wn, TCSETA, &tsave);
	wdelete(wn);
	wn = -1;
}

/* the label in the frame: as much as fits */
void g_title(s)
char *s;
{
	int n = ww / 9 - 4, i;

	if (n > WTXTLEN - 1)
		n = WTXTLEN - 1;
	for (i = 0; i < n && s[i]; ++i)
		lbuf[i] = (s[i] & 0377) < ' ' || (s[i] & 0377) > '~' ? ' ' : s[i];
	lbuf[i] = 0;
	wlabel(wn, lbuf);
}

/* c: 0 white (lit), 1 black (dark) */
void g_fill(x, y, w, h, c)
int x, y, w, h, c;
{
	rop((unsigned short *)0, 0, 0, 0, x, y, w, h, SRCPAT, DSTSRC,
		c ? dark : lit);
}

void g_invert(x, y, w, h)
int x, y, w, h;
{
	rop((unsigned short *)0, 0, 0, 0, x, y, w, h, SRCPAT, DSTXOR, lit);
}

int g_textw(s, n)
char *s;
int n;
{
	int w = 0;

	for (; n > 0; --n)
		w += glyph(*s++)->fc_hi;
	return w;
}

/* black on white, or white on black if inv. A letter's picture starts
 * fc_mr bytes after its own fc_mr field. */
void g_text(x, y, s, n, inv)
int x, y, n, inv;
char *s;
{
	struct fcdef *c;

	g_fill(x, y, g_textw(s, n), fh, inv);
	for (; n > 0; --n) {
		c = glyph(*s++);
		if (c->fc_hs > 0 && c->fc_vs > 0)
			rop((unsigned short *)((char *)&c->fc_mr + c->fc_mr),
				((c->fc_hs + 15) / 16) * 2, 0, 0,
				x + c->fc_ha, y + font->ff_baseline + c->fc_va,
				c->fc_hs, c->fc_vs, SRCSRC, inv ? DSTOR : DSTCAM,
				(unsigned short *)0);
		x += c->fc_hi;
	}
}

void g_font(height, base)
int *height, *base;
{
	*height = fh;
	*base = font->ff_baseline;
}

/* rows of 1-bit pixels, 1 black, the high bit on the left. In the
 * window 1 is lit and the low bit is on the left. */
void g_bits(x, y, w, h, bits, bpl)
int x, y, w, h, bpl;
unsigned char *bits;
{
	unsigned char *p;
	unsigned short *q;
	int spr = (w + 15) / 16, rows, n, r, k, i;

	rows = (int)(sizeof band / sizeof band[0]) / spr;
	while (h > 0) {
		n = h < rows ? h : rows;
		q = band;
		for (r = 0; r < n; ++r) {
			p = bits + (long)r * bpl;
			for (k = 0; k < spr; ++k) {
				i = 2 * k;
				if (i + 1 < bpl)
					*q++ = ~(rev[p[i]] | rev[p[i + 1]] << 8);
				else
					*q++ = ~rev[p[i]];
			}
		}
		rop(band, spr * 2, 0, 0, x, y, w, n, SRCSRC, DSTSRC,
			(unsigned short *)0);
		bits += (long)n * bpl;
		y += n;
		h -= n;
	}
}

void g_flush()
{
}

/* no file to select() on: ntwg.c waits in short steps instead */
int g_fd()
{
	return -1;
}

/* VTIME 1 (a read waits at most 0.1 s) or 0 (it does not wait) */
static void setwait(on)
int on;
{
	struct termio t;

	if (on == waiting)
		return;
	ioctl(wn, TCGETA, &t);
	t.c_cc[VTIME] = on ? 1 : 0;
	ioctl(wn, TCSETA, &t);
	waiting = on;
}

/* a click of the right button at the top or the bottom: main.c
 * scrolls up or down a page for it */
static int roll(e, down)
Event *e;
int down;
{
	e->type = E_DOWN;
	e->key = 3;
	e->x = 0;
	e->y = down ? wh - 30 : 0;
	return 1;
}

int g_event(e, wait)
Event *e;
int wait;
{
	struct uwdata uw;
	int k, x, y, b, r;

	setwait(wait);
	for (;;) {
		if (resized) {
			resized = 0;
			ioctl(wn, WIOCGETD, &uw);
			if (uw.uw_width != ww || uw.uw_height != wh) {
				e->type = E_SIZE;
				e->w = ww = uw.uw_width;
				e->h = wh = uw.uw_height;
				return 1;
			}
		}
		k = wgetc(wn);
		if (k < 0) {
			if (!wait)
				return 0;
			continue;
		}
		switch (k) {
		case KC_EXIT:
		case KC_SEXIT:
		case KC_CANCL:
		case KC_SCANCL:
			e->type = E_QUIT;
			return 1;
		case KC_RFRSH:
		case KC_CLEAR:
		case 014:			/* Ctrl-L */
			e->type = E_EXPOSE;
			e->x = e->y = 0;
			e->w = ww;
			e->h = wh;
			return 1;
		case KC_UP:
		case KC_ROLLUP:
		case KC_SPAGE:
		case KC_PREV:
			return roll(e, 0);
		case KC_DOWN:
		case KC_ROLLDN:
		case KC_PAGE:
		case KC_NEXT:
			return roll(e, 1);
		case KC_MOUSE:
			wreadmouse(wn, &x, &y, &b, &r);
			if (!(r & MSDOWN) || !b)
				break;
			e->type = E_DOWN;
			e->x = x;
			e->y = y;
			e->key = b & 4 ? 1 : b & 2 ? 2 : 3;
			return 1;
		default:
			if (k > 0 && k < 0200) {
				e->type = E_KEY;
				e->key = k;
				return 1;
			}
			break;
		}
	}
}
