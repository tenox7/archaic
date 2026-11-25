/* mac.c - Archaic's display for the classic Mac OS on a 68k Mac (System
 * 6 and 7): a window of QuickDraw with the Mac's own scroll bar, and the
 * Apple, File and Edit menus. A Mac with Color QuickDraw (a Mac II)
 * shows the pictures in colors, as many as its screen has (Monitors).
 * Built with Retro68: see mkmac.
 *
 * The Mac has no command line. main() here starts the Toolbox, reads
 * the options from the file "Archaic Options" in Archaic's folder (one
 * line, as on Unix: "-c 16 10.0.0.1:8080"), and calls main.c's main,
 * which mkmac's build names archaic_main.
 *
 * The scroll bar is the Mac's: g_bar is 0, and g_sbar sets it. Its
 * arrows and its gray parts step the page while the button is down (a
 * step each 1/10 s, after 4/10 s), as E_SCROLL events; its box is
 * dragged with TrackControl, then the page goes there (S_TO). Its
 * values are 16 bits: a long page is counted in steps of k pixels.
 * Archaic's part of the window is all of it but the scroll bar's column,
 * where the grow box is too.
 *
 * Text is Geneva 9. Archaic's text is Latin-1, the Mac's is Mac Roman:
 * the letters are changed both ways (lat[]). Cmd-. is Esc, Cmd-click
 * the right button (it scrolls), Paste types the clipboard's text.
 * Control and a letter is its control code: Ctrl-U, Ctrl-Q.
 *
 * Multitasking (System 7, and MultiFinder): the Mac's programs share the
 * CPU only when each one asks for its next event. So Archaic waits in
 * WaitNextEvent with a sleep, never in a loop of its own: g_event, and
 * mac_wait for the network (nmactcp.c, dnr.c). While a picture decodes
 * (seconds, in colors on a 68030), YIELD in img_rows lets the others
 * run each 1/15 s. Events that come meanwhile go to Archaic's queue.
 * Suspend and resume (archaic.r: acceptSuspendResumeEvents) are the
 * window's deactivate and activate. Apple events: Quit (the Finder's
 * Shut Down and Restart send it) quits; Open Application is nothing.
 * In the background a page goes on loading. On System 7 the window
 * leaves the desktop's column of disk icons at the right. */
#include "archaic.h"
#include <string.h>
#include <Multiverse.h>

#define MSB 15			/* the scroll bar's column, less its frame */
#define ZOOMDOC 8		/* zoomDocProc: a window with a zoom box */
#define GENEVA 3		/* the font Geneva's number */
#define MAPPLE 128		/* the menus */
#define MFILE 129
#define MEDIT 130
#define NQ 256			/* events from one Mac event (a Paste) */
#define BIG 2000000000L		/* S_TO past the end */

int g_bar = 0;			/* the Mac has scroll bars */
static WindowPtr win;
static ControlHandle sb;	/* the scroll bar */
static MenuHandle mapple, mfile, medit;
static int cqd;			/* the Mac has Color QuickDraw */
static int wne;			/* WaitNextEvent is there (MultiFinder) */
static int sys7;		/* System 7 or newer */
static int aeok;		/* Apple events are there (System 7) */
static int fg = 1;		/* Archaic is the front program */
static long tyield;		/* ticks: the last WaitNextEvent */
static int ww, wh;		/* the window's inside */
static int fasc, fhgt;		/* the font */
static long smax, sk = 1;	/* g_sbar's max; the scroll bar's step */
static int trk;			/* the part of the scroll bar held down */
static long tnext;		/* ticks: its next step */
static RgnHandle rgn, rgn2;
static CTabHandle ctab;		/* g_pix's colors */
static unsigned char cpal[768];	/* what ctab has */
static int cset;		/* ctab is set */
static Event q[NQ];
static int qin, qout;

