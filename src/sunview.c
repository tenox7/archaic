/* sunview.c - Archaic's display for SunView, Sun's window system before
 * X11 (SunOS 3 and 4). Built on SunOS 4.1.4 (sun4m). It uses the toolkit
 * (libsuntool): a frame with a canvas and a scroll bar, drawn through the
 * canvas's pixwin.
 *
 * What is special there:
 * - Archaic has its own loop. notify_dispatch() runs the toolkit's
 *   handlers, and they put Archaic's events in a queue (q[]). Between
 *   them, select() waits on the frame's and the canvas's files.
 * - The canvas is not retained: when a part of it shows again, SunView
 *   calls the repaint function, and Archaic draws that part again
 *   (E_EXPOSE). A retained canvas (SunView keeps a copy in memory) lost
 *   its copy after a resize: it drew those parts white.
 * - The scroll bar is SunView's own (g_bar 0). The canvas would scroll by
 *   itself: an interposed function takes the scroll requests away from
 *   it. The scroll bar has already set its new view start: that is the
 *   new place on the page (E_SCROLL, S_TO).
 * - Colors: on a color screen (8 bits, as the cg3) the canvas has a
 *   colormap segment of its own with 128 colors: 0 is white (the
 *   background), 127 black (the foreground). A segment of 256 takes all
 *   of the screen's colormap, and the other windows lose their colors.
 *   A picture's colors go into 1 to 126 (see mappal). The scroll bar
 *   draws with 0 and 127 too. Two Archaic windows: SunView loads the
 *   segment of the window under the pointer, the other one may show
 *   wrong colors. On a black and white screen (bw2): 1 bit, no colors.
 * - Text: the default font (pf_default; the DEFAULT_FONT variable
 *   chooses another one).
 * - Keys: SunView sends the arrows as ESC [ A to ESC [ D: Archaic gets
 *   Up and Down as scrolls. R9 (PgUp), R15 (PgDn), R7 (Home) and R13
 *   (End) scroll too. Esc stops a load. (The Stop key, L1, sends no
 *   event: it is the shift key of Stop-A.)
 */
#define Event Sv_event		/* SunView has an Event and an Image too */
#define Image Sv_image
#include <suntool/sunview.h>
#include <suntool/canvas.h>
#include <suntool/scrollbar.h>
#include <sunwindow/cms.h>
#include <sys/time.h>
#undef Event
#undef Image
#include "archaic.h"

#define NCMS 128		/* the colormap segment: colors */
#define QLEN 64			/* the event queue */
#define BIG 0x7fffffffL		/* End: as far down as the page goes */

static Frame frame;
static Canvas canvas;
static Scrollbar sb;
static Pixwin *pw;
static Pixfont *font;
static int fg;			/* black: the segment's last color */
static int ffd = -1, cfd = -1;	/* the frame's and the canvas's files */
static int ww, wh;		/* the canvas */
static int fh, fbase;		/* the font: height, baseline */
static int quit;		/* the frame was closed */
static int closing;		/* g_close: let the frame go */
static int esc;			/* ESC came (1), then [ (2) */
static int newsize;		/* the canvas has a new size */
static long bpos, bmax, bvis;	/* what the scroll bar shows */
static Event q[QLEN];
static int qhead, qn;
static unsigned char cr[NCMS], cg[NCMS], cb[NCMS];	/* the segment */
static unsigned char ppal[768];	/* the colors cmap is for */
static int pvalid, nused;
static unsigned char cmap[256];	/* a picture's color: its place */
static char tbuf[512];
static short band[32768];	/* memory pixrects: 64 KB at a time */
static struct mpr_data md;
static Pixrect mpr;

int g_bar = 0;

/* a new event at the end of the queue; E_MOVE takes the place of an
 * E_MOVE before it */
static Event *put(type)
int type;
{
	Event *e;

	if (type == E_MOVE && qn > 0) {
		e = &q[(qhead + qn - 1) % QLEN];
		if (e->type == E_MOVE)
			return e;
	}
	if (qn == QLEN)			/* full: the oldest goes */
		qhead = (qhead + 1) % QLEN, --qn;
	e = &q[(qhead + qn) % QLEN];
	++qn;
	e->type = type;
	e->x = e->y = e->w = e->h = e->key = 0;
	return e;
}

