/* main.c - Archaic: the page on the screen, the mouse and the keys.
 *
 *	archaic [=WxH+X+Y] [-a] [-c n] [-f fmt] [-t] [-v] [-o file]
 *		[-s script] [url]
 *
 * url		WRP's address: host:port or http://host:port/
 *		(default $WRP, else DEFURL)
 * =WxH+X+Y	window size and place (or -g WxH+X+Y)
 * -a		show WRP's M, T and C menus (mode, picture type, colors)
 * -c n		start with pictures of n colors (2 to 256), not as many
 *		as the display shows (g_colors). With -t: as if the
 *		display showed n
 * -f fmt	gif: ask WRP for GIFs only. abm (as without -f): WRP may
 *		send its black and white pictures as ABM (see pic.c). To
 *		compare the two on a computer
 * -t		test: no window; print the page and its pictures
 * -v		status messages to stderr too
 * -o file	at the end, write the first picture: PBM, or PPM if it
 *		has colors
 * -s script	act out a script, then quit: characters are keys, ~ is
 *		Return, @x,y; is a click, % waits 2 seconds. A load that a
 *		key or click starts ends before the script goes on (so do
 *		the slices of the tall picture that a scroll needs).
 *
 * Keys: Tab next field, Return sends the form, Backspace, Ctrl-U clears
 * the field, Esc stops a load. Space and b scroll a window when no field
 * has the keyboard, so does the right mouse button. The displays send
 * E_SCROLL for the wheel, the arrows, Page Up and Page Down, and for the
 * system's scroll bar (g_sbar). Where the system has none, Archaic draws
 * its own (drawbar). Ctrl-Q quits.
 *
 * WRP's K field: its text goes to the web page as keys when the form is
 * sent. WRP's Bs and Rt buttons (Backspace, Return) send that key to the
 * page; with text in K, Archaic sends the text and then the key, as keys
 * (WRP alone would send only the button's key). WRP's url field has a
 * clear button (x) at its right end. While Archaic loads what the user
 * asked for, a spinner turns at the left end of the status line.
 *
 * WRP's errors: WRP answers with a page that has its form. An answer
 * from it with no form is an error, like "Unable to find map" (WRP made
 * a new picture since, and forgot the map of the old one). WRP's page
 * then stays, with the error on the status line, so its buttons (back)
 * still work. After a click on an old picture, Archaic gets WRP's new
 * one at once (St). While WRP's next page comes, a click on the old
 * picture does nothing: its map is about to go.
 *
 * A new window width: once the size has stayed the same for 0.4 s,
 * Archaic sends WRP's form with St and the new width, so WRP makes the
 * picture again, as wide as the window (St: the page is not loaded
 * again). The frame keeps its place on the page, in proportion.
 *
 * WRP's form: Archaic sets WRP's settings itself and does not show them.
 * W and H: the picture fills the window. M, T: ISMAP, GIF (with -a, the
 * menus for M, T and C are shown and used). C, the colors: on a display
 * that shows colors, WRP's C menu stays, set to the display's colors (or
 * the nearest it has, or to -c's); a new choice there sends the form at
 * once, with St, so that WRP takes the picture again in those colors.
 * On a black and white display, C is hidden: 2, or -c's. D (what WRP
 * dithers in a black and white picture: all, only photos, or nothing; a
 * WRP of November 2026 has it) stays too, and a new choice there also
 * sends the form at once. SL=1: a WRP that knows it sends a picture of
 * the whole page (the tall picture, below), and leaves out its Up and Dn
 * buttons. WRP's buttons (Bk, St, Re, Up,
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
static int winw, winh;		/* the window */
static int vieww, viewh;	/* its page part: less the scroll bar, the status line */
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

/* Archaic's own scroll bar, for systems that have none (g_bar), in the
 * style of the Atari ST's GEM: at the right of the page part, an arrow
 * box at each end, between them a dotted shaft with a white elevator.
 * The elevator's length shows how much of the page is in the window,
 * its place where. An arrow scrolls a quarter of the window, a click in
 * the shaft a window; held down, they repeat. The elevator is dragged. */
#define SBW 17			/* the bar's width: a line and 16 pixels */
#define SBH 16			/* an arrow box's height, less its line */
#define B_UP 1			/* the parts of the bar */
#define B_DN 2
#define B_PGUP 3
#define B_PGDN 4
#define B_DRAG 5
#define AW 11			/* the arrow */
#define AH 11
static char *arrow[] = {
	".....#.....",
	"....#.#....",
	"...#...#...",
	"..#.....#..",
	".#.......#.",
	"####...####",
	"...#...#...",
	"...#...#...",
	"...#...#...",
	"...#...#...",
	"...#####..."
};
static int bar;			/* Archaic draws the scroll bar */
static int bpart;		/* the part held down, or 0 */
static int bticks;		/* waits of 1/10 s since it was pressed */
static int bpx, bpy;		/* the pointer while it is held */
static int by0, bgrab;		/* B_DRAG: the row pressed, its row in the elevator */
static long bpos0;		/* B_DRAG: the page's place then */
static long lpos = -1, lmax, lvis;	/* what g_sbar showed last */
static int focus = -1;		/* field with the keyboard */
static int popup = -1;		/* menu with its list open */
static int pressed = -1;	/* button shown pressed */
static int px, py, pw, ph, prow;	/* the open list */
static char sline[200];		/* status line */
static long pgot, plen;		/* its progress bar: pgot of plen bytes, 0 none */
static int pdone;		/* the bar's black part, as drawn */
#define SPW 13			/* the spinner: SPW x SPW, left of the status */
static int spx[] = { 6, 9, 10, 9, 6, 3, 2, 3 };	/* its 8 dots, clockwise */
static int spy[] = { 2, 3, 6, 9, 10, 9, 6, 3 };
static int spinon;		/* it turns: a load the user asked for runs */
static int spinf;		/* its step, 0 to 7: the dot that leads */
static char what[60];		/* what is coming: "Page", "Picture 1 of 2" ... */
static int phase;		/* what http_open() does with it (H_), 0 none */
static long tphase, tshown;	/* when the phase began; the second shown */
static long lbytes;		/* the bytes of this page so far */
static char nb[24];
static char msg[200];
static int loading, quit, pending, pendpost, headless, verbose, allset;
static long dcolors;		/* the colors the display shows (g_colors) */
static int ncol;		/* the colors asked of WRP, 2 to 256 */
static int copt;		/* -c's colors, or 0 */
static int cmenu;		/* WRP's C menu is shown */
static int idepth = 1;		/* the pictures' pixels: 1 or 8 bits */
static int wrpf[MAXFORM];	/* the form is WRP's */
static int oldpic;		/* WRP's next page comes: the picture shown is old */
static int pclick;		/* the pending load is a click on WRP's picture */
static char *note;		/* the next load's last status line, or 0 */
static int resz;		/* the window's width changed: a new picture comes */
static long rest;		/* g_ms() at that change */
static int pwide;		/* the pending load is that new picture */
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