/* Mac Roman 128 to 255 as Latin-1; 0 if Latin-1 has no such letter */
static unsigned char lat[128] = {
	0xc4, 0xc5, 0xc7, 0xc9, 0xd1, 0xd6, 0xdc, 0xe1,
	0xe0, 0xe2, 0xe4, 0xe3, 0xe5, 0xe7, 0xe9, 0xe8,
	0xea, 0xeb, 0xed, 0xec, 0xee, 0xef, 0xf1, 0xf3,
	0xf2, 0xf4, 0xf6, 0xf5, 0xfa, 0xf9, 0xfb, 0xfc,
	0, 0xb0, 0xa2, 0xa3, 0xa7, 0, 0xb6, 0xdf,
	0xae, 0xa9, 0, 0xb4, 0xa8, 0, 0xc6, 0xd8,
	0, 0xb1, 0, 0, 0xa5, 0xb5, 0, 0,
	0, 0, 0, 0xaa, 0xba, 0, 0xe6, 0xf8,
	0xbf, 0xa1, 0xac, 0, 0, 0, 0, 0xab,
	0xbb, 0, 0xa0, 0xc0, 0xc3, 0xd5, 0, 0,
	0, 0, 0, 0, 0, 0, 0xf7, 0,
	0xff, 0, 0, 0xa4, 0, 0, 0, 0,
	0, 0xb7, 0, 0, 0, 0xc2, 0xca, 0xc1,
	0xcb, 0xc8, 0xcd, 0xce, 0xcf, 0xcc, 0xd3, 0xd4,
	0, 0xd2, 0xda, 0xdb, 0xd9, 0, 0, 0,
	0xaf, 0, 0, 0, 0xb8, 0, 0, 0
};
static unsigned char mac[128];	/* Latin-1 128 to 255 as Mac Roman */

int archaic_main P((int argc, char **argv));

/* a C string as a Pascal string, in one of 2 buffers */
static unsigned char *ps(s)
char *s;
{
	static unsigned char b[2][256];
	static int k;
	unsigned char *p = b[k ^= 1];
	int n = 0;

	while (s[n] && n < 255) {
		p[n + 1] = (unsigned char)s[n];
		++n;
	}
	p[0] = (unsigned char)n;
	return p;
}

/* Archaic's Latin-1 text as Mac Roman, in b (n letters at most 255) */
static int tomac(s, n, b)
char *s;
int n;
unsigned char *b;
{
	int i, c;

	if (n > 255)
		n = 255;
	for (i = 0; i < n; ++i) {
		c = s[i] & 0377;
		b[i] = (unsigned char)(c < 128 ? c : mac[c - 128] ? mac[c - 128] : '?');
	}
	return n;
}

static void put(t, x, y, k)
int t, x, y, k;
{
	Event *e;

	if ((qin + 1) % NQ == qout)
		return;
	e = &q[qin];
	e->type = t;
	e->x = x;
	e->y = y;
	e->w = 0;
	e->h = 0;
	e->key = k;
	qin = (qin + 1) % NQ;
}

/* drawing: Archaic's part of the window, or all of it (the scroll bar,
 * the grow box) */
static void aclip()
{
	Rect r;

	SetRect(&r, 0, 0, ww - MSB, wh);
	ClipRect(&r);
}

static void fclip()
{
	ClipRect(&win->portRect);
}

/* the grow box alone: DrawGrowIcon also draws the lines of 2 scroll bars */
static void growbox()
{
	Rect r;

	r = win->portRect;
	r.left = r.right - MSB;
	r.top = r.bottom - MSB;
	ClipRect(&r);
	DrawGrowIcon(win);
	aclip();
}

/* the scroll bar's place: the right of the window, above the grow box */
static void sbrect(r)
Rect *r;
{
	SetRect(r, ww - MSB, -1, ww + 1, wh - MSB + 1);
}

/* the window has a new size */
static void resized()
{
	Rect r;

	ww = win->portRect.right - win->portRect.left;
	wh = win->portRect.bottom - win->portRect.top;
	fclip();
	sbrect(&r);
	MoveControl(sb, r.left, r.top);
	SizeControl(sb, r.right - r.left, r.bottom - r.top);
	InvalRect(&win->portRect);
	aclip();
	put(E_SIZE, 0, 0, 0);
	q[(qin + NQ - 1) % NQ].w = ww - MSB;
	q[(qin + NQ - 1) % NQ].h = wh;
}

