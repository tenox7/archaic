/* x11.c - Archaic's display for X11: Xlib only, calls that X11R2
 * already had. Black and white, one font. Xlib has no scroll bars:
 * Archaic draws its own (g_bar).
 */
#include "archaic.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

static Display *dpy;
static Window win;
static GC gc;
static XFontStruct *font;
static int scr, ww, wh;
static unsigned long black, white;
static Atom wmdel;

int g_bar = 1;

void g_sbar(pos, max, vis)
long pos, max, vis;
{
}

int g_open(geom, w, h)
char *geom;
int *w, *h;
{
	static XSizeHints hints;
	static char *names[] = { "6x13", "fixed", "9x15" };
	Atom proto;
	int x = 0, y = 0, m, i;
	unsigned int gw = 640, gh = 400;

	dpy = XOpenDisplay((char *)0);
	if (!dpy)
		return -1;
	scr = DefaultScreen(dpy);
	black = BlackPixel(dpy, scr);
	white = WhitePixel(dpy, scr);
	if (geom) {
		m = XParseGeometry(geom[0] == '=' ? geom + 1 : geom, &x, &y, &gw, &gh);
		if (m & XNegative)
			x += DisplayWidth(dpy, scr) - (int)gw - 2;
		if (m & YNegative)
			y += DisplayHeight(dpy, scr) - (int)gh - 2;
	}
	for (i = 0; i < 3 && !font; ++i)
		font = XLoadQueryFont(dpy, names[i]);
	if (!font)
		return -1;
	win = XCreateSimpleWindow(dpy, RootWindow(dpy, scr), x, y, gw, gh, 1,
		black, white);
	hints.flags = USPosition | USSize;
	hints.x = x;
	hints.y = y;
	hints.width = gw;
	hints.height = gh;
	XSetNormalHints(dpy, win, &hints);
	XStoreName(dpy, win, "Archaic");
	XSelectInput(dpy, win, ExposureMask | KeyPressMask | ButtonPressMask |
		ButtonReleaseMask | ButtonMotionMask | StructureNotifyMask);
	gc = XCreateGC(dpy, win, 0L, (XGCValues *)0);
	XSetFont(dpy, gc, font->fid);
	/* the window manager's close button (ICCCM, X11R4 and later) */
	proto = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmdel = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XChangeProperty(dpy, win, proto, XA_ATOM, 32, PropModeReplace,
		(unsigned char *)&wmdel, 1);
	XMapWindow(dpy, win);
	*w = ww = gw;
	*h = wh = gh;
	return 0;
}

void g_close()
{
	if (dpy)
		XCloseDisplay(dpy);
	dpy = 0;
}

void g_title(s)
char *s;
{
	XStoreName(dpy, win, s);
}

void g_fill(x, y, w, h, c)
int x, y, w, h, c;
{
	XSetForeground(dpy, gc, c ? black : white);
	XFillRectangle(dpy, win, gc, x, y, (unsigned)w, (unsigned)h);
}

void g_invert(x, y, w, h)
int x, y, w, h;
{
	XSetFunction(dpy, gc, GXxor);
	XSetForeground(dpy, gc, black ^ white);
	XFillRectangle(dpy, win, gc, x, y, (unsigned)w, (unsigned)h);
	XSetFunction(dpy, gc, GXcopy);
}

void g_text(x, y, s, n, inv)
int x, y, n, inv;
char *s;
{
	XSetForeground(dpy, gc, inv ? white : black);
	XSetBackground(dpy, gc, inv ? black : white);
	XDrawImageString(dpy, win, gc, x, y + font->ascent, s, n);
}

int g_textw(s, n)
char *s;
int n;
{
	return XTextWidth(font, s, n);
}

void g_font(height, base)
int *height, *base;
{
	*height = font->ascent + font->descent;
	*base = font->ascent;
}

/* rows of 1-bit pixels, 1 black, high bit left */
void g_bits(x, y, w, h, bits, bpl)
int x, y, w, h, bpl;
unsigned char *bits;
{
	XImage *im;

	im = XCreateImage(dpy, DefaultVisual(dpy, scr), 1, XYBitmap, 0,
		(char *)bits, (unsigned)w, (unsigned)h, 8, bpl);
	if (!im)
		return;
	im->byte_order = MSBFirst;
	im->bitmap_bit_order = MSBFirst;
	XSetForeground(dpy, gc, black);
	XSetBackground(dpy, gc, white);
	XPutImage(dpy, win, gc, im, 0, 0, x, y, (unsigned)w, (unsigned)h);
	im->data = 0;
	XDestroyImage(im);
}

void g_flush()
{
	XFlush(dpy);
}

int g_fd()
{
	return dpy ? ConnectionNumber(dpy) : -1;
}

int g_event(e, wait)
Event *e;
int wait;
{
	static XEvent ev, nx;
	static char s[8];
	fd_set r;
	struct timeval tv;
	KeySym ks;
	int n, fd;

	for (;;) {
		if (!wait && !XPending(dpy))
			return 0;
		if (wait == 2 && !XPending(dpy)) {	/* 1/10 s at most */
			fd = ConnectionNumber(dpy);
			FD_ZERO(&r);
			FD_SET(fd, &r);
			tv.tv_sec = 0;
			tv.tv_usec = 100000L;
			if (select(fd + 1, &r, (fd_set *)0, (fd_set *)0, &tv) <= 0 ||
				!XPending(dpy))
				return 0;
		}
		XNextEvent(dpy, &ev);
		switch (ev.type) {
		case Expose:
			e->type = E_EXPOSE;
			e->x = ev.xexpose.x;
			e->y = ev.xexpose.y;
			e->w = ev.xexpose.width;
			e->h = ev.xexpose.height;
			return 1;
		case KeyPress:
			n = XLookupString(&ev.xkey, s, (int)sizeof s, &ks,
				(XComposeStatus *)0);
			if (n > 0) {
				e->type = E_KEY;
				e->key = s[0] & 0377;
				return 1;
			}
			break;
		case ButtonPress:
			e->type = E_DOWN;
			e->x = ev.xbutton.x;
			e->y = ev.xbutton.y;
			e->key = ev.xbutton.button;
			if (e->key == 4 || e->key == 5) {	/* the wheel */
				e->type = E_SCROLL;
				e->y = e->key == 4 ? -1 : 1;
				e->key = S_STEP;
			}
			return 1;
		case ButtonRelease:
			if (ev.xbutton.button > 3)	/* the wheel */
				break;
			e->type = E_UP;
			e->x = ev.xbutton.x;
			e->y = ev.xbutton.y;
			e->key = ev.xbutton.button;
			return 1;
		case MotionNotify:
			/* a button is down; only the last place counts */
			while (XPending(dpy)) {
				XPeekEvent(dpy, &nx);
				if (nx.type != MotionNotify)
					break;
				XNextEvent(dpy, &ev);
			}
			e->type = E_MOVE;
			e->x = ev.xmotion.x;
			e->y = ev.xmotion.y;
			e->key = 0;
			return 1;
		case ConfigureNotify:
			if (ev.xconfigure.width != ww || ev.xconfigure.height != wh) {
				e->type = E_SIZE;
				e->w = ww = ev.xconfigure.width;
				e->h = wh = ev.xconfigure.height;
				return 1;
			}
			break;
		case ClientMessage:
			if ((Atom)ev.xclient.data.l[0] == wmdel) {
				e->type = E_QUIT;
				return 1;
			}
			break;
		}
	}
}