static void scrl(y, unit)
int y, unit;
{
	Event *e = put(E_SCROLL);

	e->y = y;
	e->key = unit;
}

static void putkey(c)
int c;
{
	put(E_KEY)->key = c;
}

/* the frame is going (its menu's Quit): Archaic quits, then closes it */
static Notify_value ondestroy(client, status)
Notify_client client;
Destroy_status status;
{
	if (!closing) {
		if (status == DESTROY_CHECKING) {
			quit = 1;
			put(E_QUIT);
			notify_veto_destroy(client);
			return NOTIFY_DONE;
		}
		quit = 1;
	}
	return notify_next_destroy_func(client, status);
}

/* the scroll bar's requests: not for the canvas, for Archaic */
static Notify_value onscroll(client, ev, arg, type)
Notify_client client;
Sv_event *ev;
Notify_arg arg;
Notify_event_type type;
{
	int v, m;

	if (event_id(ev) == SCROLL_REQUEST) {
		v = (int)scrollbar_get(sb, SCROLL_VIEW_START);
		m = v;
		if (m > bmax)		/* the bar does not know the end */
			m = (int)bmax;
		if (m < 0)
			m = 0;
		if (m != v) {
			scrollbar_set(sb, SCROLL_VIEW_START, m, 0);
			scrollbar_paint(sb);
		}
		scrl(m, S_TO);
		return NOTIFY_DONE;
	}
	return notify_next_event_func(client, (Notify_event)ev, arg, type);
}

/* a part of the canvas shows again: Archaic draws it (all the parts
 * that wait, as one rectangle) */
static void onrepaint(c, p, area)
Canvas c;
Pixwin *p;
Rectlist *area;
{
	Event *e;
	Rect *r = &area->rl_bound;
	int x, y, x2, y2;

	if (r->r_width <= 0 || r->r_height <= 0)
		return;
	x = r->r_left;
	y = r->r_top;
	x2 = x + r->r_width;
	y2 = y + r->r_height;
	if (qn > 0 && (e = &q[(qhead + qn - 1) % QLEN])->type == E_EXPOSE) {
		if (e->x < x)
			x = e->x;
		if (e->y < y)
			y = e->y;
		if (e->x + e->w > x2)
			x2 = e->x + e->w;
		if (e->y + e->h > y2)
			y2 = e->y + e->h;
	} else
		e = put(E_EXPOSE);
	e->x = x;
	e->y = y;
	e->w = x2 - x;
	e->h = y2 - y;
}

static void onresize(c, w, h)
Canvas c;
int w, h;
{
	Event *e;

	ww = w;
	wh = h;
	newsize = 1;
	e = put(E_SIZE);
	e->w = w;
	e->h = h;
}

/* the canvas's events, in its own place (the scroll bar is not in it) */
static void onevent(win, ev, arg)
Window win;
Sv_event *ev;
caddr_t arg;
{
	Event *e;
	int id = event_id(ev), b;

	if (id == MS_LEFT || id == MS_MIDDLE || id == MS_RIGHT) {
		b = id == MS_LEFT ? 1 : id == MS_MIDDLE ? 2 : 3;
		e = put(event_is_up(ev) ? E_UP : E_DOWN);
		e->x = event_x(ev);
		e->y = event_y(ev);
		e->key = b;
		return;
	}
	if (id == LOC_DRAG) {
		e = put(E_MOVE);
		e->x = event_x(ev);
		e->y = event_y(ev);
		return;
	}
	if (event_is_up(ev))
		return;
	if (id >= 0 && id <= 255) {
		if (esc == 1 && id == '[') {
			esc = 2;
			return;
		}
		if (esc == 2) {		/* the arrows: ESC [ A to D */
			esc = 0;
			if (id == 'A')
				scrl(-1, S_STEP);
			else if (id == 'B')
				scrl(1, S_STEP);
			else if (id < 'A' || id > 'D') {
				putkey(033);
				putkey('[');
				putkey(id);
			}
			return;
		}
		if (esc)
			putkey(033);
		esc = id == 033;
		if (!esc)
			putkey(id);
		return;
	}
	if (id == KEY_RIGHT(8))
		scrl(-1, S_STEP);
	else if (id == KEY_RIGHT(14))
		scrl(1, S_STEP);
	else if (id == KEY_RIGHT(9))
		scrl(-1, S_PAGE);
	else if (id == KEY_RIGHT(15))
		scrl(1, S_PAGE);
	else if (id == KEY_RIGHT(7))
		scrl(0, S_TO);
	else if (id == KEY_RIGHT(13))
		scrl((int)BIG, S_TO);
}

