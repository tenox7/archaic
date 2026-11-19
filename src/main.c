/* main.c - Archaic: the page on the screen, the mouse and the keys.
 *
 *	archaic [=WxH+X+Y] [-a] [-t] [-v] [-o file.pbm] [-s script] [url]
 *
 * url		WRP's address: host:port or http://host:port/
 *		(default $WRP, else DEFURL)
 * =WxH+X+Y	window size and place (or -g WxH+X+Y)
 * -a		show WRP's M, T and C menus (mode, picture type, colors)
 * -t		test: no window; print the page and its pictures
 * -v		status messages to stderr too
 * -o file	at the end, write the first picture as a PBM file
 * -s script	act out a script, then quit: characters are keys, ~ is
 *		Return, @x,y; is a click, % waits 2 seconds. A load that a
 *		key or click starts ends before the script goes on.
 *
 * Keys: Tab next field, Return sends the form, Backspace, Ctrl-U clears
 * the field, Esc stops a load. Space and b scroll a window when no field
 * has the keyboard, so does the right mouse button. The displays send
 * E_SCROLL for the wheel, the arrows, Page Up and Page Down. Ctrl-Q quits.
 *
 * WRP's form: Archaic sets WRP's settings itself and does not show them.
 * W and H: the picture fills the window. M, T, C: ISMAP, GIF, 2 colors
 * (with -a, the menus for these are shown and used). SL=1: a WRP that
 * knows it sends a picture of the whole page (the tall picture, below),
 * and leaves out its Up and Dn buttons. WRP's buttons (Bk, St, Re, Up,
 * Dn, Bs, Rt) are drawn as pictures: see icons[].
 */
#include "archaic.h"

#ifdef NOPROTO
char *getenv();
void free();
#else
#include <stdlib.h>
#endif

#define M 4		/* margin */
#define PAD 2		/* inside fields, buttons and menus */
#define BPAD 5		/* left and right inside buttons */

int stop;		/* end the load that is running */

static Image imgs[MAXIMG];
static int nimgs;
static char base[MAXURL];	/* this page's URL */
static int winw, winh, viewh;	/* the window, the page part of it */
static int fh, fb, cw;		/* font height, baseline, width of "0" */
static int doch, top;		/* page height, how far scrolled */

/* The tall picture: WRP's picture of a whole page (sl=1), higher than
 * the window. Its item is a frame under WRP's form, tallh high, that
 * shows the page from row oy down; only the frame scrolls. The picture
 * comes in slices as high as the frame: page rows sbase + k * ssize up
 * to the next slice. Archaic keeps NSLICE of them: the ones in the
 * frame, and the next one below and above, which it gets while the user
 * reads (slicework). */
#define NSLICE 4
typedef struct slice {
	Image im;
	long y0;		/* the page row of its first row */
	int state;		/* 0 empty, 1 loading, 2 here */
} Slice;
static Slice slices[NSLICE];
static int tall = -1;		/* its item, or -1 */
static int tallh;		/* the frame's height */
static long oy;			/* the page row at the top of the frame */
static long sbase, ssize;	/* where the slices start, how high */
static int sdir = 1;		/* the last scroll: 1 down, -1 up */
static int sstop;		/* Esc or an error: no slices until a scroll */
static int quiet;		/* getting a slice the user did not ask for */
static char surl[MAXURL];	/* a slice's address */
static char spage[MAXURL];	/* the address WRP showed before */
static char ssave[200];		/* the status line before a slice */
static int focus = -1;		/* field with the keyboard */
static int popup = -1;		/* menu with its list open */
static int pressed = -1;	/* button shown pressed */
static int px, py, pw, ph, prow;	/* the open list */
static char sline[200];		/* status line */
static char msg[200];
static int loading, quit, pending, pendpost, headless, verbose, allset;
static int wrpf[MAXFORM];	/* the form is WRP's */
static char num[16];
static char pendurl[MAXURL], pendbody[MAXURL];
static char lurl[MAXURL], lbody[MAXURL];
static char *script, *pbm;
static long t0;
static char u[MAXURL], q[MAXURL];
static char page[MAXPAGE + 1];
static char stars[MAXVAL];
static char empty[] = "";

static int len(s)
char *s;
{
	int n = 0;

	while (s[n])
		++n;
	return n;
}

static int tw(s, n)
char *s;
int n;
{
	return headless ? n * 6 : g_textw(s, n);
}

/* drawing on the page part of the window: clipped to it */
static void fill(x, y, w, h, c)
int x, y, w, h, c;
{
	if (y < 0) {
		h += y;
		y = 0;
	}
	if (y + h > viewh)
		h = viewh - y;
	if (w > 0 && h > 0)
		g_fill(x, y, w, h, c);
}

static void inv(x, y, w, h)
int x, y, w, h;
{
	if (y < 0) {
		h += y;
		y = 0;
	}
	if (y + h > viewh)
		h = viewh - y;
	if (w > 0 && h > 0)
		g_invert(x, y, w, h);
}

static void text(x, y, s, n)
int x, y, n;
char *s;
{
	if (n > 0 && y >= 0 && y + fh <= viewh)
		g_text(x, y, s, n, 0);
}

static void box(x, y, w, h)
int x, y, w, h;
{
	fill(x, y, w, 1, 1);
	fill(x, y + h - 1, w, 1, 1);
	fill(x, y, 1, h, 1);
	fill(x + w - 1, y, 1, h, 1);
}

static void drawstatus()
{
	int n;

	if (headless)
		return;
	g_fill(0, viewh, winw, winh - viewh, 0);
	g_fill(0, viewh, winw, 1, 1);
	n = len(sline);
	while (n > 0 && tw(sline, n) > winw - 2 * M)
		--n;
	if (n > 0)
		g_text(M, viewh + 2, sline, n, 0);
}

void say(s)
char *s;
{
	scopy(sline, s, (int)sizeof sline);
	if (headless) {
		if (verbose)
			printf("status: %s\n", sline);
		return;
	}
	if (verbose)
		fprintf(stderr, "archaic: %s\n", sline);
	drawstatus();
	g_flush();
}

