/* x11.c - Archaic's display for X11: Xlib only, calls that X11R2
 * already had. One font. Xlib has no scroll bars: Archaic draws its own
 * (g_bar).
 *
 * The colors are the default visual's. Black and white (depth 1): the
 * pictures in 1 bit. TrueColor (and DirectColor): each color's pixel
 * from the visual's masks. A colormap (PseudoColor, StaticColor, the
 * grays): the colors of a picture are allocated (XAllocColor), shared
 * with the other programs; when the colormap is full, the nearest
 * color it has. A new picture's colors free the last ones.
 */
#include "archaic.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef NOPROTO
char *malloc();
void free();
#else
#include <stdlib.h>
#endif

static Display *dpy;
static Window win;
static GC gc;
static XFontStruct *font;
static int scr, ww, wh;
static unsigned long black, white;
static Atom wmdel;
static Visual *vis;
static Colormap cmap;
static int depth, truec;		/* truec: TrueColor or DirectColor */
static unsigned char ppal[768];		/* the colors pmap is for */
static unsigned long pmap[256];		/* their pixels */
static int pvalid;
static unsigned long cells[256];	/* the cells allocated for them */
static int ncells;
static XColor *cmc;			/* the colormap's colors, for the nearest */
static int ncmc;

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
	vis = DefaultVisual(dpy, scr);
	depth = DefaultDepth(dpy, scr);
	cmap = DefaultColormap(dpy, scr);
	truec = vis->class == TrueColor || vis->class == DirectColor;
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

/* the colors: 2 (depth 1), the colormap's size, or 2 to the depth */
long g_colors()
{
	if (depth <= 1)
		return 2L;
	if (truec)
		return 1L << (depth > 24 ? 24 : depth);
	return vis->map_entries > 2 ? (long)vis->map_entries : 2L;
}

/* v (0 to 255) in the field of a TrueColor pixel that mask has */
static unsigned long field(v, mask)
int v;
unsigned long mask;
{
	int s = 0;

	if (!mask)
		return 0;
	while (!(mask & 1)) {
		mask >>= 1;
		++s;
	}
	return ((unsigned long)v * mask + 127) / 255 << s;
}

/* the colormap's cell nearest to r, g, b (0 to 255) */
static unsigned long nearest(r, g, b)
int r, g, b;
{
	long d, best = -1;
	unsigned long p = black;
	int i, n, dr, dg, db;

	if (!cmc) {			/* what the colormap has now */
		n = vis->map_entries > 4096 ? 4096 : vis->map_entries;
		cmc = (XColor *)malloc((unsigned)n * sizeof *cmc);
		if (!cmc)
			return black;
		for (i = 0; i < n; ++i)
			cmc[i].pixel = i;
		XQueryColors(dpy, cmap, cmc, n);
		ncmc = n;
	}
	for (i = 0; i < ncmc; ++i) {
		dr = (cmc[i].red >> 8) - r;
		dg = (cmc[i].green >> 8) - g;
		db = (cmc[i].blue >> 8) - b;
		d = (long)dr * dr + (long)dg * dg + (long)db * db;
		if (best < 0 || d < best) {
			best = d;
			p = cmc[i].pixel;
		}
	}
	return p;
}

/* pmap: the pixel of each color of pal, unless pal is the last one */
static void mappal(pal)
unsigned char *pal;
{
	XColor c;
	int i, j;

	for (i = 0; i < 768 && pal[i] == ppal[i]; ++i)
		;
	if (pvalid && i == 768)
		return;
	for (i = 0; i < 768; ++i)
		ppal[i] = pal[i];
	pvalid = 1;
	if (truec) {
		for (i = 0; i < 256; ++i)
			pmap[i] = field(pal[3 * i], vis->red_mask) |
				field(pal[3 * i + 1], vis->green_mask) |
				field(pal[3 * i + 2], vis->blue_mask);
		return;
	}
	if (ncells && (vis->class == PseudoColor || vis->class == GrayScale))
		XFreeColors(dpy, cmap, cells, ncells, 0L);
	ncells = 0;
	if (cmc)
		free((char *)cmc);
	cmc = 0;
	for (i = 0; i < 256; ++i) {
		for (j = 0; j < i; ++j)		/* the same as one before */
			if (pal[3 * j] == pal[3 * i] && pal[3 * j + 1] ==
				pal[3 * i + 1] && pal[3 * j + 2] == pal[3 * i + 2])
				break;
		if (j < i) {
			pmap[i] = pmap[j];
			continue;
		}
		c.red = pal[3 * i] * 257;
		c.green = pal[3 * i + 1] * 257;
		c.blue = pal[3 * i + 2] * 257;
		c.flags = DoRed | DoGreen | DoBlue;
		if (XAllocColor(dpy, cmap, &c)) {
			pmap[i] = c.pixel;
			cells[ncells++] = c.pixel;
		} else
			pmap[i] = nearest(pal[3 * i], pal[3 * i + 1], pal[3 * i + 2]);
	}
}

/* rows of 8-bit color numbers; pal: the red, green, blue of each */
void g_pix(x, y, w, h, pix, bpl, pal)
int x, y, w, h, bpl;
unsigned char *pix, *pal;
{
	static int one = 1;
	XImage *im;
	unsigned char *b;
	unsigned int *d;
	char *d8;
	int i, j;

	if (w <= 0 || h <= 0)
		return;
	mappal(pal);
	im = XCreateImage(dpy, vis, (unsigned)depth, ZPixmap, 0, (char *)0,
		(unsigned)w, (unsigned)h, 32, 0);
	if (!im)
		return;
	im->data = (char *)malloc((unsigned)((long)im->bytes_per_line * h));
	if (!im->data) {
		XDestroyImage(im);
		return;
	}
	/* 32 bits a pixel: written in this machine's order, Xlib turns
	 * them round for the server if it needs to */
	if (im->bits_per_pixel == 32 && sizeof *d == 4)
		im->byte_order = *(char *)&one ? LSBFirst : MSBFirst;
	for (j = 0; j < h; ++j) {
		b = pix + (long)j * bpl;
		d8 = im->data + (long)j * im->bytes_per_line;
		d = (unsigned int *)d8;
		if (im->bits_per_pixel == 32 && sizeof *d == 4)
			for (i = 0; i < w; ++i)
				d[i] = (unsigned int)pmap[b[i]];
		else if (im->bits_per_pixel == 8)
			for (i = 0; i < w; ++i)
				d8[i] = (char)pmap[b[i]];
		else
			for (i = 0; i < w; ++i)
				XPutPixel(im, i, j, pmap[b[i]]);
	}
	XPutImage(dpy, win, gc, im, 0, 0, x, y, (unsigned)w, (unsigned)h);
	XDestroyImage(im);		/* and its data */
}

void g_flush()
{
	XFlush(dpy);
}

int g_fd()
{
	return dpy ? ConnectionNumber(dpy) : -1;
}

/* a clock in milliseconds (for the spinner) */
long g_ms()
{
	struct timeval tv;

	gettimeofday(&tv, (struct timezone *)0);
	return (tv.tv_sec % 1000000L) * 1000L + tv.tv_usec / 1000;
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