int g_open(geom, w, h)
char *geom;
int *w, *h;
{
	Rect r, sr, b;
	FontInfo fi;
	int i, n[4], k = 0;

	sr = qd.screenBits.bounds;
	SetRect(&r, 4, GetMBarHeight() + 22, sr.right - (sys7 ? 70 : 4),
		sr.bottom - 4);
	/* =WxH+X+Y, or -g WxH+X+Y: the inside's size, and its place */
	if (geom && *geom == '=')
		++geom;
	for (i = 0; i < 4; ++i)
		n[i] = -1;
	while (geom && *geom && k < 4) {
		if (*geom >= '0' && *geom <= '9') {
			n[k] = 0;
			while (*geom >= '0' && *geom <= '9')
				n[k] = n[k] * 10 + *geom++ - '0';
			++k;
		} else
			++geom;
	}
	if (n[0] > 100 && n[1] > 60) {
		r.right = r.left + n[0] + MSB;
		r.bottom = r.top + n[1];
		if (n[2] >= 0 && n[3] >= 0)
			OffsetRect(&r, n[2] - r.left, n[3] - r.top);
		b = sr;
		b.top = GetMBarHeight() + 20;
		if (r.right > b.right)
			OffsetRect(&r, b.right - r.right, 0);
		if (r.bottom > b.bottom)
			OffsetRect(&r, 0, b.bottom - r.bottom);
		if (r.left < b.left)
			r.left = b.left;
		if (r.top < b.top)
			r.top = b.top;
	}
	if (cqd)
		win = (WindowPtr)NewCWindow(0, &r, ps("Archaic"), 1, ZOOMDOC,
			(WindowPtr)-1, 1, 0);
	else
		win = NewWindow(0, &r, ps("Archaic"), 1, ZOOMDOC, (WindowPtr)-1,
			1, 0);
	if (!win)
		return -1;
	SetPort(win);
	TextFont(GENEVA);
	TextSize(9);
	GetFontInfo(&fi);
	fasc = fi.ascent;
	fhgt = fi.ascent + fi.descent + fi.leading;
	ww = r.right - r.left;
	wh = r.bottom - r.top;
	sbrect(&b);
	sb = NewControl(win, &b, ps(""), 1, 0, 0, 0, scrollBarProc, 0);
	rgn = NewRgn();
	rgn2 = NewRgn();
	aclip();
	*w = ww - MSB;
	*h = wh;
	return 0;
}

void g_close()
{
	if (win)
		DisposeWindow(win);
	win = 0;
}

void g_title(s)
char *s;
{
	static unsigned char b[256];

	b[0] = (unsigned char)tomac(s, (int)strlen(s), b + 1);
	SetWTitle(win, b);
}

void g_fill(x, y, w, h, c)
int x, y, w, h, c;
{
	Rect r;

	SetRect(&r, x, y, x + w, y + h);
	FillRect(&r, c ? &qd.black : &qd.white);
}

void g_invert(x, y, w, h)
int x, y, w, h;
{
	Rect r;

	SetRect(&r, x, y, x + w, y + h);
	InvertRect(&r);
}

/* as X11's XDrawImageString: the letters' box is filled first */
void g_text(x, y, s, n, inv)
int x, y, n, inv;
char *s;
{
	static unsigned char b[256];
	Rect r;

	n = tomac(s, n, b);
	SetRect(&r, x, y, x + TextWidth((Ptr)b, 0, n), y + fhgt);
	FillRect(&r, inv ? &qd.black : &qd.white);
	TextMode(inv ? srcBic : srcOr);
	MoveTo(x, y + fasc);
	DrawText((Ptr)b, 0, n);
	TextMode(srcOr);
}

int g_textw(s, n)
char *s;
int n;
{
	static unsigned char b[256];

	n = tomac(s, n, b);
	return TextWidth((Ptr)b, 0, n);
}

void g_font(height, base)
int *height, *base;
{
	*height = fhgt;
	*base = fasc;
}

/* QuickDraw wants an even rowBytes: rows of an odd bpl are copied to
 * this buffer first, as many as it holds at a time */