static void drawfield(i)
int i;
{
	Item *it = &items[i];
	int x = it->x, y = it->y - top, n, k;
	char *s = it->val;

	fill(x + 1, y + 1, it->w - 2, it->h - 2, 0);
	box(x, y, it->w, it->h);
	n = len(s);
	if (it->pass) {
		for (k = 0; k < n; ++k)
			stars[k] = '*';
		s = stars;
	}
	for (k = n; k > 0 && tw(s + n - k, k) > it->w - 2 * PAD - 4; --k)
		;
	text(x + 1 + PAD, y + 1 + PAD, s + n - k, k);
	if (i == focus)
		fill(x + 1 + PAD + tw(s + n - k, k), y + 1 + PAD, 1, fh, 1);
}

/* WRP's buttons get pictures, like the buttons of a real browser. (WRP
 * names them with 2 letters because old browsers can only show text on
 * a button.) Bk back, St stop, Re reload, Up and Dn scroll the page;
 * Bs and Rt are Backspace and Return for the K field. # is black. */
#define IW 13
#define IH 11
static char *icons[] = {
	"Bk",
	".....#.......",
	"....##.......",
	"...###.......",
	"..####.......",
	".############",
	"#############",
	".############",
	"..####.......",
	"...###.......",
	"....##.......",
	".....#.......",
	"St",
	"....#####....",
	"...#.....#...",
	"..#.#####.#..",
	".#.#######.#.",
	".#.#######.#.",
	".#.#######.#.",
	".#.#######.#.",
	".#.#######.#.",
	"..#.#####.#..",
	"...#.....#...",
	"....#####....",
	"Re",
	"....#####..#.",
	"..#######.##.",
	"..###...####.",
	".##.....####.",
	".##....#####.",
	".##..........",
	".##..........",
	".##.......##.",
	"..###...###..",
	"..#########..",
	"....#####....",
	"Up",
	"......#......",
	".....###.....",
	"....#####....",
	"...#######...",
	"..#########..",
	".###########.",
	".....###.....",
	".....###.....",
	".....###.....",
	".....###.....",
	".....###.....",
	"Dn",
	".....###.....",
	".....###.....",
	".....###.....",
	".....###.....",
	".....###.....",
	".###########.",
	"..#########..",
	"...#######...",
	"....#####....",
	".....###.....",
	"......#......",
	"Bs",
	".....########",
	"....#########",
	"...##########",
	"..####.###.##",
	".######.#.###",
	"########.####",
	".######.#.###",
	"..####.###.##",
	"...##########",
	"....#########",
	".....########",
	"Rt",
	"...........##",
	"...........##",
	"...#.......##",
	"..##.......##",
	".###.......##",
	"#############",
	"#############",
	".###.........",
	"..##.........",
	"...#.........",
	".............",
	0
};

/* the picture of button it, or 0: only WRP's buttons (NAME=Fn) have one */
static char **icon(it)
Item *it;
{
	int i;

	if (it->type != I_BUTTON || it->form < 0 || it->form >= MAXFORM ||
		!wrpf[it->form] || !it->name || !strieq(it->name, "Fn") ||
		!it->text)
		return (char **)0;
	for (i = 0; icons[i]; i += IH + 1)
		if (strieq(icons[i], it->text))
			return &icons[i + 1];
	return (char **)0;
}

/* picture p at x, y: drawn only when all of it is on the page part */
static void drawicon(x, y, p)
int x, y;
char **p;
{
	static unsigned char b[IH * ((IW + 7) / 8)];
	int bpl = (IW + 7) / 8, r, c;

	if (y < 0 || y + IH > viewh)
		return;
	for (r = 0; r < IH * bpl; ++r)
		b[r] = 0;
	for (r = 0; r < IH; ++r)
		for (c = 0; c < IW && p[r][c]; ++c)
			if (p[r][c] == '#')
				b[r * bpl + c / 8] |= 0x80 >> (c & 7);
	g_bits(x, y, IW, IH, b, bpl);
}

static void drawbutton(i)
int i;
{
	Item *it = &items[i];
	int x = it->x, y = it->y - top, w = it->w, h = it->h, n;
	char **p = icon(it);

	fill(x, y, w, h, 0);
	box(x, y, w - 1, h - 1);
	fill(x + 1, y + h - 1, w - 1, 1, 1);	/* shadow */
	fill(x + w - 1, y + 1, 1, h - 1, 1);
	if (p)
		drawicon(x + (w - 1 - IW) / 2, y + 1 + (h - 3 - IH) / 2, p);
	else {
		n = len(it->text);
		text(x + (w - 1 - tw(it->text, n)) / 2, y + 1 + PAD, it->text,
			n);
	}
	if (i == pressed)
		inv(x + 1, y + 1, w - 3, h - 3);
}

static void drawselect(i)
int i;
{
	Item *it = &items[i];
	int x = it->x, y = it->y - top, w = it->w, h = it->h, a = h - 2, n, j;
	char *s = it->sel >= 0 ? opts[it->opt + it->sel].text : empty;

	fill(x + 1, y + 1, w - 2, h - 2, 0);
	box(x, y, w, h);
	n = len(s);
	while (n > 0 && tw(s, n) > w - a - 2 * PAD - 3)
		--n;
	text(x + 1 + PAD, y + 1 + PAD, s, n);
	fill(x + w - 1 - a, y + 1, 1, h - 2, 1);	/* the arrow */
	for (j = 0; j < 4; ++j)
		fill(x + w - 1 - a / 2 - 3 + j, y + h / 2 - 2 + j, 7 - 2 * j, 1, 1);
}

/* screen rows r0 up to r1 of the tall picture's frame: the slices that
 * are here (rows still on their way are white) */
static void drawtall(r0, r1)
int r0, r1;
{
	Item *it = &items[tall];
	Slice *s;
	int y = it->y - top, a, b, i, sa, sb;
	long p;

	a = y > r0 ? y : r0;
	if (a < 0)
		a = 0;
	b = y + it->h;
	if (b > r1)
		b = r1;
	if (b > viewh)
		b = viewh;
	for (i = 0; i < NSLICE && b > a; ++i) {
		s = &slices[i];
		if (!s->state || !s->im.bits)
			continue;
		p = y + s->y0 - oy;	/* the screen row of its first row */
		if (p >= b || p + s->im.h <= a)
			continue;
		sa = p > a ? (int)p : a;
		sb = p + s->im.h < b ? (int)(p + s->im.h) : b;
		g_bits(it->x, sa, s->im.w, sb - sa,
			s->im.bits + (sa - p) * s->im.bpl, s->im.bpl);
	}
}