/* the number at the start of s */
static int atoin(s)
char *s;
{
	int n = 0;

	while (s && *s >= '0' && *s <= '9' && n < 10000)
		n = n * 10 + *s++ - '0';
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
	if (x + w > vieww)
		w = vieww - x;
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
	if (x + w > vieww)
		w = vieww - x;
	if (w > 0 && h > 0)
		g_invert(x, y, w, h);
}

/* text: whole lines only, and only the letters left of the scroll bar */
static void text(x, y, s, n)
int x, y, n;
char *s;
{
	if (y < 0 || y + fh > viewh)
		return;
	while (n > 0 && x + tw(s, n) > vieww)
		--n;
	if (n > 0)
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

/* the progress bar at the right of the status line: its width */
static int pbarw()
{
	return winw / 5 < 100 ? winw / 5 : 100;
}

/* its black part, for an inside w wide */
static int pblack(w)
int w;
{
	long a = pgot, b = plen;

	if (b <= 0 || a <= 0)
		return 0;
	if (a > b)
		a = b;
	while (b > 100000L) {	/* a * w must fit in a long */
		a >>= 4;
		b >>= 4;
	}
	return (int)(a * w / b);
}

/* the bar: all of it, or (all 0) only the black part that changed */
static void drawprog(all)
int all;
{
	int w = pbarw(), x = winw - M - w, y = viewh + 3, h = fh - 2, b;

	if (all) {
		g_fill(x, y, w, h, 1);
		g_fill(x + 1, y + 1, w - 2, h - 2, 0);
		pdone = 0;
	}
	b = pblack(w - 4);
	if (b > pdone)
		g_fill(x + 2 + pdone, y + 2, b - pdone, h - 4, 1);
	else if (b < pdone)
		g_fill(x + 2 + b, y + 2, pdone - b, h - 4, 0);
	pdone = b;
}

static void dot(b, x, y)
unsigned char *b;
int x, y;
{
	b[y * 2 + x / 8] |= 0x80 >> (x & 7);
}

/* the spinner at the left end of the status line: 8 dots in a circle,
 * the one that leads big, the 2 behind it smaller */
static void drawspin()
{
	static unsigned char b[SPW * 2];
	int j, a, x, y;

	if (headless)
		return;
	for (j = 0; j < SPW * 2; ++j)
		b[j] = 0;
	for (j = 0; spinon && j < 8; ++j) {
		a = (spinf - j + 8) & 7;	/* 0 leads */
		x = spx[j];
		y = spy[j];
		dot(b, x, y);
		if (a <= 2) {
			dot(b, x - 1, y);
			dot(b, x + 1, y);
			dot(b, x, y - 1);
			dot(b, x, y + 1);
		}
		if (a == 0) {
			dot(b, x - 1, y - 1);
			dot(b, x + 1, y - 1);
			dot(b, x - 1, y + 1);
			dot(b, x + 1, y + 1);
		}
	}
	g_bits(M, viewh + 2 + (fh - SPW) / 2, SPW, SPW, b, 2);
}

/* the spinner a step on: 8 steps a second by the display's clock, or,
 * if it has none, a step each time the network waits */
static void spin()
{
	long t;
	int f;

	if (!spinon || headless)
		return;
	t = g_ms();
	f = t < 0 ? (spinf + 1) & 7 : (int)((t / 125) & 7);
	if (f == spinf)
		return;
	spinf = f;
	drawspin();
	g_flush();
}

static void drawstatus()
{
	int n, x = M, w = winw - 2 * M;

	if (headless)
		return;
	g_fill(0, viewh, winw, winh - viewh, 0);
	g_fill(0, viewh, winw, 1, 1);
	if (spinon) {		/* the text goes right of it */
		drawspin();
		x += SPW + M;
		w -= SPW + M;
	}
	if (plen > 0) {
		drawprog(1);
		w -= pbarw() + M;
	}
	n = len(sline);
	while (n > 0 && tw(sline, n) > w)
		--n;
	if (n > 0)
		g_text(x, viewh + 2, sline, n, 0);
}

/* the spinner starts (on 1) or stops */
static void spinto(on)
int on;
{
	if (on == spinon)
		return;
	spinon = on;
	if (!headless) {
		drawstatus();
		g_flush();
	}
}

/* the status line to the screen, and with -v to the log */
static void showstatus()
{
	if (headless) {
		if (verbose)
			printf("status: %s\n", sline);
		return;
	}
	if (verbose) {
		fprintf(stderr, "archaic: %s\n", sline);
		fflush(stderr);		/* SunOS 3 buffers it in a file */
	}
	drawstatus();
	g_flush();
}

void say(s)
char *s;
{
	plen = 0;
	scopy(sline, s, (int)sizeof sline);
	showstatus();
}

static int same(a, b)
char *a, *b;
{
	while (*a && *a == *b) {
		++a;
		++b;
	}
	return *a == *b;
}

/* status s with the progress bar at got of n bytes (n <= 0: no bar).
 * Only what changed is drawn again. */
static void sayp(s, got, n)
char *s;
long got, n;
{
	if (n < 0)
		n = 0;
	if (same(s, sline) && (n > 0) == (plen > 0)) {
		pgot = got;
		plen = n;
		if (n > 0 && !headless && pblack(pbarw() - 4) != pdone) {
			drawprog(0);
			g_flush();
		}
		return;
	}
	pgot = got;
	plen = n;
	scopy(sline, s, (int)sizeof sline);
	showstatus();
}

/* n bytes, for the status line */
static char *bytes(n)
long n;
{
	if (n < 1024)
		sprintf(nb, "%ld bytes", n);
	else
		sprintf(nb, "%ld KB", (n + 512) / 1024);
	return nb;
}

/* what is coming, and how much of it has come */
static void progress()
{
	long g = http_got, n = http_len;

	if (!what[0])
		return;
	if (n > 0 && n < 1024)
		sprintf(msg, "%.50s: %ld of %ld bytes", what, g, n);
	else if (n > 0)
		sprintf(msg, "%.50s: %ld of %ld KB", what, (g + 512) / 1024,
			(n + 512) / 1024);
	else
		sprintf(msg, "%.50s: %s", what, bytes(g));
	sayp(msg, g, n);
}

/* http.c: what http_open() does now (H_CONNECT, H_WAIT, H_BODY; 0 at
 * the end). ui_poll() counts the seconds of the wait. */
void ui_note(p)
int p;
{
	phase = p;
	tphase = now();
	tshown = 0;
	if (!p || !what[0])
		return;
	if (p == H_CONNECT)
		sprintf(msg, "%.50s: connecting to %.60s port %d", what,
			http_host, http_port);
	else if (p == H_WAIT)
		sprintf(msg, "%.50s: waiting for %.60s", what, http_host);
	else {
		progress();
		return;
	}
	say(msg);
}

/* WRP's url field gets a clear button at its right end, while it has
 * text: a white x in a black circle. A click on it empties the field. */
#define XW 11
static char *xicon[] = {
	"....###....",
	"..#######..",
	".#.#####.#.",
	".##.###.##.",
	"####.#.####",
	"#####.#####",
	"####.#.####",
	".##.###.##.",
	".#.#####.#.",
	"..#######..",
	"....###...."
};

/* picture p, w x h (16 x 16 at most), at x, y; upside down if flip */
static void pict(x, y, p, w, h, flip)
int x, y, w, h, flip;
char **p;
{
	static unsigned char b[16 * 2];
	int bpl = (w + 7) / 8, r, c;
	char *s;

	for (r = 0; r < h * bpl; ++r)
		b[r] = 0;
	for (r = 0; r < h; ++r) {
		s = p[flip ? h - 1 - r : r];
		for (c = 0; c < w && s[c]; ++c)
			if (s[c] == '#')
				b[r * bpl + c / 8] |= 0x80 >> (c & 7);
	}
	g_bits(x, y, w, h, b, bpl);
}

/* Is it WRP's url field? */
static int isurl(it)
Item *it;
{
	return it->type == I_FIELD && it->name && strieq(it->name, "url") &&
		it->form >= 0 && it->form < MAXFORM && wrpf[it->form];
}

static void drawfield(i)
int i;
{
	Item *it = &items[i];
	int x = it->x, y = it->y - top, room = it->w - 2 * PAD - 4, n, k;
	char *s = it->val;

	fill(x + 1, y + 1, it->w - 2, it->h - 2, 0);
	box(x, y, it->w, it->h);
	n = len(s);
	if (isurl(it)) {
		room -= XW + PAD;
		k = y + (it->h - XW) / 2;
		if (n > 0 && k >= 0 && k + XW <= viewh &&
			x + it->w - 1 - PAD <= vieww)
			pict(x + it->w - 1 - PAD - XW, k, xicon, XW, XW, 0);
	}
	if (it->pass) {
		for (k = 0; k < n; ++k)
			stars[k] = '*';
		s = stars;
	}
	for (k = n; k > 0 && tw(s + n - k, k) > room; --k)
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

/* a picture p at x, y: only when all of it is on the page part. It is IH
 * rows high, and as wide as its rows. */
static void drawicon(x, y, p)
int x, y;
char **p;
{
	int w;

	w = len(p[0]);
	if (y >= 0 && y + IH <= viewh && x + w <= vieww)
		pict(x, y, p, w, IH, 0);
}

/* WRP's one-letter labels get pictures too: Z a magnifying glass for the
 * zoom, C a palette for the colors, D a dither pattern, K a keyboard for
 * the keys to send. Z and D are narrower than the others: with the 3B1's
 * 9-pixel font, WRP's form then still fits on one line (680 of 682). */
static char *kicon[] = {
	".............",
	".###########.",
	"#############",
	"##.#.#.#.#.##",
	"#############",
	"###.#.#.#.###",
	"#############",
	"##.#.....#.##",
	"#############",
	".###########.",
	"............."
};

static char *cicon[] = {
	"...#######...",
	".###########.",
	"##..##..#####",
	"##..##..#####",
	"#############",
	"#..##########",
	"#..######....",
	"########.....",
	"########.....",
	".#########...",
	"...#######..."
};

static char *zicon[] = {
	"..####.....",
	".##..##....",
	"##....##...",
	"#......#...",
	"#......#...",
	"##....##...",
	".##..###...",
	"..#######..",
	".......###.",
	"........###",
	".........##"
};

static char *dicon[] = {
	".........",
	"#.#.#.#.#",
	".#.#.#.#.",
	"#.#.#.#.#",
	".#.#.#.#.",
	"#.#.#.#.#",
	".#.#.#.#.",
	"#.#.#.#.#",
	".#.#.#.#.",
	"#.#.#.#.#",
	"........."
};

/* the picture of word it, or 0: only the label of WRP's K field and of
 * its C, Z and D menus has one. A label is the word just before them,
 * their NAME. */
static char **lpict(it)
Item *it;
{
	Item *f = it + 1;

	if (it->type != I_TEXT || it->href || it->form < 0 ||
		it->form >= MAXFORM || !wrpf[it->form] ||
		it - items + 1 >= nitems || f->form != it->form || !f->name ||
		!strieq(it->text, f->name))
		return (char **)0;
	if (f->type == I_FIELD && strieq(f->name, "k"))
		return kicon;
	if (f->type != I_SELECT)
		return (char **)0;
	if (strieq(f->name, "c"))
		return cicon;
	if (strieq(f->name, "z"))
		return zicon;
	if (strieq(f->name, "d"))
		return dicon;
	return (char **)0;
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

/* rows r up to r + h of picture im, at x, y */
static void drawpic(x, y, w, h, im, r)
int x, y, w, h;
Image *im;
long r;
{
	unsigned char *p = im->bits + r * im->bpl;

	if (im->depth != 8)
		g_bits(x, y, w, h, p, im->bpl);
	else if (im->pal)
		g_pix(x, y, w, h, p, im->bpl, im->pal);
}

/* screen rows r0 up to r1 of the tall picture's frame: the slices that
 * are here (rows still on their way are white) */
static void drawtall(r0, r1)
int r0, r1;
{
	Item *it = &items[tall];
	Slice *s;
	int y = it->y - top, a, b, i, sa, sb, w;
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
		w = it->x + s->im.w > vieww ? vieww - it->x : s->im.w;
		if (w > 0)
			drawpic(it->x, sa, w, sb - sa, &s->im, sa - p);
	}
}

/* screen rows r0 up to r1 of image item i */
static void drawimage(i, r0, r1)
int i, r0, r1;
{
	Item *it = &items[i];
	Image *im = it->img >= 0 ? &imgs[it->img] : 0;
	int y = it->y - top, a, b, w;

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
	w = it->x + im->w > vieww ? vieww - it->x : im->w;
	if (b > a && w > 0)
		drawpic(it->x, a, w, b - a, im, (long)(a - y));
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

/* The page's place, for the scroll bar: top and the frame's oy as one
 * number, 0 up to maxpos(). Down, the picture moves first in its frame,
 * then the page (if the frame does not fit); up, first the page. */
static long curpos()
{
	return top + (tall >= 0 ? oy : 0L);
}

static long maxpos()
{
	return maxtop() + (tall >= 0 ? (long)(items[tall].ih - items[tall].h) : 0L);
}

/* how much of it shows: the frame, or the page part */
static long vispos()
{
	return tall >= 0 ? (long)items[tall].h : (long)viewh;
}

/* Archaic's scroll bar: the shaft is rows *a up to *b (with the lines
 * of the arrow boxes), the elevator rows *ey up to *ey + *eh. 0 if the
 * page part is too low for an elevator. */
static int barpos(a, b, ey, eh)
int *a, *b, *ey, *eh;
{
	long m = maxpos(), v = vispos(), t;

	*a = SBH;
	*b = viewh - SBH;
	t = *b - *a;
	*ey = *a;
	*eh = (int)t;
	if (t < SBH + 2)
		return 0;
	if (m > 0) {
		*eh = (int)(t * v / (v + m));
		if (*eh < SBH)
			*eh = SBH;
		*ey = *a + (int)((t - *eh) * curpos() / m);
	}
	return 1;
}

static void drawbar()
{
	static unsigned char pat[68 * 2];	/* the shaft: 1 dot in 8, as GEM's */
	static int made;
	int x = vieww + 1, w = SBW - 1, a, b, ey, eh, y, n;

	if (!made) {
		for (y = 0; y < 68; ++y)
			pat[2 * y] = pat[2 * y + 1] = (y & 1) ? ((y & 2) ? 0x11 : 0x44) : 0;
		made = 1;
	}
	g_fill(vieww, 0, 1, viewh, 1);		/* the line left of it */
	g_fill(x, 0, w, viewh, 0);
	if (viewh < 2 * SBH + 2)
		return;
	pict(x + (w - AW) / 2, (SBH - AH) / 2, arrow, AW, AH, 0);
	g_fill(x, SBH, w, 1, 1);
	pict(x + (w - AW) / 2, viewh - SBH + (SBH - AH + 1) / 2, arrow, AW, AH, 1);
	g_fill(x, viewh - SBH - 1, w, 1, 1);
	if (bpart == B_UP)		/* held down */
		g_invert(x, 0, w, SBH);
	if (bpart == B_DN)
		g_invert(x, viewh - SBH, w, SBH);
	if (!barpos(&a, &b, &ey, &eh))
		return;
	for (y = a + 1; y < b - 1; y += n) {
		n = b - 1 - y < 64 ? b - 1 - y : 64;
		g_bits(x, y, w, n, pat + 2 * (y & 3), 2);
	}
	g_fill(x, ey + 1, w, eh - 2, 0);	/* the elevator */
	g_fill(x, ey, w, 1, 1);
	g_fill(x, ey + eh - 1, w, 1, 1);
}

/* the scroll bar shows the page's place: Archaic's own, or the system's */
static void showbar()
{
	long p, m, v;

	if (headless)
		return;
	if (bar) {
		drawbar();
		return;
	}
	p = curpos();
	m = maxpos();
	v = vispos();
	if (p != lpos || m != lmax || v != lvis) {
		lpos = p;
		lmax = m;
		lvis = v;
		g_sbar(p, m, v);
	}
}

/* draw the part x, y, w, h of the window again */
static void redraw(rx, ry, rw, rh)
int rx, ry, rw, rh;
{
	Item *it;
	int i, y, r1 = ry + rh, r = rx + rw;
	char **p;

	if (headless)
		return;
	if (r > vieww)
		r = vieww;
	if (ry < viewh && rx < r) {
		fill(rx, ry, r - rx, rh, 0);
		for (i = 0; i < nitems; ++i) {
			it = &items[i];
			y = it->y - top;
			if (it->w <= 0 || it->h <= 0 || y >= r1 || y + it->h <= ry ||
				it->x >= r || it->x + it->w <= rx)
				continue;
			switch (it->type) {
			case I_TEXT:
				if ((p = lpict(it)) != 0) {
					drawicon(it->x, y, p);
					break;
				}
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
			r > px)
			drawpop();
	}
	if (ry < viewh && (!bar || rx + rw > vieww))
		showbar();
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
	if (w > vieww)
		w = vieww;
	k = it->nopt;
	if (k * prow + 2 > viewh)
		k = (viewh - 2) / prow;
	pw = w;
	ph = k * prow + 2;
	px = it->x;
	if (px + pw > vieww)
		px = vieww - pw;
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
	char **p;

	it->w = it->h = 0;
	switch (it->type) {
	case I_TEXT:
		if ((p = lpict(it)) != 0) {
			it->w = len(p[0]);
			it->h = IH;
			break;
		}
		it->w = tw(it->text, len(it->text));
		it->h = fh;
		break;
	case I_FIELD:
		it->w = it->size * cw + 2 * PAD + 4;
		if (isurl(it))		/* and its clear button */
			it->w += XW + PAD;
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
		if (x + sp + it->w > vieww - M && x > M) {
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
 * m, t and c unless -a, and c not if its menu is shown. */
static int ours(it)
Item *it;
{
	static char *set[] = { "w", "h", "m", "t", "c" };
	int i;

	if (it->form < 0 || it->form >= MAXFORM || !wrpf[it->form] || !it->name)
		return 0;
	for (i = 0; i < (allset ? 2 : 5); ++i)
		if (strieq(it->name, set[i]))
			return i < 4 || !cmenu;
	return 0;
}

/* Is it WRP's C menu, shown? */
static int iscmenu(it)
Item *it;
{
	return it->type == I_SELECT && it->name && strieq(it->name, "c") &&
		it->form >= 0 && it->form < MAXFORM && wrpf[it->form];
}

/* Is it WRP's D menu: what WRP dithers in a black and white picture? */
static int isdmenu(it)
Item *it;
{
	return it->type == I_SELECT && it->name && strieq(it->name, "d") &&
		it->form >= 0 && it->form < MAXFORM && wrpf[it->form];
}

/* WRP's C menu: the choice nearest ncol, and not above it if it can */
static void setcol(it)
Item *it;
{
	Opt *o;
	int j, v, best = -1, bv = 0;

	for (j = 0; j < it->nopt; ++j) {
		o = &opts[it->opt + j];
		v = atoin(o->val);
		if (o->dis || v < 2)
			continue;
		if (best < 0 || (v <= ncol ? bv > ncol || v > bv :
			bv > ncol && v < bv)) {
			best = j;
			bv = v;
		}
	}
	if (best >= 0)
		it->sel = best;
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
	for (i = 0; cmenu && !allset && i < nitems; ++i)
		if (iscmenu(&items[i]))
			setcol(&items[i]);
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
	*w = vieww - 2 * M;
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
	pclick = pwide = 0;
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

/* the key that WRP's button b sends to the page (Bs, Rt), else 0 */
static int fnkey(f, b)
int f, b;
{
	Item *it;

	if (b < 0 || f < 0 || f >= MAXFORM || !wrpf[f])
		return 0;
	it = &items[b];
	if (it->type != I_BUTTON || !it->name || !strieq(it->name, "Fn") ||
		!it->text)
		return 0;
	return strieq(it->text, "Bs") ? '\b' : strieq(it->text, "Rt") ? '\r' : 0;
}

/* WRP's K field in form f, if it has text */
static int iskeys(it, f)
Item *it;
int f;
{
	return it->type == I_FIELD && it->form == f && it->name &&
		strieq(it->name, "k") && it->val && it->val[0];
}

/* send form f, with button b (-1: Return in a field) */
static void submit(f, b)
int f, b;
{
	static char kv[MAXVAL + 2];
	Item *it;
	int i, n, w, h, kb;

	if (f < 0 || f >= nforms)
		return;
	/* WRP's Bs or Rt with text in K: WRP would send only the button's
	 * key, so send the text and then that key, as keys */
	kb = fnkey(f, b);
	for (i = 0; kb && i < nitems && !iskeys(&items[i], f); ++i)
		;
	if (i == nitems)
		kb = 0;
	q[0] = 0;
	for (i = 0; i < nitems; ++i) {
		it = &items[i];
		if (it->form != f || !it->name || !it->name[0] || ours(it))
			continue;
		if (kb && iskeys(it, f)) {
			scopy(kv, it->val, MAXVAL);
			n = len(kv);
			kv[n] = kb;
			kv[n + 1] = 0;
			addq(it->name, kv);
		} else if (it->type == I_FIELD || it->type == I_HIDDEN)
			addq(it->name, it->val);
		else if (it->type == I_SELECT && it->sel >= 0)
			addq(it->name, opts[it->opt + it->sel].val);
		else if (it->type == I_BUTTON && i == b && !kb)
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
			for (i = 0; i < nitems; ++i)
				if (items[i].form == f && iscmenu(&items[i]))
					break;
			if (i == nitems) {	/* no C menu: Archaic's colors */
				sprintf(num, "%d", ncol);
				addq("c", num);
			}
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

/* scroll to place v of the page (see curpos) */
static void scrollto(v)
long v;
{
	Item *it;
	long r, ooy = oy, m = maxpos();
	int otop = top, opop = popup, a, b;

	if (v > m)
		v = m;
	if (v < 0)
		v = 0;
	if (v != curpos()) {
		sdir = v > curpos() ? 1 : -1;
		sstop = 0;
	}
	if (tall >= 0) {
		r = items[tall].ih - items[tall].h;
		oy = v < r ? v : r;
		v -= oy;
	}
	top = (int)v;
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

/* scroll d pixels, down if d > 0 */
static void scroll(d)
int d;
{
	if (d == 0)
		return;
	sdir = d > 0 ? 1 : -1;
	sstop = 0;
	scrollto(curpos() + d);
}

/* a window's worth of scrolling: the frame (or the window) less 2 lines */
static int pagestep()
{
	int n = (tall >= 0 ? items[tall].h : viewh) - 2 * fh;

	return n > fh ? n : fh;
}

/* a quarter of the frame (or the page part): a step of the wheel, an arrow */
static int stepsize()
{
	return (tall >= 0 ? items[tall].h : viewh) / 4;
}

/* the part of Archaic's scroll bar at row y */
static int barpart(y)
int y;
{
	int a, b, ey, eh;

	if (y < SBH)
		return B_UP;
	if (y >= viewh - SBH)
		return B_DN;
	if (!barpos(&a, &b, &ey, &eh))
		return 0;
	if (y < ey)
		return B_PGUP;
	if (y >= ey + eh)
		return B_PGDN;
	return B_DRAG;
}

/* what part p of the bar does, once */
static void baract(p)
int p;
{
	if (p == B_UP)
		scroll(-stepsize());
	else if (p == B_DN)
		scroll(stepsize());
	else if (p == B_PGUP)
		scroll(-pagestep());
	else if (p == B_PGDN)
		scroll(pagestep());
}

/* the elevator taken to row y: the page to the place it shows */
static void drag(y)
int y;
{
	int a, b, ey, eh;

	if (!barpos(&a, &b, &ey, &eh) || b - a - eh <= 0)
		return;
	if (y == by0)
		scrollto(bpos0);
	else
		scrollto((long)(y - bgrab - a) * maxpos() / (b - a - eh));
}

/* a button pressed on the bar at x, y */
static void barpress(x, y)
int x, y;
{
	int a, b, ey, eh;

	bpart = barpart(y);
	bticks = 0;
	bpx = x;
	bpy = by0 = y;
	if (bpart == B_DRAG) {
		barpos(&a, &b, &ey, &eh);
		bgrab = y - ey;
		bpos0 = curpos();
	} else
		baract(bpart);
	drawbar();
	g_flush();
}

/* the button let go at row y. A display with no E_MOVE (X10) moves the
 * elevator only here, as GEM did. */
static void barup(y)
int y;
{
	if (bpart == B_DRAG)
		drag(y);
	bpart = 0;
	drawbar();
	g_flush();
}

/* a part of the bar held down 1/10 s more: after 4/10 s, the arrows and
 * the shaft do it again, while the pointer stays on them */
static void bartick()
{
	if (bpart == B_DRAG || ++bticks < 4)
		return;
	if (bpx >= vieww && bpx < winw && barpart(bpy) == bpart)
		baract(bpart);
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
		if ((k == '\r' || k == '\n') && nitems == 0)
			go(lurl, (char *)0);	/* no page yet: try again */
		else if (k == ' ' || k == 'F' - 64)
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

/* WRP's form on this page, or -1 */
static int wrpform()
{
	int f;

	for (f = 0; f < nforms && f < MAXFORM; ++f)
		if (wrpf[f])
			return f;
	return -1;
}

/* WRP's St (stop) button in form f, or -1. WRP takes a new picture of
 * the page as it is for St, and does not load the page again. */
static int stbutton(f)
int f;
{
	Item *b;
	int i;

	for (i = 0; i < nitems; ++i) {
		b = &items[i];
		if (b->type == I_BUTTON && b->form == f && b->name &&
			strieq(b->name, "Fn") && b->text && strieq(b->text, "St"))
			return i;
	}
	return -1;
}

/* a new choice in WRP's C menu: send the form now, with St, so that WRP
 * takes the picture again in those colors */
static void newcol(it)
Item *it;
{
	ncol = atoin(opts[it->opt + it->sel].val);
	if (ncol < 2 || ncol > 256)
		ncol = ncol < 2 ? 2 : 256;
	submit(it->form, stbutton(it->form));
}

static void click(x, y, b)
int x, y, b;
{
	Item *it;
	int i, j, old;

	if (popup >= 0) {
		i = popup;
		it = &items[i];
		old = it->sel;
		if (x >= px && x < px + pw && y > py && y < py + ph - 1) {
			j = (y - py - 1) / prow;
			if (j >= 0 && j < it->nopt && !opts[it->opt + j].dis)
				it->sel = j;
		}
		closepop();
		drawone(i);
		if (it->sel != old && cmenu && !allset && iscmenu(it))
			newcol(it);
		else if (it->sel != old && !allset && isdmenu(it))
			submit(it->form, stbutton(it->form));
		return;
	}
	if (y >= viewh)
		return;
	if (bar && x >= vieww) {
		barpress(x, y);
		return;
	}
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
		if (isurl(it) && it->val[0] && x >= it->x + it->w - 2 - PAD - XW)
			it->val[0] = 0;		/* its clear button */
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
			if (oldpic)
				break;	/* WRP's next page comes: this map goes */
			sprintf(u, "%.1500s?%d,%ld", it->href, x - it->x,
				y + top - it->y + (i == tall ? oy : 0L));
			go(u, (char *)0);
			pclick = 1;
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
	switch (e->type) {
	case E_KEY:
		key(e->key);
		break;
	case E_DOWN:
		if (bpart)		/* its E_UP did not come */
			barup(bpy);
		click(e->x, e->y, e->key);
		break;
	case E_UP:
		if (bpart)
			barup(e->y);
		break;
	case E_MOVE:
		bpx = e->x;
		bpy = e->y;
		if (bpart == B_DRAG)
			drag(e->y);
		break;
	case E_EXPOSE:
		redraw(e->x, e->y, e->w, e->h);
		break;
	case E_SIZE:
		if (e->w == winw && e->h == winh)
			break;
		if (e->w != winw) {	/* WRP's picture again: see rewide */
			resz = 1;
			rest = g_ms();
		}
		winw = e->w;
		winh = e->h;
		vieww = bar ? winw - SBW : winw;
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
		if (e->key == S_PAGE)
			scroll(e->y * pagestep());
		else if (e->key == S_STEP)
			scroll(e->y * stepsize());
		else if (e->key == S_TO)
			scrollto((long)e->y);
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
	long t;

	spin();
	if (phase == H_WAIT && what[0]) {	/* WRP makes the page */
		t = now() - tphase;
		if (t != tshown) {
			tshown = t;
			sprintf(msg, "%.50s: waiting for %.60s, %ld s", what,
				http_host, t);
			say(msg);
		}
	}
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
	progress();
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
	g_flush();
}

static void imfree(im)
Image *im;
{
	if (im->bits)
		free((char *)im->bits);
	if (im->pal)
		free((char *)im->pal);
	im->bits = im->pal = 0;
	im->w = im->h = 0;
	im->kind = 0;
}

static void freeimgs()
{
	int i;

	for (i = 0; i < nimgs; ++i)
		imfree(&imgs[i]);
	nimgs = 0;
}

static void freeslices()
{
	Slice *s;

	for (s = slices; s < slices + NSLICE; ++s) {
		imfree(&s->im);
		s->state = 0;
	}
}

/* picture k of the n on the page: item i */
static void loadimg(i, k, n)
int i, k, n;
{
	Item *it = &items[i];
	Image *im = &imgs[it->img];

	url_join(base, it->text, u);
	if (n > 1)
		sprintf(what, "Picture %d of %d", k, n);
	else
		scopy(what, "Picture", (int)sizeof what);
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
	im->depth = idepth;
	if (pic_load(im) == 0)
		progress();
	else if (!stop) {
		sprintf(msg, "Picture: can't show %s", http_type[0] ? http_type : "it");
		say(msg);
	}
	lbytes += http_got;
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
	int n, c, ok = 0, was = spinon;
	long k, m;

	if (!quiet)
		spinto(1);
	imfree(&s->im);
	s->im.depth = idepth;
	s->y0 = a;
	s->state = 1;
	k = sk(a) - sk(0L) + 1;
	m = sk((long)items[tall].ih - 1) - sk(0L) + 1;
	sprintf(what, quiet ? "Picture part %ld of %ld (ahead)" :
		"Picture part %ld of %ld", k, m);
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
			} else if (pic_load(&s->im) < 0) {
				if (!stop)
					say("Picture: can't show the slice");
			} else if (s->im.h != b - a)	/* the server ignored y and h */
				say("Picture: WRP did not send the rows asked for");
			else {
				ok = 1;
				lbytes += http_got;
				progress();	/* all of it */
			}
			http_close();
		}
	}
	loading = 0;
	spinto(was);
	if (ok && !stop) {
		s->state = 2;
		say(ssave);
		return;
	}
	imfree(&s->im);
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
		if (isurl(it) && it->val)
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
	f = wrpform();
	if (f < 0)
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

/* v * a / b, for page rows: no long overflows */
static long lscale(v, a, b)
long v, a, b;
{
	while (a > 30000L || b > 30000L) {
		a >>= 1;
		b >>= 1;
	}
	return b > 0 ? v / b * a + v % b * a / b : v;
}

/* the width of WRP's picture on this page, or 0 */
static int picw()
{
	Item *it;
	int i;

	for (i = 0; i < nitems; ++i) {
		it = &items[i];
		if (it->type == I_IMAGE && it->ismap && it->href)
			return it->iw > 0 ? it->iw :
				it->img >= 0 ? imgs[it->img].w : 0;
	}
	return 0;
}

/* The window's width changed: once it has stayed the same for 0.4 s
 * (with no clock: at once), and WRP's picture is not as wide as the
 * window, send WRP's form with St and the new width. 1 if it did. */
static int rewide()
{
	long t = g_ms();
	int f, w, h, pw;

	if (t >= 0 && t >= rest && t - rest < 400)
		return 0;
	resz = 0;
	f = wrpform();
	pw = picw();
	if (f < 0 || pw <= 0)
		return 0;
	picsize(&w, &h);
	if (w >= pw - 1 && w <= pw + 1)
		return 0;	/* as wide (WRP's zoom may round it) */
	submit(f, stbutton(f));
	pwide = 1;
	return 1;
}

/* Has the page (n bytes at p) a form? */
static int hasform(p, n)
char *p;
int n;
{
	int i;

	for (i = 0; i + 5 <= n; ++i)
		if (p[i] == '<' && strnieq(p + i, "<form", 5))
			return 1;
	return 0;
}

/* the first line of the page that has text, less its tags: the status
 * line for an error from WRP */
static void errline(p, n)
char *p;
int n;
{
	int i, c, k, tag = 0;

	scopy(msg, "WRP: ", (int)sizeof msg);
	k = len(msg);
	for (i = 0; i < n && k < 130; ++i) {
		c = p[i] & 0377;
		if (c == '<')
			tag = 1;
		else if (tag) {
			if (c == '>')
				tag = 0;
		} else if (c == '\n' || c == '\r') {
			if (k > 5)
				break;
		} else if (c == ' ' || c == '\t') {
			if (k > 5 && msg[k - 1] != ' ')
				msg[k++] = ' ';
		} else if (c > ' ' && c < 0177)
			msg[k++] = c;
	}
	while (k > 5 && msg[k - 1] == ' ')
		--k;
	msg[k] = 0;
	if (k == 5)
		scopy(msg + 5, "an answer with no page", (int)sizeof msg - 5);
}

static void load(url, post)
char *url, *post;
{
	Item *it;
	static char fname[40];
	static char fval[MAXVAL];
	int i, n, c, k, nk, fromwrp = wrpform() >= 0, byclick = pclick;
	int byresize = pwide;
	char *done = note;	/* the last status line, if not the usual one */
	long oldh;

	pclick = pwide = 0;
	note = 0;
	scopy(lurl, url, MAXURL);
	if (post)
		scopy(lbody, post, MAXURL);
	loading = 1;
	oldpic = fromwrp;
	spinto(1);
	stop = 0;
	t0 = now();
	lbytes = 0;
	scopy(what, "Page", (int)sizeof what);
	if (http_open(lurl, post ? lbody : (char *)0) < 0) {
		if (nitems == 0) {	/* no page: nothing to click */
			sprintf(msg, "%.150s. Return: try again", http_err);
			say(msg);
		} else
			say(http_err);
		goto out;
	}
	if (strnieq(http_type, "image/", 6) && fromwrp) {
		sprintf(msg, "Not shown: a picture (%.60s)", http_type);
		say(msg);		/* WRP's page stays */
		http_close();
		goto out;
	} else if (strnieq(http_type, "image/", 6)) {
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
			if ((http_got & 255) == 0)
				progress();
		}
		progress();
		lbytes += http_got;
		http_close();
		if (stop) {
			if (!pending)
				say(http_err[0] ? http_err : "Stopped");
			goto out;
		}
		page[n] = 0;
		/* WRP's error: its page stays. After a click on an old
		 * picture, WRP's new picture comes (St). */
		if (fromwrp && !hasform(page, n)) {
			errline(page, n);
			say(msg);
			if (byclick && wrpform() >= 0) {
				submit(wrpform(), stbutton(wrpform()));
				note = "The picture was old: here is WRP's new one. Click again.";
			}
			goto out;
		}
	}
	/* the new page */
	fname[0] = fval[0] = 0;
	if (focus >= 0 && items[focus].name) {
		scopy(fname, items[focus].name, (int)sizeof fname);
		if (items[focus].val)
			scopy(fval, items[focus].val, MAXVAL);
	}
	oldh = tall >= 0 ? (long)items[tall].ih : 0L;
	scopy(spage, wrpurl(), MAXURL);
	scopy(base, http_url, MAXURL);
	focus = popup = pressed = -1;
	top = 0;
	tall = -1;
	freeimgs();
	freeslices();
	if (html_parse(page, n) < 0)
		say("The page is too big: only its start is shown");
	oldpic = 0;
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
	else if (byresize && oldh > 0)	/* in proportion: it is wider */
		oy = lscale(oy, (long)items[tall].ih, oldh);
	ssize = 0;
	sstop = 0;
	tallfix();
	for (i = 0; fname[0] && i < nitems; ++i) {
		it = &items[i];
		if (it->type == I_FIELD && it->name && strieq(it->name, fname)) {
			focus = i;
			if (byresize && it->val)	/* keep what was typed */
				scopy(it->val, fval, MAXVAL);
			break;
		}
	}
	redraw(0, 0, winw, winh);
	for (i = nk = 0; i < nitems; ++i)
		if (items[i].type == I_IMAGE && items[i].img >= 0 && i != tall)
			++nk;
	for (i = k = 0; i < nitems && !stop; ++i)
		if (items[i].type == I_IMAGE && items[i].img >= 0 && i != tall)
			loadimg(i, ++k, nk);
	while (!stop && slicework(0))	/* the frame of the tall picture */
		;
	if (stop) {
		if (!pending)
			say("Stopped");
		goto out;
	}
	if (done)
		say(done);
	else {
		sprintf(msg, "%ld s, %s  %.150s", now() - t0, bytes(lbytes), base);
		say(msg);
	}
out:
	loading = 0;
	oldpic = 0;
	spinto(0);
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
	for (i = 0; im->depth == 8 && im->pal && i < 768; ++i)
		s = (s * 31 + im->pal[i]) & 0xffffffffL;
	return s;
}

/* -t: what the page is made of */
static void dump()
{
	static char *tn[] = { "?", "text", "br", "para", "field", "button",
		"hidden", "select", "image" };
	static char *kn[] = { "none", "gif", "abm", "?" };
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
		if (icon(it) || lpict(it))
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
					printf(" got %dx%d %s sum %08lx%s", s->im.w, s->im.h,
						kn[s->im.kind & 3], sum(&s->im),
						s->im.depth == 8 ? " colors" : "");
			}
			printf(" tall %dx%d at %ld", it->iw, it->ih, oy);
		} else if (it->type == I_IMAGE && it->img >= 0) {
			im = &imgs[it->img];
			printf(" got %dx%d %s sum %08lx%s", im->w, im->h,
				kn[im->kind & 3], sum(im),
				im->depth == 8 ? " colors" : "");
		}
		printf("\n");
	}
	fflush(stdout);
}

static void writepbm()
{
	FILE *f;
	Image *im = 0;
	long n;
	int i, c;

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
	if (im->depth == 8 && im->pal) {
		fprintf(f, "P6\n%d %d\n255\n", im->w, im->h);
		for (n = 0; n < (long)im->bpl * im->h; ++n) {
			c = im->bits[n] * 3;
			putc(im->pal[c], f);
			putc(im->pal[c + 1], f);
			putc(im->pal[c + 2], f);
		}
	} else {
		fprintf(f, "P4\n%d %d\n", im->w, im->h);
		fwrite((char *)im->bits, 1, (int)((long)im->bpl * im->h), f);
	}
	fclose(f);
}

/* the display's colors, in words */
static char *cname(n)
long n;
{
	if (n <= 2)
		return "black and white";
	if (n >= 1L << 24)
		return "millions of colors";
	if (n > 256)
		return "thousands of colors";
	sprintf(nb, "%ld colors", n);
	return nb;
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
	if (e.type == E_DOWN) {
		e.type = E_UP;
		event(&e);
	}
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
			(a[1] == 'o' || a[1] == 's' || a[1] == 'g' || a[1] == 'c' ||
			a[1] == 'f')) {
			if (a[1] == 'f') {
				a = argv[++i];
				if (strieq(a, "gif"))
					http_accept = "text/html, image/gif";
				else if (!strieq(a, "abm")) {
					fprintf(stderr, "archaic: -f takes gif or abm\n");
					return 2;
				}
			} else if (a[1] == 'o')
				pbm = argv[++i];
			else if (a[1] == 's')
				script = argv[++i];
			else if (a[1] == 'c') {
				copt = atoin(argv[++i]);
				if (copt < 2 || copt > 256) {
					fprintf(stderr, "archaic: -c takes 2 to 256 colors\n");
					return 2;
				}
			} else
				geom = argv[++i];
		} else if (a[0] == '-') {
			fprintf(stderr, "usage: archaic [=WxH+X+Y] [-a] [-c colors] [-f gif|abm] [-t] [-v] [-o file] [-s script] [url]\n");
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
	bar = !headless && g_bar;
	vieww = bar ? winw - SBW : winw;
	viewh = winh - fh - 3;
	/* the colors: what the display shows (-t: none, or -c's) */
	dcolors = headless ? (copt ? (long)copt : 2L) : g_colors();
	if (dcolors < 2)
		dcolors = 2;
	ncol = copt ? copt : dcolors > 256 ? 256 : (int)dcolors;
	cmenu = dcolors > 2;
	idepth = cmenu ? 8 : 1;
	sprintf(msg, "Archaic %s, %s", VERSION, cname(dcolors));
	say(msg);
	/* WRP's start page as the next ones: its C menu (WRP shows it
	 * for GIF), and no Up and Dn (slices) */
	for (i = 7; u[i] && u[i] != '/'; ++i)
		;
	if (!allset && u[i] == '/' && !u[i + 1] && i < MAXURL - 50)
		sprintf(u + i + 1, "?m=ismap&t=gif&c=%d&sl=1", ncol);
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
			/* the slices a scroll needs come before the next step */
			if (!headless && slicework(1))
				continue;
			runscript();
			continue;
		}
		if (headless)
			break;
		if (resz) {		/* a new width: WRP's picture again */
			if (!rewide() && resz && g_event(&e, 2))
				event(&e);
			continue;
		}
		if (slicework(1))	/* the frame's slices, then the next ones */
			continue;
		/* a part of the scroll bar held down: wait 1/10 s at most */
		if (g_event(&e, bpart && bpart != B_DRAG ? 2 : 1))
			event(&e);
		else if (bpart)
			bartick();
	}
	if (pbm)
		writepbm();
	if (!headless)
		g_close();
	return 0;
}
