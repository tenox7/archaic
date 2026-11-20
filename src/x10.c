/* x10.c - Archaic's display for X10 (X Version 10 Release 4), as on
 * Whitesmiths Idris for the Atari ST: 640x400, black and white.
 *
 * What is special there:
 * - the server says the screen is 0 x 0: then 640 x 400 is used
 * - the keyboard goes to the window under the mouse pointer, so the
 *   pointer is moved into the window when it opens
 * - XBitmapBitsPut takes rows of 16-bit shorts, the low bit on the left
 * - a resized window comes as ExposeWindow with the new size
 * - the ST's left mouse button comes as MiddleButton (1), the right one
 *   as RightButton (0)
 * - no scroll bars: Archaic draws its own (g_bar). No motion events
 *   are asked for: the elevator moves where the button is let go, as in
 *   GEM. X10 can't wait for a time, so wait 2 waits as 1: held arrows
 *   don't repeat.
 * - $ARCHAICDBG set: every X event is printed to stderr
 */
#include "archaic.h"
#include <X/Xlib.h>

#ifdef NOPROTO
char *getenv();
#else
#include <stdlib.h>
#endif

static Display *dpy;
static Window win;
static FontInfo *font;
static int ww, wh;
static short band[1024];		/* 2048 bytes of bits at a time */
static unsigned char rev[256];		/* a byte's bits the other way round */
static char gbuf[64];

int g_bar = 1;

void g_sbar(pos, max, vis)
long pos, max, vis;
{
}

/* black and white only: no colors, so Archaic never calls g_pix */
long g_colors()
{
	return 2L;
}

void g_pix(x, y, w, h, pix, bpl, pal)
int x, y, w, h, bpl;
unsigned char *pix, *pal;
{
}

/* The pointer: an arrow with its tip, the hot spot, at 0,0.
 * - The Idris server draws the root window's X cursor 7 pixels up and
 *   left of where it clicks (it seems to take the hot spot off twice).
 *   A hot spot of 0,0 is drawn where it clicks either way.
 * - It also draws cursor bit 1 white and the rest of the mask black,
 *   whatever colors it gets. So the bits here are the arrow's white
 *   edge, and the black arrow is the mask without them. */
static char *arrow[] = {
	"X",
	"XX",
	"XXX",
	"XXXX",
	"XXXXX",
	"XXXXXX",
	"XXXXXXX",
	"XXXXXXXX",
	"XXXXX",
	"XX XX",
	"X   XX",
	"    XX",
	"     XX",
	"     XX",
	0
};
static short body[16], cbits[16], mbits[16];

static void mkcursor()
{
	Cursor c;
	int x, y, dx, dy;

	for (y = 0; arrow[y]; ++y)
		for (x = 0; arrow[y][x]; ++x) {
			if (arrow[y][x] != 'X')
				continue;
			body[y] |= 1 << x;
			/* the mask: the arrow and an edge around it */
			for (dy = -1; dy <= 1; ++dy)
				for (dx = -1; dx <= 1; ++dx)
					if (y + dy >= 0 && y + dy < 16 &&
						x + dx >= 0 && x + dx < 16)
						mbits[y + dy] |= 1 << (x + dx);
		}
	for (y = 0; y < 16; ++y)
		cbits[y] = mbits[y] & ~body[y];
	c = XCreateCursor(16, 16, cbits, mbits, 0, 0, BlackPixel, WhitePixel,
		GXcopy);
	if (c)
		XDefineCursor(win, c);
}

int g_open(geom, w, h)
char *geom;
int *w, *h;
{
	int sw, sh, x = 0, y = 0, gw, gh, m = 0, i, j, b;

	dpy = XOpenDisplay((char *)0);
	if (!dpy)
		dpy = XOpenDisplay("unix:0");
	if (!dpy)
		return -1;
	sw = dpy->width;
	sh = dpy->height;
	if (sw <= 0 || sh <= 0) {
		sw = 640;
		sh = 400;
	}
	gw = sw * 9 / 10;		/* a bit smaller than the screen, */
	gh = sh * 9 / 10;		/* so the desktop shows around it */
	if (geom) {
		gbuf[0] = '=';
		scopy(gbuf + 1, geom[0] == '=' ? geom + 1 : geom, (int)sizeof gbuf - 1);
		m = XParseGeometry(gbuf, &x, &y, &gw, &gh);
		if ((m & XValue) && (m & XNegative))
			x += sw - gw - 2;
		if ((m & YValue) && (m & YNegative))
			y += sh - gh - 2;
	}
	if (gw < 200)
		gw = 200;
	if (gh < 100)
		gh = 100;
	if (!(m & XValue))		/* no place given: the middle */
		x = (sw - gw - 2) / 2;
	if (!(m & YValue))
		y = (sh - gh - 2) / 2;
	if (x > sw - gw - 2)
		x = sw - gw - 2;
	if (y > sh - gh - 2)
		y = sh - gh - 2;
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	font = XOpenFont("6x13");
	if (!font)
		font = XOpenFont("6x10");
	if (!font)
		return -1;
	win = XCreateWindow(RootWindow, x, y, gw, gh, 1, BlackPixmap, WhitePixmap);
	if (!win)
		return -1;
	XStoreName(win, "Archaic");
	mkcursor();
	XSelectInput(win, KeyPressed | ButtonPressed | ButtonReleased |
		ExposeWindow | ExposeRegion);
	XMapWindow(win);
	XWarpMouse(win, gw / 2, gh / 2);
	for (i = 0; i < 256; ++i) {
		for (b = j = 0; j < 8; ++j)
			if (i & (1 << j))
				b |= 0x80 >> j;
		rev[i] = b;
	}
	*w = ww = gw;
	*h = wh = gh;
	return 0;
}