/* screen rows r0 up to r1 of image item i */
static void drawimage(i, r0, r1)
int i, r0, r1;
{
	Item *it = &items[i];
	Image *im = it->img >= 0 ? &imgs[it->img] : 0;
	int y = it->y - top, a, b;

	if (i == tall) {
		drawtall(r0, r1);
		return;
	}
	if (!im || !im->bits) {
		box(it->x, y, it->w, it->h);
		return;
	}
	a = y > r0 ? y : r0;
	if (a < 0)
		a = 0;
	b = y + im->h;
	if (b > r1)
		b = r1;
	if (b > viewh)
		b = viewh;
	if (b > a)
		g_bits(it->x, a, im->w, b - a, im->bits + (long)(a - y) * im->bpl,
			im->bpl);
}

static void drawpop()
{
	Item *it = &items[popup];
	Opt *o;
	int j, n, ty;

	fill(px, py, pw, ph, 0);
	box(px, py, pw, ph);
	for (j = 0; j < it->nopt && 1 + (j + 1) * prow < ph; ++j) {
		o = &opts[it->opt + j];
		ty = py + 1 + j * prow;
		n = len(o->text);
		while (n > 0 && tw(o->text, n) > pw - 2 * PAD - 2)
			--n;
		text(px + 1 + PAD, ty + 1, o->text, n);
		if (o->dis)		/* can't be chosen: a dotted line */
			for (n = 0; n < pw - 2 * PAD - 2; n += 2)
				fill(px + 1 + PAD + n, ty + prow / 2, 1, 1, 1);
		if (j == it->sel)
			inv(px + 1, ty, pw - 2, prow);
	}
}

/* draw the part x, y, w, h of the window again */
static void redraw(rx, ry, rw, rh)
int rx, ry, rw, rh;
{
	Item *it;
	int i, y, r1 = ry + rh;

	if (headless)
		return;
	if (ry < viewh) {
		fill(rx, ry, rw, rh, 0);
		for (i = 0; i < nitems; ++i) {
			it = &items[i];
			y = it->y - top;
			if (it->w <= 0 || it->h <= 0 || y >= r1 || y + it->h <= ry ||
				it->x >= rx + rw || it->x + it->w <= rx)
				continue;
			switch (it->type) {
			case I_TEXT:
				text(it->x, y, it->text, len(it->text));
				if (it->href)
					fill(it->x, y + fb + 1, it->w, 1, 1);
				break;
			case I_FIELD:
				drawfield(i);
				break;
			case I_BUTTON:
				drawbutton(i);
				break;
			case I_SELECT:
				drawselect(i);
				break;
			case I_IMAGE:
				drawimage(i, ry, r1);
				break;
			}
		}
		if (popup >= 0 && ry < py + ph && r1 > py && rx < px + pw &&
			rx + rw > px)
			drawpop();
	}
	if (r1 > viewh)
		drawstatus();
	g_flush();
}

static void drawone(i)
int i;
{
	if (i >= 0 && i < nitems)
		redraw(items[i].x, items[i].y - top, items[i].w, items[i].h);
}

static void openpop(i)
int i;
{
	Item *it = &items[i];
	int j, k, w;

	prow = fh + 2;
	w = it->w;
	for (j = 0; j < it->nopt; ++j) {
		k = tw(opts[it->opt + j].text, len(opts[it->opt + j].text)) + 2 * PAD + 2;
		if (k > w)
			w = k;
	}
	if (w > winw)
		w = winw;
	k = it->nopt;
	if (k * prow + 2 > viewh)
		k = (viewh - 2) / prow;
	pw = w;
	ph = k * prow + 2;
	px = it->x;
	if (px + pw > winw)
		px = winw - pw;
	if (px < 0)
		px = 0;
	py = it->y - top + it->h;
	if (py + ph > viewh)
		py = it->y - top - ph;
	if (py < 0)
		py = viewh - ph;
	if (py < 0)
		py = 0;
	popup = i;
	if (!headless) {
		drawpop();
		g_flush();
	}
}

static void closepop()
{
	if (popup < 0)
		return;
	popup = -1;
	redraw(px, py, pw, ph);
}

static void size(it)
Item *it;
{
	Image *im;
	int j, k, w;

	it->w = it->h = 0;
	switch (it->type) {
	case I_TEXT:
		it->w = tw(it->text, len(it->text));
		it->h = fh;
		break;
	case I_FIELD:
		it->w = it->size * cw + 2 * PAD + 4;
		it->h = fh + 2 * PAD + 2;
		break;
	case I_BUTTON:
		it->w = (icon(it) ? IW : tw(it->text, len(it->text))) +
			2 * BPAD + 3;
		it->h = fh + 2 * PAD + 2;
		break;
	case I_SELECT:
		w = 2 * cw;
		for (j = 0; j < it->nopt; ++j) {
			k = tw(opts[it->opt + j].text, len(opts[it->opt + j].text));
			if (k > w)
				w = k;
		}
		it->h = fh + 2 * PAD + 2;
		it->w = w + 2 * PAD + 3 + it->h - 2;
		break;
	case I_IMAGE:
		im = it->img >= 0 ? &imgs[it->img] : 0;
		if (tall >= 0 && it == &items[tall]) {	/* the tall picture's frame */
			it->w = it->iw;
			it->h = tallh;
		} else if (im && im->w > 0) {
			it->w = im->w;
			it->h = im->h;
		} else {
			it->w = it->iw > 0 ? it->iw : 24;
			it->h = it->ih > 0 ? it->ih : 24;
		}
		break;
	}
}