static int ready();

/* run the toolkit's handlers; a lone ESC is Esc */
static void dispatch()
{
	int n;

	notify_dispatch();
	/* each dispatch reads one event: the rest of ESC [ A comes next */
	for (n = 0; esc && n < 10 && ready(50); ++n)
		notify_dispatch();
	if (esc) {
		putkey(033);
		if (esc == 2)
			putkey('[');
		esc = 0;
	}
	if (newsize && sb) {		/* the canvas set the bar: Archaic's again */
		newsize = 0;
		g_sbar(bpos, bmax, bvis);
	}
}

/* wait for the frame or the canvas, at most ms milliseconds; 1 if one
 * of them has input */
static int waitfor(ms)
int ms;
{
	struct timeval tv;
	fd_set r;

	FD_ZERO(&r);
	FD_SET(ffd, &r);
	FD_SET(cfd, &r);
	tv.tv_sec = ms / 1000;
	tv.tv_usec = (ms % 1000) * 1000L;
	return select((ffd > cfd ? ffd : cfd) + 1, &r, (fd_set *)0,
		(fd_set *)0, &tv) > 0;
}

/* the canvas has input within ms milliseconds */
static int ready(ms)
int ms;
{
	struct timeval tv;
	fd_set r;

	FD_ZERO(&r);
	FD_SET(cfd, &r);
	tv.tv_sec = 0;
	tv.tv_usec = ms * 1000L;
	return select(cfd + 1, &r, (fd_set *)0, (fd_set *)0, &tv) > 0;
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

/* the font: its height, and the baseline below the top */
static void fontsize()
{
	int c, a;

	fh = font->pf_defaultsize.y;
	fbase = 0;
	for (c = ' '; c < 127; ++c) {
		a = -font->pf_char[c].pc_home.y;
		if (font->pf_char[c].pc_pr && a > fbase)
			fbase = a;
	}
	if (fbase <= 0 || fbase > fh)
		fbase = fh * 3 / 4;
}

int g_open(geom, w, h)
char *geom;
int *w, *h;
{
	Rect *sr;
	struct colormapseg cms;
	static char name[CMS_NAMESIZE];
	int sw = 1152, sh = 900, gw, gh, x = -1, y = -1, xneg = 0, yneg = 0;
	int planes, sbw, i;
	char *s;

	frame = window_create((Window)0, FRAME, FRAME_LABEL, "Archaic",
		FRAME_SHOW_LABEL, TRUE, 0);
	if (!frame)
		return -1;
	sr = (Rect *)window_get(frame, WIN_SCREEN_RECT);
	if (sr && sr->r_width > 0 && sr->r_height > 0) {
		sw = sr->r_width;
		sh = sr->r_height;
	}
	gw = sw * 9 / 10 - 30;		/* a bit smaller than the screen */
	gh = sh * 9 / 10 - 40;
	if (geom) {
		s = geom;
		if (*s == '=')
			++s;
		if (num(&s, &i) && *s == 'x') {
			++s;
			if (num(&s, &y)) {
				gw = i;
				gh = y;
			}
			y = -1;
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
	font = pf_default();
	if (!font)
		return -1;
	fontsize();
	sb = scrollbar_create(SCROLL_LINE_HEIGHT, fh, 0);
	canvas = window_create(frame, CANVAS,
		WIN_VERTICAL_SCROLLBAR, sb,
		CANVAS_RETAINED, FALSE,
		CANVAS_AUTO_EXPAND, TRUE,
		CANVAS_AUTO_SHRINK, TRUE,
		CANVAS_RESIZE_PROC, onresize,
		CANVAS_REPAINT_PROC, onrepaint,
		WIN_EVENT_PROC, onevent,
		0);
	if (!canvas)
		return -1;
	window_set(canvas, WIN_CONSUME_KBD_EVENTS, WIN_ASCII_EVENTS,
		WIN_RIGHT_KEYS, 0, 0);
	window_set(canvas, WIN_CONSUME_PICK_EVENTS, WIN_MOUSE_BUTTONS,
		WIN_UP_EVENTS, LOC_DRAG, 0, 0);
	/* the size inside: the scroll bar is in the canvas's window */
	sbw = (int)scrollbar_get(sb, SCROLL_THICKNESS);
	if (sbw <= 0 || sbw > 50)
		sbw = SCROLL_DEFAULT_WIDTH;
	window_set(canvas, WIN_WIDTH, gw + sbw, WIN_HEIGHT, gh, 0);
	window_fit(frame);
	gw = (int)window_get(frame, WIN_WIDTH);
	gh = (int)window_get(frame, WIN_HEIGHT);
	if (x < 0)			/* no place given: the middle */
		x = (sw - gw) / 2;
	else if (xneg)
		x = sw - gw - x;
	if (y < 0)
		y = (sh - gh) / 2;
	else if (yneg)
		y = sh - gh - y;
	if (x > sw - gw)
		x = sw - gw;
	if (y > sh - gh)
		y = sh - gh;
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	window_set(frame, WIN_X, x, WIN_Y, y, 0);
	pw = canvas_pixwin(canvas);
	fg = 1;
	if (pw->pw_pixrect->pr_depth > 1) {	/* colors: a segment of our own */
		for (i = 0; i < NCMS; ++i)
			cr[i] = cg[i] = cb[i] = 255;
		cr[NCMS - 1] = cg[NCMS - 1] = cb[NCMS - 1] = 0;
		sprintf(name, "archaic%d", getpid() % 100000);
		pw_setcmsname(pw, name);
		pw_putcolormap(pw, 0, NCMS, cr, cg, cb);
	}
	pw_getcmsdata(pw, &cms, &planes);
	if (cms.cms_size > 1)
		fg = cms.cms_size - 1;
	notify_interpose_event_func(canvas, onscroll, NOTIFY_SAFE);
	notify_interpose_destroy_func(frame, ondestroy);
	ffd = (int)window_get(frame, WIN_FD);
	cfd = (int)window_get(canvas, WIN_FD);
	mpr.pr_ops = &mem_ops;
	mpr.pr_data = (caddr_t)&md;
	window_set(frame, WIN_SHOW, TRUE, 0);
	/* drawing counts only once the canvas has its size */
	ww = wh = 0;
	for (i = 0; i < 100 && !ww && !quit; ++i) {
		dispatch();
		if (!ww)
			waitfor(50);
	}
	if (!ww) {
		ww = (int)window_get(canvas, CANVAS_WIDTH);
		wh = (int)window_get(canvas, CANVAS_HEIGHT);
	}
	qn = 0;				/* that first E_SIZE is the size */
	g_fill(0, 0, ww, wh, 0);
	*w = ww;
	*h = wh;
	return 0;
}

void g_close()
{
	if (!frame)
		return;
	closing = 1;
	window_set(frame, FRAME_NO_CONFIRM, TRUE, 0);
	window_destroy(frame);
	notify_dispatch();
	frame = 0;
}

void g_title(s)
char *s;
{
	static char lab[MAXTITLE + 12];
	int i;

	for (i = 0; i < MAXTITLE && s[i]; ++i)
		lab[i] = (s[i] & 0377) < ' ' ? ' ' : s[i];
	lab[i] = 0;
	window_set(frame, FRAME_LABEL, lab, 0);
}

/* the scroll bar: the page is end + vis long, vis shows, from pos */
void g_sbar(pos, end, vis)	/* end: max (SunView has a max macro) */
long pos, end, vis;
{
	bpos = pos;
	bmax = end;
	bvis = vis;
	if (!sb || vis <= 0)
		return;
	scrollbar_set(sb, SCROLL_OBJECT_LENGTH, (int)(end + vis),
		SCROLL_VIEW_START, (int)pos, SCROLL_VIEW_LENGTH, (int)vis, 0);
	scrollbar_paint(sb);
}

/* c: 0 white (the background), 1 black (the foreground) */
void g_fill(x, y, w, h, c)
int x, y, w, h, c;
{
	if (w > 0 && h > 0)
		pw_writebackground(pw, x, y, w, h, c ? PIX_SET : PIX_CLR);
}

void g_invert(x, y, w, h)
int x, y, w, h;
{
	if (w > 0 && h > 0)
		pw_writebackground(pw, x, y, w, h, PIX_NOT(PIX_DST));
}

/* the font's letter for c: a space for a no-break space, ? for the
 * letters it does not have */
static int glyph(c)
int c;
{
	c &= 0377;
	if (c == 0240)
		c = ' ';
	if (c != ' ' && !font->pf_char[c].pc_pr)
		c = '?';
	return c;
}

int g_textw(s, n)
char *s;
int n;
{
	int w = 0;

	for (; n > 0; --n)
		w += font->pf_char[glyph(*s++)].pc_adv.x;
	return w;
}

/* black on white, or white on black if inv */
void g_text(x, y, s, n, inv)
int x, y, n, inv;
char *s;
{
	int k, i;

	while (n > 0) {
		k = n < (int)sizeof tbuf - 1 ? n : (int)sizeof tbuf - 1;
		for (i = 0; i < k; ++i)
			tbuf[i] = glyph(s[i]);
		tbuf[k] = 0;
		i = g_textw(s, k);
		g_fill(x, y, i, fh, inv);
		pw_text(pw, x, y + fbase, inv ? PIX_NOT(PIX_SRC) & PIX_DST :
			PIX_SRC | PIX_DST, font, tbuf);
		x += i;
		s += k;
		n -= k;
	}
}

void g_font(height, base)
int *height, *base;
{
	*height = fh;
	*base = fbase;
}

/* a memory pixrect on band: w x h, d bits a pixel, lb bytes a row */
static Pixrect *mem(w, h, d, lb)
int w, h, d, lb;
{
	mpr.pr_size.x = w;
	mpr.pr_size.y = h;
	mpr.pr_depth = d;
	md.md_linebytes = lb;
	md.md_image = band;
	md.md_offset.x = md.md_offset.y = 0;
	md.md_primary = 0;
	md.md_flags = 0;
	return &mpr;
}

/* rows of 1-bit pixels, 1 black, the high bit on the left: as a Sun's
 * memory pixrect, but its rows are a whole number of 16-bit words */
void g_bits(x, y, w, h, bits, bpl)
int x, y, w, h, bpl;
unsigned char *bits;
{
	unsigned char *d, *p;
	int lb = mpr_linebytes(w, 1), rows, n, r, i;

	if (w <= 0 || h <= 0)
		return;
	rows = (int)sizeof band / lb;
	while (h > 0) {
		n = h < rows ? h : rows;
		for (r = 0; r < n; ++r) {
			p = bits + (long)r * bpl;
			d = (unsigned char *)band + (long)r * lb;
			for (i = 0; i < lb; ++i)
				d[i] = i < bpl ? p[i] : 0;
		}
		pw_rop(pw, x, y, w, n, PIX_SRC, mem(w, n, 1, lb), 0, 0);
		bits += (long)n * bpl;
		y += n;
		h -= n;
	}
}

/* the colors: the segment's, or 2 on a black and white screen */
long g_colors()
{
	return pw->pw_pixrect->pr_depth > 1 ? (long)NCMS : 2L;
}

static long dist(r, g, b, r2, g2, b2)
int r, g, b, r2, g2, b2;
{
	return (long)(r - r2) * (r - r2) + (long)(g - g2) * (g - g2) +
		(long)(b - b2) * (b - b2);
}

/* the place nearest to r, g, b among the first n of the segment and
 * black */
static int nearest(r, g, b, n)
int r, g, b, n;
{
	long d, best = -1;
	int i, p = fg;

	for (i = 0; i <= n; ++i) {
		if (i == n)
			i = NCMS - 1;
		d = dist(r, g, b, (int)cr[i], (int)cg[i], (int)cb[i]);
		if (best < 0 || d < best) {
			best = d;
			p = i;
		}
	}
	return p;
}

/* a picture's colors (but white and black) that may get a place */
static int kr[256], kg[256], kb[256];
static int nn[256];		/* the nearest other one; -1: white or black */
static long nd[256];		/* how near */
static char gone[256];		/* it has no place: the nearest takes it */

/* color i's nearest among white, black and the others still in */
static void near1(i, n)
int i, n;
{
	long d;
	int j;

	nn[i] = -1;
	nd[i] = dist(kr[i], kg[i], kb[i], 255, 255, 255);
	d = dist(kr[i], kg[i], kb[i], 0, 0, 0);
	if (d < nd[i])
		nd[i] = d;
	for (j = 0; j < n; ++j) {
		if (j == i || gone[j])
			continue;
		d = dist(kr[i], kg[i], kb[i], kr[j], kg[j], kb[j]);
		if (d < nd[i]) {
			nd[i] = d;
			nn[i] = j;
		}
	}
}

/* cmap: each color of pal in the segment, unless pal is the last one.
 * White and black have their places (0, NCMS - 1). If more colors are
 * left than places (WRP's 128 colors often have no pure black), the
 * color nearest to another one goes, until they fit: a small color
 * that is far from the rest (a red line) keeps its place. */
static void mappal(pal)
unsigned char *pal;
{
	int i, j, n, left, r, g, b;

	for (i = 0; i < 768 && pal[i] == ppal[i]; ++i)
		;
	if (pvalid && i == 768)
		return;
	for (i = 0; i < 768; ++i)
		ppal[i] = pal[i];
	pvalid = 1;
	for (n = i = 0; i < 256; ++i) {
		r = pal[3 * i];
		g = pal[3 * i + 1];
		b = pal[3 * i + 2];
		if ((r == 255 && g == 255 && b == 255) || (!r && !g && !b))
			continue;
		for (j = 0; j < n; ++j)
			if (kr[j] == r && kg[j] == g && kb[j] == b)
				break;
		if (j == n) {
			kr[n] = r;
			kg[n] = g;
			kb[n] = b;
			gone[n++] = 0;
		}
	}
	left = n;
	if (left > NCMS - 2)
		for (i = 0; i < n; ++i)
			near1(i, n);
	while (left > NCMS - 2) {
		for (j = -1, i = 0; i < n; ++i)
			if (!gone[i] && (j < 0 || nd[i] < nd[j]))
				j = i;
		gone[j] = 1;
		--left;
		for (i = 0; i < n; ++i)
			if (!gone[i] && nn[i] == j)
				near1(i, n);
	}
	for (nused = 1, i = 0; i < n; ++i)	/* 0 is white */
		if (!gone[i]) {
			cr[nused] = kr[i];
			cg[nused] = kg[i];
			cb[nused] = kb[i];
			++nused;
		}
	for (i = 0; i < 256; ++i)
		cmap[i] = nearest((int)pal[3 * i], (int)pal[3 * i + 1],
			(int)pal[3 * i + 2], nused);
	pw_putcolormap(pw, 0, NCMS, cr, cg, cb);
}

/* rows of 8-bit color numbers; pal: the red, green, blue of each */
void g_pix(x, y, w, h, pix, bpl, pal)
int x, y, w, h, bpl;
unsigned char *pix, *pal;
{
	unsigned char *d, *p;
	int lb = mpr_linebytes(w, 8), rows, n, r, i;

	if (w <= 0 || h <= 0)
		return;
	mappal(pal);
	rows = (int)sizeof band / lb;
	while (h > 0) {
		n = h < rows ? h : rows;
		for (r = 0; r < n; ++r) {
			p = pix + (long)r * bpl;
			d = (unsigned char *)band + (long)r * lb;
			for (i = 0; i < w; ++i)
				d[i] = cmap[p[i]];
		}
		pw_rop(pw, x, y, w, n, PIX_SRC, mem(w, n, 8, lb), 0, 0);
		pix += (long)n * bpl;
		y += n;
		h -= n;
	}
}

void g_flush()
{
}

/* the canvas's file: input there ends the network's wait (nsock.c) */
int g_fd()
{
	return cfd;
}

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
	Event *f;
	int tries = 0;

	for (;;) {
		if (!qn)
			dispatch();
		if (qn) {
			f = &q[qhead];
			e->type = f->type;
			e->x = f->x;
			e->y = f->y;
			e->w = f->w;
			e->h = f->h;
			e->key = f->key;
			qhead = (qhead + 1) % QLEN;
			--qn;
			return 1;
		}
		if (!wait || quit || (wait == 2 && tries++))
			return quit ? (e->type = E_QUIT, 1) : 0;
		/* not longer than 1/2 s: a signal (SIGWINCH) may have come
		 * just before the select */
		waitfor(wait == 2 ? 100 : 500);
	}
}