static unsigned char eb[16384];

static void bits1(x, y, w, h, bits, bpl)
int x, y, w, h, bpl;
unsigned char *bits;
{
	BitMap bm;
	Rect d;

	bm.baseAddr = (Ptr)bits;
	bm.rowBytes = (short)bpl;
	SetRect(&bm.bounds, 0, 0, w, h);
	SetRect(&d, x, y, x + w, y + h);
	CopyBits(&bm, &win->portBits, &bm.bounds, &d, srcCopy, 0);
}

void g_bits(x, y, w, h, bits, bpl)
int x, y, w, h, bpl;
unsigned char *bits;
{
	int k, n, i, j;
	unsigned char *s, *d;

	if (w <= 0 || h <= 0)
		return;
	if (!(bpl & 1)) {
		bits1(x, y, w, h, bits, bpl);
		return;
	}
	n = (int)(sizeof eb / (bpl + 1));
	for (k = 0; k < h; k += n) {
		if (n > h - k)
			n = h - k;
		for (i = 0; i < n; ++i) {
			s = bits + (long)(k + i) * bpl;
			d = eb + (long)i * (bpl + 1);
			for (j = 0; j < bpl; ++j)
				d[j] = s[j];
		}
		bits1(x, y + k, w, n, eb, bpl + 1);
	}
}

long g_colors()
{
	GDHandle gd;
	int d;

	if (!cqd)
		return 2;
	gd = GetMainDevice();
	d = (**(**gd).gdPMap).pixelSize;
	if (d >= 16)
		return 1L << 24;
	return 1L << d;
}

/* g_pix's colors: a new table when pal has others */
static void colors(pal)
unsigned char *pal;
{
	ColorSpec *c;
	int i;

	if (!ctab) {
		ctab = (CTabHandle)NewHandle((long)sizeof(ColorTable) +
			255L * (long)sizeof(ColorSpec));
		if (!ctab)
			return;
	}
	for (i = 0; i < 768 && cset && cpal[i] == pal[i]; ++i)
		;
	if (cset && i == 768)
		return;
	(**ctab).ctSeed = GetCTSeed();
	(**ctab).ctFlags = 0;
	(**ctab).ctSize = 255;
	for (i = 0; i < 256; ++i) {
		c = &(**ctab).ctTable[i];
		c->value = (short)i;
		c->rgb.red = (unsigned short)(pal[3 * i] * 257);
		c->rgb.green = (unsigned short)(pal[3 * i + 1] * 257);
		c->rgb.blue = (unsigned short)(pal[3 * i + 2] * 257);
	}
	for (i = 0; i < 768; ++i)
		cpal[i] = pal[i];
	cset = 1;
}

static void pix1(x, y, w, h, pix, bpl)
int x, y, w, h, bpl;
unsigned char *pix;
{
	static PixMap pm;
	Rect d;

	pm.baseAddr = (Ptr)pix;
	pm.rowBytes = (short)(bpl | 0x8000);
	SetRect(&pm.bounds, 0, 0, w, h);
	pm.pmVersion = 0;
	pm.packType = 0;
	pm.packSize = 0;
	pm.hRes = 72L << 16;
	pm.vRes = 72L << 16;
	pm.pixelType = 0;
	pm.pixelSize = 8;
	pm.cmpCount = 1;
	pm.cmpSize = 8;
	pm.planeBytes = 0;
	pm.pmTable = ctab;
	SetRect(&d, x, y, x + w, y + h);
	HLock((Handle)ctab);
	CopyBits((BitMap *)&pm, &win->portBits, &pm.bounds, &d, srcCopy, 0);
	HUnlock((Handle)ctab);
}