/* place the items in lines, left to right, as wide as the window */
static void place()
{
	Item *it;
	int i, j, x, y, lh, l0, sp, gap;

	x = y = M;
	lh = l0 = 0;
	gap = 1;		/* no gap needed: nothing since the last one */
	for (i = 0; i <= nitems; ++i) {
		it = i < nitems ? &items[i] : 0;
		if (it)
			size(it);
		if (!it || it->type == I_BR || it->type == I_PARA) {
			if (lh > 0) {
				for (j = l0; j < i; ++j)
					items[j].y = y + (lh - items[j].h) / 2;
				y += lh + 2;
				gap = 0;
			} else if (it && it->type == I_BR && y > M)
				y += fh / 2;
			if (it && it->type == I_PARA && !gap && y > M) {
				y += fh / 2;
				gap = 1;
			}
			if (it)
				it->x = x, it->y = y;
			x = M;
			lh = 0;
			l0 = i + 1;
			continue;
		}
		if (it->w <= 0) {
			it->x = x;
			continue;
		}
		sp = it->sp && x > M ? cw : 0;
		if (x + sp + it->w > winw - M && x > M) {
			for (j = l0; j < i; ++j)
				items[j].y = y + (lh - items[j].h) / 2;
			y += lh + 2;
			gap = 0;
			x = M;
			sp = lh = 0;
			l0 = i;
		}
		it->x = x + sp;
		x = it->x + it->w;
		if (it->h > lh)
			lh = it->h;
	}
	doch = y + M;
}

/* place the items; the tall picture's frame goes down to the bottom of
 * the window, but not past the end of the picture */
static void layout()
{
	Item *it;
	int h;

	tallh = viewh;
	place();
	if (tall < 0)
		return;
	it = &items[tall];
	h = viewh - it->y - M;
	if (h < 50)
		h = 50;
	if (h > it->ih)
		h = it->ih;
	tallh = h;
	place();
}

/* Is it one of WRP's settings that Archaic sets itself? w and h always;
 * m, t and c unless -a. */
static int ours(it)
Item *it;
{
	static char *set[] = { "w", "h", "m", "t", "c" };
	int i;

	if (it->form < 0 || it->form >= MAXFORM || !wrpf[it->form] || !it->name)
		return 0;
	for (i = 0; i < (allset ? 2 : 5); ++i)
		if (strieq(it->name, set[i]))
			return 1;
	return 0;
}

/* A new page: find WRP's form (it has a url and a w field), and hide
 * the settings Archaic sets, each with its one-letter label. */
static void wrpforms()
{
	Item *it;
	int f, i, n;

	for (f = 0; f < nforms && f < MAXFORM; ++f) {
		for (n = i = 0; i < nitems; ++i) {
			it = &items[i];
			if (it->form == f && it->name &&
				(strieq(it->name, "url") || strieq(it->name, "w")))
				++n;
		}
		wrpf[f] = n >= 2;
	}
	for (i = 0; i < nitems; ++i) {
		it = &items[i];
		if (!ours(it))
			continue;
		it->type = I_HIDDEN;
		if (i > 0 && it[-1].type == I_TEXT && it[-1].form == it->form &&
			strieq(it[-1].text, it->name))
			it[-1].type = I_HIDDEN;	/* no name: not sent */
	}
}

/* The size of WRP's next picture: as wide as the window, and as high as
 * the part of it under the form. The picture comes after the form, where
 * the first thing after the form is now. */
static void picsize(w, h)
int *w, *h;
{
	int i, last = -1, y = doch;

	for (i = 0; i < nitems; ++i)
		if (items[i].form >= 0)
			last = i;
	for (i = last + 1; i < nitems; ++i)
		if (items[i].w > 0 && items[i].h > 0) {
			y = items[i].y;
			break;
		}
	*w = winw - 2 * M;
	*h = viewh - y - M;
	if (*h < 50)
		*h = 50;
}

/* the item at page position x, y, or -1 */
static int hit(x, y)
int x, y;
{
	Item *it;
	int i;

	for (i = 0; i < nitems; ++i) {
		it = &items[i];
		if (it->w > 0 && it->h > 0 && it->type != I_BR &&
			it->type != I_PARA && x >= it->x && x < it->x + it->w &&
			y >= it->y && y < it->y + it->h)
			return i;
	}
	return -1;
}

static void setfocus(i)
int i;
{
	int old = focus;

	focus = i;
	drawone(old);
	drawone(i);
}

static void nextfield()
{
	int i, j;

	for (j = 1; j <= nitems; ++j) {
		i = (focus + j) % nitems;
		if (i >= 0 && items[i].type == I_FIELD) {
			setfocus(i);
			return;
		}
	}
}

/* load url next (post: form data to send, or 0) */
static void go(url, post)
char *url, *post;
{
	url_join(base, url, pendurl);
	pendpost = post != 0;
	if (post)
		scopy(pendbody, post, MAXURL);
	pending = 1;
	stop = 1;
}

static void addq(name, val)
char *name, *val;
{
	int n = len(q);

	if (n > 0 && n < MAXURL - 1) {
		q[n++] = '&';
		q[n] = 0;
	}
	url_enc(q, name);
	n = len(q);
	if (n < MAXURL - 1) {
		q[n++] = '=';
		q[n] = 0;
	}
	url_enc(q, val ? val : empty);
}

/* send form f, with button b (-1: Return in a field) */
static void submit(f, b)
int f, b;
{
	Item *it;
	int i, n, w, h;

	if (f < 0 || f >= nforms)
		return;
	q[0] = 0;
	for (i = 0; i < nitems; ++i) {
		it = &items[i];
		if (it->form != f || !it->name || !it->name[0] || ours(it))
			continue;
		if (it->type == I_FIELD || it->type == I_HIDDEN)
			addq(it->name, it->val);
		else if (it->type == I_SELECT && it->sel >= 0)
			addq(it->name, opts[it->opt + it->sel].val);
		else if (it->type == I_BUTTON && i == b)
			addq(it->name, it->text);
	}
	if (f < MAXFORM && wrpf[f]) {
		picsize(&w, &h);
		sprintf(num, "%d", w);
		addq("w", num);
		sprintf(num, "%d", h);
		addq("h", num);
		addq("sl", "1");	/* the whole page, in slices */
		if (!allset) {
			addq("m", "ismap");
			addq("t", "gif");
			addq("c", "2");
		}
	}
	if (b >= 0) {
		pressed = b;
		drawone(b);
	}
	url_join(base, forms[f].action, u);
	if (forms[f].post)
		go(u, q);
	else {
		for (n = 0; u[n] && u[n] != '?'; ++n)
			;
		u[n] = 0;
		if (n < MAXURL - 2) {
			u[n] = '?';
			scopy(u + n + 1, q, MAXURL - n - 1);
		}
		go(u, (char *)0);
	}
}