/* No XCloseDisplay: on Idris it needs sigsetmask, which no library has.
 * The connection closes when the program ends. */
void g_close()
{
	if (dpy)
		XFlush();
	dpy = 0;
}

void g_title(s)
char *s;
{
	XStoreName(win, s);
}

void g_fill(x, y, w, h, c)
int x, y, w, h, c;
{
	XPixSet(win, x, y, w, h, c ? BlackPixel : WhitePixel);
}

void g_invert(x, y, w, h)
int x, y, w, h;
{
	XPixFill(win, x, y, w, h, BlackPixel, (Bitmap)0, GXinvert, AllPlanes);
}

void g_text(x, y, s, n, inv)
int x, y, n, inv;
char *s;
{
	XText(win, x, y, s, n, font->id, inv ? WhitePixel : BlackPixel,
		inv ? BlackPixel : WhitePixel);
}

int g_textw(s, n)
char *s;
int n;
{
	int w = 0, c;

	if (font->fixedwidth || !font->widths)
		return n * font->width;
	for (; n > 0; --n) {
		c = *s++ & 0377;
		if (c >= font->firstchar && c <= font->lastchar)
			w += font->widths[c - font->firstchar];
	}
	return w;
}

void g_font(height, base)
int *height, *base;
{
	*height = font->height;
	*base = font->baseline;
}

/* rows of 1-bit pixels, 1 black, the high bit on the left */
void g_bits(x, y, w, h, bits, bpl)
int x, y, w, h, bpl;
unsigned char *bits;
{
	unsigned char *p;
	short *q;
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
					*q++ = rev[p[i]] | rev[p[i + 1]] << 8;
				else
					*q++ = rev[p[i]];
			}
		}
		XBitmapBitsPut(win, x, y, w, n, band, BlackPixel, WhitePixel,
			(Bitmap)0, GXcopy, AllPlanes);
		bits += (long)n * bpl;
		y += n;
		h -= n;
	}
}

void g_flush()
{
	XFlush();
}

int g_fd()
{
	return dpy ? dpyno() : -1;
}

/* no clock: the spinner steps each time the network waits */
long g_ms()
{
	return -1L;
}

/* $ARCHAICDBG set: each X event to stderr */
static void trace(ev, s, n)
XEvent *ev;
char *s;
int n;
{
	static int on = -1;
	XKeyOrButtonEvent *k = (XKeyOrButtonEvent *)ev;
	int i;

	if (on < 0)
		on = getenv("ARCHAICDBG") != 0;
	if (!on)
		return;
	fprintf(stderr, "x10: type %lx detail %x x %d y %d", (long)ev->type,
		k->detail & 0xffff, k->x, k->y);
	for (i = 0; s && i < n; ++i)
		fprintf(stderr, " %02x", s[i] & 0377);
	fprintf(stderr, "\n");
}

int g_event(e, wait)
Event *e;
int wait;
{
	static XEvent ev;
	XKeyOrButtonEvent *k;
	XExposeEvent *x;
	char *s;
	int n;

	for (;;) {
		if (!wait && !XPending())
			return 0;
		XNextEvent(&ev);
		s = 0;
		n = 0;
		if (ev.type == KeyPressed)
			s = XLookupMapping((XKeyEvent *)&ev, &n);
		trace(&ev, s, n);
		switch ((int)ev.type) {
		case KeyPressed:
			/* function keys come as ESC sequences; the arrow keys
			 * all come as Ctrl-Space (0) */
			if (s && n >= 1 && s[0] && !(n > 1 && s[0] == 033)) {
				e->type = E_KEY;
				e->key = s[0] & 0377;
				return 1;
			}
			break;
		case ButtonPressed:
		case ButtonReleased:
			k = (XKeyOrButtonEvent *)&ev;
			e->type = ev.type == ButtonPressed ? E_DOWN : E_UP;
			e->x = k->x;
			e->y = k->y;
			switch (k->detail & ValueMask) {
			case LeftButton:
				e->key = 1;
				break;
			case MiddleButton:
				e->key = 2;
				break;
			default:
				e->key = 3;
				break;
			}
			return 1;
		case ExposeWindow:
			x = (XExposeEvent *)&ev;
			if (x->width > 0 && x->height > 0 &&
				(x->width != ww || x->height != wh)) {
				e->type = E_SIZE;
				e->w = ww = x->width;
				e->h = wh = x->height;
				return 1;
			}
			e->type = E_EXPOSE;
			e->x = e->y = 0;
			e->w = ww;
			e->h = wh;
			return 1;
		case ExposeRegion:
			x = (XExposeEvent *)&ev;
			e->type = E_EXPOSE;
			e->x = x->x;
			e->y = x->y;
			e->w = x->width;
			e->h = x->height;
			return 1;
		}
	}
}