void g_pix(x, y, w, h, pix, bpl, pal)
int x, y, w, h, bpl;
unsigned char *pix, *pal;
{
	int k, n, i, j;
	unsigned char *s, *d;

	if (!cqd || w <= 0 || h <= 0)
		return;
	colors(pal);
	if (!ctab)
		return;
	if (!(bpl & 1)) {
		pix1(x, y, w, h, pix, bpl);
		return;
	}
	n = (int)(sizeof eb / (bpl + 1));
	for (k = 0; k < h; k += n) {
		if (n > h - k)
			n = h - k;
		for (i = 0; i < n; ++i) {
			s = pix + (long)(k + i) * bpl;
			d = eb + (long)i * (bpl + 1);
			for (j = 0; j < bpl; ++j)
				d[j] = s[j];
		}
		pix1(x, y + k, w, n, eb, bpl + 1);
	}
}

/* x, y, w, h to dy rows down, if all of it shows (no other window or a
 * desk accessory is over it) */
int g_copy(x, y, w, h, dy)
int x, y, w, h, dy;
{
	Rect s, d, u;

	SetRect(&s, x, y, x + w, y + h);
	d = s;
	OffsetRect(&d, 0, dy);
	UnionRect(&s, &d, &u);
	RectRgn(rgn, &u);
	SectRgn(rgn, win->visRgn, rgn2);
	if (!EqualRgn(rgn, rgn2))
		return 0;
	CopyBits(&win->portBits, &win->portBits, &s, &d, srcCopy, 0);
	return 1;
}

void g_flush()
{
}

int g_fd()
{
	return -1;
}

long g_ms()
{
	unsigned long t = TickCount();

	return (long)(t / 3 * 50 + t % 3 * 50 / 3);
}

/* the page is at pos, 0 up to max; vis pixels of it show */
void g_sbar(pos, max, vis)
long pos, max, vis;
{
	if (!sb)
		return;
	smax = max;
	sk = max > 32000 ? (max + 31999) / 32000 : 1;
	fclip();
	if (max <= 0) {
		SetControlMaximum(sb, 0);
		SetControlValue(sb, 0);
	} else {
		SetControlMaximum(sb, (short)(max / sk));
		SetControlValue(sb, (short)(pos / sk));
	}
	aclip();
}

/* one step of part p of the scroll bar */
static void sbstep(p)
int p;
{
	if (p == inUpButton || p == inDownButton)
		put(E_SCROLL, 0, p == inUpButton ? -1 : 1, S_STEP);
	else if (p == inPageUp || p == inPageDown)
		put(E_SCROLL, 0, p == inPageUp ? -1 : 1, S_PAGE);
}

/* the part held down: its next step when it is due, while the pointer
 * is on it */
static void hold()
{
	Point p;
	int part;

	if (!StillDown()) {
		HiliteControl(sb, 0);
		trk = 0;
		return;
	}
	if ((long)TickCount() < tnext)
		return;
	GetMouse(&p);
	part = TestControl(sb, p);
	fclip();
	HiliteControl(sb, part == trk ? trk : 0);
	aclip();
	if (part != trk)
		return;
	sbstep(trk);
	tnext = (long)TickCount() + 6;
}

static void about()
{
	static char *l[] = {
		"A web browser for old systems, through WRP.",
		"https://github.com/tenox7/archaic",
		"Click to go on.",
		0
	};
	static char t[80];
	EventRecord ev;
	WindowPtr a;
	Rect r, sr;
	int i;

	sr = qd.screenBits.bounds;
	SetRect(&r, 0, 0, 320, 100);
	OffsetRect(&r, (sr.right - 320) / 2, (sr.bottom - 100) / 3);
	a = NewWindow(0, &r, ps(""), 1, dBoxProc, (WindowPtr)-1, 0, 0);
	if (!a)
		return;
	SetPort(a);
	sprintf(t, "Archaic v%s", VERSION);
	TextFont(0);
	TextSize(12);
	MoveTo(20, 28);
	DrawString(ps(t));
	TextFont(GENEVA);
	TextSize(9);
	for (i = 0; l[i]; ++i) {
		MoveTo(20, 50 + i * 14);
		DrawString(ps(l[i]));
	}
	for (;;) {
		if (wne) {
			if (!WaitNextEvent(mDownMask | keyDownMask | updateMask, &ev,
				30L, 0))
				continue;
		} else {
			SystemTask();
			if (!GetNextEvent(mDownMask | keyDownMask | updateMask, &ev))
				continue;
		}
		if (ev.what == mouseDown || ev.what == keyDown)
			break;
		if (ev.what == updateEvt && (WindowPtr)ev.message != a) {
			BeginUpdate((WindowPtr)ev.message);
			EndUpdate((WindowPtr)ev.message);
		}
	}
	DisposeWindow(a);
	SetPort(win);
	InvalRect(&win->portRect);
}