/* how far the page scrolls: to its end. With a tall picture, only to the
 * end of the frame, if the window is too small for all of it: the form
 * stays at the top, and WRP's last line below the window. */
static int maxtop()
{
	int m;

	if (tall >= 0)
		m = items[tall].y + items[tall].h + M - viewh;
	else
		m = doch - viewh;
	return m > 0 ? m : 0;
}

/* scroll d pixels, down if d > 0. With a tall picture: down, first the
 * picture in its frame, then the page (if the frame does not fit); up,
 * first the page back to its top, then the picture. */
static void scroll(d)
int d;
{
	Item *it;
	long o, ooy = oy;
	int t, otop = top, opop = popup, a, b;

	if (d == 0)
		return;
	sdir = d > 0 ? 1 : -1;
	sstop = 0;
	if (tall >= 0) {
		it = &items[tall];
		if (d < 0 && top > 0) {
			t = top + d < 0 ? 0 : top + d;
			d -= t - top;
			top = t;
		}
		o = oy + d;
		if (o > it->ih - it->h)
			o = it->ih - it->h;
		if (o < 0)
			o = 0;
		d -= (int)(o - oy);
		oy = o;
		if (d < 0)
			d = 0;
	}
	t = top + d;
	if (t > maxtop())
		t = maxtop();
	if (t < 0)
		t = 0;
	top = t;
	if (top == otop && oy == ooy)
		return;
	popup = -1;
	if (top == otop && opop < 0) {	/* only the frame changed */
		it = &items[tall];
		a = it->y - top;
		b = a + it->h;
		if (a < 0)
			a = 0;
		if (b > viewh)
			b = viewh;
		redraw(0, a, winw, b - a);
	} else
		redraw(0, 0, winw, viewh);
}

/* a window's worth of scrolling: the frame (or the window) less 2 lines */
static int pagestep()
{
	int n = (tall >= 0 ? items[tall].h : viewh) - 2 * fh;

	return n > fh ? n : fh;
}

/* after a layout: keep oy inside the picture; when the frame's height
 * changed, slices of the new height, one starting at the top of the frame */
static void tallfix()
{
	Item *it;

	if (tall < 0)
		return;
	it = &items[tall];
	if (oy > it->ih - it->h)
		oy = it->ih - it->h;
	if (oy < 0)
		oy = 0;
	if (ssize != it->h) {
		ssize = it->h;
		sbase = oy % ssize;
	}
}

static void key(k)
int k;
{
	Item *it;
	int n;

	if (k == 'Q' - 64) {			/* Ctrl-Q */
		quit = stop = 1;
		return;
	}
	if (popup >= 0) {
		closepop();
		return;
	}
	if (k == 033) {
		if (loading) {
			stop = 1;
			say("Stopping");
		} else
			setfocus(-1);
		return;
	}
	if (k == '\t') {
		nextfield();
		return;
	}
	if (focus < 0) {
		if (k == ' ' || k == 'F' - 64)
			scroll(pagestep());
		else if (k == 'b' || k == 'B' - 64)
			scroll(-pagestep());
		return;
	}
	it = &items[focus];
	n = len(it->val);
	if (k == '\b' || k == 0177) {
		if (n == 0)
			return;
		it->val[n - 1] = 0;
	} else if (k == 'U' - 64)
		it->val[0] = 0;
	else if (k == '\r' || k == '\n') {
		submit(it->form, -1);
		return;
	} else if (k >= ' ' && k < 0400 && n < MAXVAL - 1) {
		it->val[n] = k;
		it->val[n + 1] = 0;
	} else
		return;
	drawone(focus);
}

static void click(x, y, b)
int x, y, b;
{
	Item *it;
	int i, j;

	if (popup >= 0) {
		i = popup;
		it = &items[i];
		if (x >= px && x < px + pw && y > py && y < py + ph - 1) {
			j = (y - py - 1) / prow;
			if (j >= 0 && j < it->nopt && !opts[it->opt + j].dis)
				it->sel = j;
		}
		closepop();
		drawone(i);
		return;
	}
	if (y >= viewh)
		return;
	if (b == 3) {
		scroll(y < viewh / 2 ? -pagestep() : pagestep());
		return;
	}
	i = hit(x, y + top);
	if (i < 0) {
		if (focus >= 0)
			setfocus(-1);
		return;
	}
	it = &items[i];
	switch (it->type) {
	case I_FIELD:
		setfocus(i);
		break;
	case I_BUTTON:
		if (it->form >= 0)
			submit(it->form, i);
		break;
	case I_SELECT:
		if (it->nopt > 0)
			openpop(i);
		break;
	case I_IMAGE:
		if (!it->href)
			break;
		if (it->ismap) {	/* the tall picture: rows of the whole page */
			sprintf(u, "%.1500s?%d,%ld", it->href, x - it->x,
				y + top - it->y + (i == tall ? oy : 0L));
			go(u, (char *)0);
		} else
			go(it->href, (char *)0);
		break;
	case I_TEXT:
		if (it->href)
			go(it->href, (char *)0);
		break;
	}
}

static void event(e)
Event *e;
{
	int n;

	switch (e->type) {
	case E_KEY:
		key(e->key);
		break;
	case E_DOWN:
		click(e->x, e->y, e->key);
		break;
	case E_EXPOSE:
		redraw(e->x, e->y, e->w, e->h);
		break;
	case E_SIZE:
		if (e->w == winw && e->h == winh)
			break;
		winw = e->w;
		winh = e->h;
		viewh = winh - fh - 3;
		popup = -1;
		layout();
		tallfix();
		if (top > maxtop())
			top = maxtop();
		if (top < 0)
			top = 0;
		redraw(0, 0, winw, winh);
		break;
	case E_SCROLL:
		n = tall >= 0 ? items[tall].h : viewh;
		if (e->key == S_PAGE)
			scroll(e->y * pagestep());
		else if (e->key == S_STEP)
			scroll(e->y * (n / 4));
		else
			scroll(e->y);
		break;
	case E_QUIT:
		quit = stop = 1;
		break;
	}
}

/* the network is waiting: handle the user meanwhile. 1 = stop the load */
int ui_poll()
{
	static Event e;

	if (!headless)
		while (!stop && g_event(&e, 0))
			event(&e);
	return stop;
}

/* rows y up to y + n - 1 of im have come */
void img_rows(im, y, n)
Image *im;
int y, n;
{
	Item *it;
	int i, a;
	long p;

	http_pump();
	if (headless)
		return;
	for (i = 0; i < nitems; ++i) {
		it = &items[i];
		if (it->type != I_IMAGE || it->img < 0 || &imgs[it->img] != im ||
			i == tall)
			continue;
		a = it->y - top + y;
		drawimage(i, a, a + n);
		if (popup >= 0 && a < py + ph && a + n > py)
			drawpop();
	}
	for (i = 0; tall >= 0 && i < NSLICE; ++i) {
		if (im != &slices[i].im)
			continue;
		p = items[tall].y - top + slices[i].y0 - oy + y;
		if (p < viewh && p + n > 0) {
			a = p < 0 ? 0 : (int)p;
			drawtall(a, p + n < viewh ? (int)(p + n) : viewh);
			if (popup >= 0 && a < py + ph && p + n > py)
				drawpop();
		}
	}
	if (quiet) {
		g_flush();
		return;
	}
	if (http_len > 0)
		sprintf(msg, "Picture %ld of %ld bytes", http_got, http_len);
	else
		sprintf(msg, "Picture %ld bytes", http_got);
	say(msg);
}

static void freeimgs()
{
	int i;

	for (i = 0; i < nimgs; ++i) {
		if (imgs[i].bits)
			free((char *)imgs[i].bits);
		imgs[i].bits = 0;
		imgs[i].w = imgs[i].h = 0;
	}
	nimgs = 0;
}

static void freeslices()
{
	Slice *s;

	for (s = slices; s < slices + NSLICE; ++s) {
		if (s->im.bits)
			free((char *)s->im.bits);
		s->im.bits = 0;
		s->im.w = s->im.h = 0;
		s->state = 0;
	}
}

static void loadimg(i)
int i;
{
	Item *it = &items[i];
	Image *im = &imgs[it->img];

	url_join(base, it->text, u);
	sprintf(msg, "Picture %.150s", u);
	say(msg);
	if (http_open(u, (char *)0) < 0) {
		say(http_err);
		return;
	}
	if (http_status != 200) {
		sprintf(msg, "Picture: HTTP %d", http_status);
		say(msg);
		http_close();
		return;
	}
	if (gif_load(im) < 0 && !stop) {
		sprintf(msg, "Picture: can't show %s", http_type[0] ? http_type : "it");
		say(msg);
	}
	http_close();
	if (im->w > 0 && (im->w != it->w || im->h != it->h)) {
		layout();
		redraw(0, 0, winw, winh);
	}
}

/* the slice k of the tall picture: page rows *a up to *b; 0 if none */
static int srows(k, a, b)
long k, *a, *b;
{
	*a = sbase + k * ssize;
	*b = *a + ssize;
	if (*a < 0)
		*a = 0;
	if (*b > items[tall].ih)
		*b = items[tall].ih;
	return *b > *a;
}

/* the slice that has page row r */
static long sk(r)
long r;
{
	return r < sbase ? -1L : (r - sbase) / ssize;
}

/* is slot i the slice k? */
static int sis(i, k)
int i;
long k;
{
	long a, b;

	return slices[i].state == 2 && srows(k, &a, &b) &&
		slices[i].y0 == a && slices[i].y0 + slices[i].im.h == b;
}

/* how far slot i is from the frame (0: it shows in it) */
static long sdist(i)
int i;
{
	Slice *s = &slices[i];
	long e = oy + items[tall].h;

	if (s->y0 >= e)
		return s->y0 - e + 1;
	if (s->y0 + s->im.h <= oy)
		return oy - s->y0 - s->im.h + 1;
	return 0;
}

/* get page rows a up to b of the tall picture into slot i */
static void getslice(i, a, b)
int i;
long a, b;
{
	Slice *s = &slices[i];
	int n, c, ok = 0;

	if (s->im.bits)
		free((char *)s->im.bits);
	s->im.bits = 0;
	s->im.w = s->im.h = 0;
	s->y0 = a;
	s->state = 1;
	url_join(base, items[tall].text, surl);
	for (n = 0; surl[n] && surl[n] != '?'; ++n)
		;
	c = surl[n] ? '&' : '?';
	n = len(surl);
	scopy(ssave, sline, (int)sizeof ssave);
	loading = 1;
	if (n > MAXURL - 30)
		say("Picture: the address is too long");
	else {
		sprintf(surl + n, "%cy=%ld&h=%ld", c, a, b - a);
		if (http_open(surl, (char *)0) < 0)
			say(http_err);
		else {
			if (http_status != 200) {
				sprintf(msg, "Picture: HTTP %d", http_status);
				say(msg);
			} else if (gif_load(&s->im) < 0) {
				if (!stop)
					say("Picture: can't show the slice");
			} else if (s->im.h != b - a)	/* the server ignored y and h */
				say("Picture: WRP did not send the rows asked for");
			else
				ok = 1;
			http_close();
		}
	}
	loading = 0;
	if (ok && !stop) {
		s->state = 2;
		if (!quiet)
			say(ssave);
		return;
	}
	if (s->im.bits)
		free((char *)s->im.bits);
	s->im.bits = 0;
	s->im.w = s->im.h = 0;
	s->state = 0;
	if (!pending)		/* Esc, or an error: wait for a scroll */
		sstop = 1;
	if (stop && !pending)
		say("Stopped");
}

/* get one slice: one the frame shows that is missing, else (near) the
 * next one below or above. 1 if it got one, or tried */