/* Paste: the clipboard's text as keys, but not its line ends */
static void paste()
{
	Handle h = NewHandle(0);
	long off, n, i;
	int c;

	if (!h)
		return;
	n = GetScrap(h, 'TEXT', &off);
	for (i = 0; i < n && i < NQ / 2; ++i) {
		c = (*h)[i] & 0377;
		if (c == '\r' || c == '\n')
			continue;
		put(E_KEY, 0, 0, c < 128 ? c : lat[c - 128] ? lat[c - 128] : '?');
	}
	DisposeHandle(h);
}

static void domenu(mr)
long mr;
{
	static unsigned char name[256];
	int m = HiWord(mr), i = LoWord(mr);

	if (m == MAPPLE) {
		if (i == 1)
			about();
		else {
			GetMenuItemText(mapple, i, name);
			OpenDeskAcc(name);
			SetPort(win);
		}
	} else if (m == MFILE && i == 1)
		put(E_QUIT, 0, 0, 0);
	else if (m == MEDIT && !SystemEdit(i - 1) && i == 5)
		paste();
	HiliteMenu(0);
}

static void key(ev)
EventRecord *ev;
{
	int c = (int)(ev->message & charCodeMask);

	if (ev->modifiers & cmdKey) {
		if (c == '.')
			put(E_KEY, 0, 0, 27);
		else if (ev->what == keyDown)
			domenu(MenuKey(c));
		return;
	}
	/* Control and a letter: System 6.0.8 gives the letter, with
	 * ControlKey in the modifiers. Archaic wants its control code. */
	if ((ev->modifiers & ControlKey) && c >= '@' && c < 0177)
		c &= 037;
	switch (c) {
	case 3:			/* Enter */
		c = '\r';
		break;
	case 0x1e:		/* the arrows */
	case 0x1f:
		put(E_SCROLL, 0, c == 0x1e ? -1 : 1, S_STEP);
		return;
	case 0x0b:		/* Page Up, Page Down */
	case 0x0c:
		put(E_SCROLL, 0, c == 0x0b ? -1 : 1, S_PAGE);
		return;
	case 0x01:		/* Home, End */
		put(E_SCROLL, 0, 0, S_TO);
		return;
	case 0x04:
		put(E_SCROLL, 0, (int)BIG, S_TO);
		return;
	case 0x1c:		/* left, right, Help, the F keys */
	case 0x1d:
	case 0x05:
	case 0x10:
		return;
	}
	if (c >= 128)
		c = lat[c - 128] ? lat[c - 128] : '?';
	put(E_KEY, 0, 0, c);
}

static void click(ev)
EventRecord *ev;
{
	static Rect drag;
	WindowPtr w;
	ControlHandle ch;
	Point p;
	Rect lim;
	long n;
	int part;

	part = FindWindow(ev->where, &w);
	switch (part) {
	case inMenuBar:
		domenu(MenuSelect(ev->where));
		break;
	case inSysWindow:
		SystemClick(ev, w);
		break;
	case inDrag:
		drag = qd.screenBits.bounds;
		InsetRect(&drag, 4, 4);
		DragWindow(w, ev->where, &drag);
		break;
	case inGoAway:
		if (TrackGoAway(w, ev->where))
			put(E_QUIT, 0, 0, 0);
		break;
	case inGrow:
		SetRect(&lim, 200, 120, qd.screenBits.bounds.right,
			qd.screenBits.bounds.bottom);
		n = GrowWindow(w, ev->where, &lim);
		if (n) {
			SizeWindow(w, LoWord(n), HiWord(n), 1);
			resized();
		}
		break;
	case inZoomIn:
	case inZoomOut:
		if (TrackBox(w, ev->where, part)) {
			fclip();
			EraseRect(&w->portRect);
			ZoomWindow(w, part, 1);
			resized();
		}
		break;
	case inContent:
		if (w != FrontWindow()) {
			SelectWindow(w);
			break;
		}
		p = ev->where;
		GlobalToLocal(&p);
		part = FindControl(p, w, &ch);
		if (ch == sb && part) {
			fclip();
			if (part == inThumb) {
				if (TrackControl(sb, p, 0) == inThumb)
					put(E_SCROLL, 0, (int)(GetControlValue(sb) *
						sk), S_TO);
			} else {
				trk = part;
				HiliteControl(sb, part);
				sbstep(part);
				tnext = (long)TickCount() + 24;
			}
			aclip();
		} else if (p.h < ww - MSB)
			put(E_DOWN, p.h, p.v, ev->modifiers & cmdKey ? 3 : 1);
		break;
	}
}

/* the window is active (on), or not: its scroll bar and grow box */
static void activate(on)
int on;
{
	if (!win || !sb)
		return;
	fclip();
	HiliteControl(sb, on ? 0 : 255);
	growbox();
}

static void handle(ev)
EventRecord *ev;
{
	WindowPtr w;
	Point a, b;
	Rect r;

	switch (ev->what) {
	case mouseDown:
		click(ev);
		break;
	case mouseUp:
		if (trk) {
			fclip();
			HiliteControl(sb, 0);
			aclip();
			trk = 0;
		} else {
			a = ev->where;
			GlobalToLocal(&a);
			put(E_UP, a.h, a.v, 1);
		}
		break;
	case keyDown:
	case autoKey:
		key(ev);
		break;
	case updateEvt:
		w = (WindowPtr)ev->message;
		if (w != win) {
			BeginUpdate(w);
			EndUpdate(w);
			break;
		}
		r = (**((WindowPeek)w)->updateRgn).rgnBBox;
		a.h = r.left;
		a.v = r.top;
		b.h = r.right;
		b.v = r.bottom;
		GlobalToLocal(&a);
		GlobalToLocal(&b);
		BeginUpdate(w);
		fclip();
		DrawControls(w);
		growbox();
		EndUpdate(w);
		put(E_EXPOSE, a.h, a.v, 0);
		q[(qin + NQ - 1) % NQ].w = b.h - a.h;
		q[(qin + NQ - 1) % NQ].h = b.v - a.v;
		break;
	case activateEvt:
		if ((WindowPtr)ev->message == win)
			activate(ev->modifiers & activeFlag);
		break;
	case osEvt:		/* suspend (another program is in front), resume */
		if (((ev->message >> 24) & 0377) == 1) {
			fg = (int)(ev->message & 1);
			activate(fg);
		}
		break;
	case kHighLevelEvent:	/* an Apple event: Quit, from Shut Down */
		if (aeok)
			AEProcessAppleEvent(ev);
		break;
	}
}

/* the next Mac event; 0 if none came in t ticks. The other programs
 * run meanwhile. */
static int next(ev, t)
EventRecord *ev;
long t;
{
	tyield = (long)TickCount();
	if (wne)
		return WaitNextEvent(everyEvent, ev, (unsigned long)t, 0);
	SystemTask();
	return GetNextEvent(everyEvent, ev);
}

/* the network waits: the other programs run, at most t ticks. An event
 * for Archaic goes to the queue, for g_event. */
void mac_wait(t)
long t;
{
	EventRecord ev;

	if (next(&ev, t))
		handle(&ev);
}

/* long work: the other programs run each 1/15 s */
void mac_yield()
{
	if ((long)TickCount() - tyield >= 4)
		mac_wait(0L);
}