static int slicework(near)
int near;
{
	static long want[4];
	long a, b, k0, k1;
	int i, j, k, n, v, best;

	if (tall < 0 || ssize <= 0 || sstop)
		return 0;
	k0 = sk(oy);
	k1 = sk(oy + items[tall].h - 1);
	n = 0;
	want[n++] = k0;
	if (k1 != k0)
		want[n++] = k1;
	v = n;			/* want[0] up to want[v - 1] are in the frame */
	if (near) {
		want[n++] = sdir > 0 ? k1 + 1 : k0 - 1;
		want[n++] = sdir > 0 ? k0 - 1 : k1 + 1;
	}
	for (i = 0; i < n; ++i) {
		if (!srows(want[i], &a, &b))
			continue;
		for (j = 0; j < NSLICE && !sis(j, want[i]); ++j)
			;
		if (j < NSLICE)
			continue;	/* here */
		/* a slot: an empty one, else the farthest one not wanted */
		best = -1;
		for (j = 0; j < NSLICE; ++j) {
			if (!slices[j].state) {
				best = j;
				break;
			}
			for (k = 0; k < n && !sis(j, want[k]); ++k)
				;
			if (k == n && (best < 0 || sdist(j) > sdist(best)))
				best = j;
		}
		if (best < 0)
			return 0;
		quiet = i >= v;
		getslice(best, a, b);
		quiet = 0;
		return 1;
	}
	return 0;
}

/* the address in WRP's form (the page WRP shows), or "" */
static char *wrpurl()
{
	Item *it;
	int i;

	for (i = 0; i < nitems; ++i) {
		it = &items[i];
		if (it->type == I_FIELD && it->form >= 0 && it->form < MAXFORM &&
			wrpf[it->form] && it->name && strieq(it->name, "url") &&
			it->val)
			return it->val;
	}
	return empty;
}

/* WRP's ISMAP picture on a page with WRP's form. It is the tall picture
 * if it is higher than its frame: load() sees after the layout. */
static void findtall()
{
	Item *it;
	int f, i;

	tall = -1;
	for (f = 0; f < nforms && f < MAXFORM && !wrpf[f]; ++f)
		;
	if (f >= nforms || f >= MAXFORM)
		return;
	for (i = 0; i < nitems; ++i) {
		it = &items[i];
		if (it->type == I_IMAGE && it->ismap && it->href && it->iw > 0 &&
			it->ih > 0) {
			tall = i;
			return;
		}
	}
}

static void load(url, post)
char *url, *post;
{
	Item *it;
	static char fname[40];
	int i, n, c;

	scopy(lurl, url, MAXURL);
	if (post)
		scopy(lbody, post, MAXURL);
	loading = 1;
	stop = 0;
	t0 = now();
	sprintf(msg, "Loading %.150s", lurl);
	say(msg);
	if (http_open(lurl, post ? lbody : (char *)0) < 0) {
		say(http_err);
		goto out;
	}
	if (strnieq(http_type, "image/", 6)) {
		/* a picture alone: a page with just that picture */
		http_close();
		sprintf(page, "<TITLE>%.150s</TITLE><IMG SRC=\"%.1500s\">", http_url, http_url);
		n = len(page);
	} else if (http_type[0] && !strnieq(http_type, "text/", 5)) {
		sprintf(msg, "Not shown: %.60s (HTTP %d)", http_type, http_status);
		say(msg);
		http_close();
		goto out;
	} else {
		n = 0;
		while ((c = http_getc()) >= 0) {
			if (n < MAXPAGE)
				page[n++] = c;
			if ((http_got & 1023) == 0) {
				sprintf(msg, "Page %ld bytes", http_got);
				say(msg);
			}
		}
		http_close();
		if (stop) {
			if (!pending)
				say(http_err[0] ? http_err : "Stopped");
			goto out;
		}
		page[n] = 0;
	}
	/* the new page */
	fname[0] = 0;
	if (focus >= 0 && items[focus].name)
		scopy(fname, items[focus].name, (int)sizeof fname);
	scopy(spage, wrpurl(), MAXURL);
	scopy(base, http_url, MAXURL);
	focus = popup = pressed = -1;
	top = 0;
	tall = -1;
	freeimgs();
	freeslices();
	if (html_parse(page, n) < 0)
		say("The page is too big: only its start is shown");
	for (i = 0; i < nitems; ++i)
		if (items[i].type == I_IMAGE && nimgs < MAXIMG)
			items[i].img = nimgs++;
	if (!headless)
		g_title(title[0] ? title : "Archaic");
	wrpforms();
	findtall();
	layout();
	if (tall >= 0 && items[tall].ih <= items[tall].h) {
		tall = -1;	/* it fits: an ordinary picture */
		layout();
	}
	if (tall < 0 || !spage[0] || !strieq(spage, wrpurl()))
		oy = 0;		/* a new page: its top. The same page: stay */
	ssize = 0;
	sstop = 0;
	tallfix();
	for (i = 0; fname[0] && i < nitems; ++i) {
		it = &items[i];
		if (it->type == I_FIELD && it->name && strieq(it->name, fname)) {
			focus = i;
			break;
		}
	}
	redraw(0, 0, winw, winh);
	for (i = 0; i < nitems && !stop; ++i)
		if (items[i].type == I_IMAGE && items[i].img >= 0 && i != tall)
			loadimg(i);
	while (!stop && slicework(0))	/* the frame of the tall picture */
		;
	if (stop) {
		if (!pending)
			say("Stopped");
		goto out;
	}
	sprintf(msg, "%ld s  %.150s", now() - t0, base);
	say(msg);
out:
	loading = 0;
	if (pressed >= 0) {
		i = pressed;
		pressed = -1;
		drawone(i);
	}
}

static unsigned long sum(im)
Image *im;
{
	unsigned long s = 0;
	long i, n;

	if (!im->bits)
		return 0;
	n = (long)im->bpl * im->h;
	for (i = 0; i < n; ++i)
		s = (s * 31 + im->bits[i]) & 0xffffffffL;
	return s;
}

/* -t: what the page is made of */
static void dump()
{
	static char *tn[] = { "?", "text", "br", "para", "field", "button",
		"hidden", "select", "image" };
	Item *it;
	Image *im;
	Slice *s;
	Opt *o;
	int i, j;

	printf("page %s\n", base);
	printf("title \"%s\", %d items, %d forms; status: %s\n", title, nitems,
		nforms, sline);
	for (i = 0; i < nforms; ++i)
		printf("form %d: %s \"%s\"\n", i, forms[i].post ? "POST" : "GET",
			forms[i].action);
	for (i = 0; i < nitems; ++i) {
		it = &items[i];
		printf("%3d %-6s %3d %3d %3d %3d", i, tn[it->type], it->x, it->y,
			it->w, it->h);
		if (it->form >= 0)
			printf(" f%d", it->form);
		if (it->name)
			printf(" %s", it->name);
		if (it->val)
			printf(" =\"%s\"", it->val);
		if (it->type == I_TEXT || it->type == I_BUTTON || it->type == I_IMAGE)
			printf(" \"%s\"", it->text);
		if (icon(it))
			printf(" icon");
		if (it->type == I_SELECT) {
			printf(" sel %d:", it->sel);
			for (j = 0; j < it->nopt; ++j) {
				o = &opts[it->opt + j];
				printf(" %s%s=%s", o->dis ? "-" : "", o->val, o->text);
			}
		}
		if (it->href)
			printf(" href=%s", it->href);
		if (it->ismap)
			printf(" ismap");
		if (i == tall) {	/* the slice at the top of the frame */
			for (j = 0; j < NSLICE; ++j) {
				s = &slices[j];
				if (s->state == 2 && s->y0 <= oy && oy < s->y0 + s->im.h)
					printf(" got %dx%d sum %08lx", s->im.w, s->im.h,
						sum(&s->im));
			}
			printf(" tall %dx%d at %ld", it->iw, it->ih, oy);
		} else if (it->type == I_IMAGE && it->img >= 0) {
			im = &imgs[it->img];
			printf(" got %dx%d sum %08lx", im->w, im->h, sum(im));
		}
		printf("\n");
	}
	fflush(stdout);
}

static void writepbm()
{
	FILE *f;
	Image *im = 0;
	int i;

	for (i = 0; i < nimgs && !im; ++i)
		if (imgs[i].bits)
			im = &imgs[i];
	for (i = 0; i < NSLICE && !im; ++i)	/* the tall picture's first slice */
		if (slices[i].state == 2 && slices[i].y0 <= oy &&
			oy < slices[i].y0 + slices[i].im.h)
			im = &slices[i].im;
	if (!im) {
		fprintf(stderr, "archaic: no picture for %s\n", pbm);
		return;
	}
#ifdef MSDOS
	f = fopen(pbm, "wb");
#else
	f = fopen(pbm, "w");
#endif
	if (!f) {
		fprintf(stderr, "archaic: can't write %s\n", pbm);
		return;
	}
	fprintf(f, "P4\n%d %d\n", im->w, im->h);
	fwrite((char *)im->bits, 1, (int)((long)im->bpl * im->h), f);
	fclose(f);
}

/* one step of the -s script */
static void runscript()
{
	static Event e;
	long t;
	int c;

	c = *script++ & 0377;
	e.type = E_KEY;
	e.key = c;
	if (c == '@') {
		e.type = E_DOWN;
		e.x = e.y = 0;
		e.key = 1;
		while (*script >= '0' && *script <= '9')
			e.x = e.x * 10 + *script++ - '0';
		if (*script == ',')
			++script;
		while (*script >= '0' && *script <= '9')
			e.y = e.y * 10 + *script++ - '0';
		if (*script == ';')
			++script;
	} else if (c == '~')
		e.key = '\r';
	else if (c == '%') {
		for (t = now() + 2; now() < t;)
			ui_poll();
		return;
	}
	event(&e);
}

int main(argc, argv)
int argc;
char **argv;
{
	static Event e;
	char *a, *url = 0, *geom = 0;
	int i, n;

	for (i = 1; i < argc; ++i) {
		a = argv[i];
		if (a[0] == '=')
			geom = a;
		else if (a[0] == '-' && a[1] && !a[2] &&
			(a[1] == 'a' || a[1] == 't' || a[1] == 'v')) {
			if (a[1] == 'a')
				allset = 1;
			else if (a[1] == 't')
				headless = 1;
			else
				verbose = 1;
		} else if (a[0] == '-' && a[1] && !a[2] && i + 1 < argc &&
			(a[1] == 'o' || a[1] == 's' || a[1] == 'g')) {
			if (a[1] == 'o')
				pbm = argv[++i];
			else if (a[1] == 's')
				script = argv[++i];
			else
				geom = argv[++i];
		} else if (a[0] == '-') {
			fprintf(stderr, "usage: archaic [=WxH+X+Y] [-a] [-t] [-v] [-o file.pbm] [-s script] [url]\n");
			return 2;
		} else
			url = a;
	}
	if (!url)
		url = getenv("WRP");
	if (!url || !url[0])
		url = DEFURL;
	/* http://host:port/ */
	u[0] = 0;
	if (!strnieq(url, "http://", 7))
		scopy(u, "http://", MAXURL);
	n = len(u);
	scopy(u + n, url, MAXURL - n);
	for (i = 7; u[i] && u[i] != '/'; ++i)
		;
	if (!u[i])
		scopy(u + i, "/", MAXURL - i);
	scopy(base, u, MAXURL);
	if (headless) {
		winw = 640;
		winh = 400;
		fh = 13;
		fb = 10;
	} else {
		if (g_open(geom, &winw, &winh) < 0) {
			fprintf(stderr, "archaic: can't open the display\n");
			return 1;
		}
		g_font(&fh, &fb);
	}
	cw = tw("0", 1);
	viewh = winh - fh - 3;
	sprintf(msg, "Archaic %s", VERSION);
	say(msg);
	go(u, (char *)0);
	while (!quit) {
		if (pending) {
			pending = 0;
			load(pendurl, pendpost ? pendbody : (char *)0);
			if (headless)
				dump();
			continue;
		}
		stop = 0;
		if (script) {
			if (!*script)
				break;
			if (!headless)
				while (g_event(&e, 0))
					event(&e);
			runscript();
			continue;
		}
		if (headless)
			break;
		if (slicework(1))	/* the frame's slices, then the next ones */
			continue;
		if (g_event(&e, 1))
			event(&e);
	}
	if (pbm)
		writepbm();
	if (!headless)
		g_close();
	return 0;
}