int g_event(e, wait)
Event *e;
int wait;
{
	EventRecord ev;
	long t0 = (long)TickCount(), s;
	Event *f;

	for (;;) {
		if (qout != qin) {
			f = &q[qout];
			e->type = f->type;
			e->x = f->x;
			e->y = f->y;
			e->w = f->w;
			e->h = f->h;
			e->key = f->key;
			qout = (qout + 1) % NQ;
			return 1;
		}
		if (trk)
			hold();
		if (qout != qin)
			continue;
		/* sleep: the Mac and the other programs get the time */
		s = 0;
		if (wait == 1)
			s = trk ? 2 : fg ? 30 : 60;
		else if (wait == 2) {
			s = 6 - ((long)TickCount() - t0);
			if (s < 1)
				s = 1;
		}
		if (next(&ev, s)) {
			handle(&ev);
			continue;
		}
		if (!wait || (wait == 2 && (long)TickCount() - t0 >= 6))
			return 0;
	}
}

/* the words of the file "Archaic Options" in Archaic's folder, as the
 * command line's, after av[0] */
static int options(av, max)
char **av;
int max;
{
	static char b[256];
	long n = 255;
	short ref;
	int ac = 1;
	char *p;

	av[0] = "archaic";
	if (FSOpen(ps("Archaic Options"), 0, &ref) != noErr)
		return ac;
	if (FSRead(ref, &n, b) != noErr && n <= 0)
		n = 0;
	FSClose(ref);
	b[n] = 0;
	for (p = b; *p && ac < max - 1;) {
		while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
			*p++ = 0;
		if (!*p)
			break;
		av[ac++] = p;
		while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
			++p;
	}
	av[ac] = 0;
	return ac;
}

/* Apple events (System 7). The Finder sends Open Application at the
 * start, and Quit for Shut Down and Restart. */
static pascal OSErr aeopen(const AppleEvent *e, AppleEvent *r, int32_t k)
{
	return noErr;
}

static pascal OSErr aedocs(const AppleEvent *e, AppleEvent *r, int32_t k)
{
	return errAEEventNotHandled;
}

static pascal OSErr aequit(const AppleEvent *e, AppleEvent *r, int32_t k)
{
	put(E_QUIT, 0, 0, 0);
	return noErr;
}

int main()
{
	static char *av[32];
	SysEnvRec env;
	LONGINT g;
	int i;

	InitGraf(&qd.thePort);
	InitFonts();
	FlushEvents(everyEvent, 0);
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(0);
	InitCursor();
	MaxApplZone();
	for (i = 0; i < 4; ++i)
		MoreMasters();
	SysEnvirons(1, &env);
	cqd = env.hasColorQD;
	wne = NGetTrapAddress(0xA860, kToolboxTrapType) !=
		NGetTrapAddress(0xA89F, kToolboxTrapType);
	if (NGetTrapAddress(0xA1AD, kOSTrapType) !=	/* Gestalt */
		NGetTrapAddress(0xA89F, kToolboxTrapType)) {
		sys7 = Gestalt(gestaltSystemVersion, &g) == noErr && g >= 0x700;
		aeok = Gestalt(gestaltAppleEventsAttr, &g) == noErr && (g & 1);
	}
	if (aeok) {
		AEInstallEventHandler(kCoreEventClass, kAEOpenApplication,
			NewAEEventHandlerUPP(aeopen), 0, false);
		AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments,
			NewAEEventHandlerUPP(aedocs), 0, false);
		AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments,
			NewAEEventHandlerUPP(aedocs), 0, false);
		AEInstallEventHandler(kCoreEventClass, kAEQuitApplication,
			NewAEEventHandlerUPP(aequit), 0, false);
	}
	for (i = 0; i < 128; ++i)
		if (lat[i])
			mac[lat[i] - 128] = (unsigned char)(i + 128);
	mapple = NewMenu(MAPPLE, ps("\024"));
	AppendMenu(mapple, ps("About Archaic\311;(-"));
	AppendResMenu(mapple, 'DRVR');
	InsertMenu(mapple, 0);
	mfile = NewMenu(MFILE, ps("File"));
	AppendMenu(mfile, ps("Quit/Q"));
	InsertMenu(mfile, 0);
	medit = NewMenu(MEDIT, ps("Edit"));
	AppendMenu(medit, ps("Undo/Z;(-;Cut/X;Copy/C;Paste/V;Clear"));
	InsertMenu(medit, 0);
	DrawMenuBar();
	i = options(av, 32);
	return archaic_main(i, av);
}
