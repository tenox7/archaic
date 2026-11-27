/* ntwin.c - Archaic's display for Windows NT, 3.1 and newer, on each of
 * its CPUs (x86, MIPS, Alpha, PowerPC): a Win32 window, drawn with GDI.
 * Only calls that NT 3.1 has (the Win32 SDK of July 1993).
 *
 * The window has NT's own scroll bar: a scroll bar control at its
 * right (g_bar 0). Its arrows and its gray parts step while the button
 * is down, and the box can be dragged: the page moves with it. The
 * control does this in a loop of its own, so its WM_VSCROLL messages
 * call ui_poll(): main.c moves the page at once.
 *
 * The font: the ANSI variable font (MS Sans Serif). The colors: as the
 * screen shows them (GetDeviceCaps). A screen with a palette (256
 * colors): a logical palette of the colors that the pictures use, put
 * in as they are drawn, up to 236 (the 20 others are NT's). WRP's
 * pictures of 216 colors (GIP) have the web colors: the same on every
 * page, so they stay. When a picture needs more, its colors take the
 * palette's place. Pictures go to the screen as DIBs (SetDIBitsToDevice).
 *
 * Archaic is a console program, so that -t, -v, --help and the
 * messages of -u go to the console. Started from the Program Manager,
 * it has a console of its own: g_open lets it go (FreeConsole).
 */
#include "archaic.h"
#include <windows.h>
#include <stdlib.h>

#define QN 128			/* events not given yet */
#define NPAL 236		/* colors of our palette (256 less NT's 20) */
#define SKMAX 30000L		/* a scroll bar takes 16 bits */

static Event q[QN];
static int qn, qi;		/* events in q, the first one */
static HINSTANCE hinst;
static HWND hw, sb;		/* the window, its scroll bar */
static HDC dc;			/* the window's own (CS_OWNDC) */
static HFONT font;
static HPALETTE hpal;		/* a screen with a palette: ours */
static int ww, wh;		/* the window's inside, less the scroll bar */
static int sbw;			/* the scroll bar's width */
static int fh, fa;		/* the font's height and ascent */
static int cwid[256];		/* the width of each character */
static COLORREF tfg = 1, tbg = 1;	/* the text's colors now */
static int btn;			/* mouse buttons down */
static int inpoll;		/* the scroll bar's WM_VSCROLL calls ui_poll */
static int polled;		/*  and it did: g_event goes back to main.c */
static int trk;			/* the scroll bar's box is dragged */
static long sk = 1, smax, spos = -1;	/* g_sbar: pos / sk on the bar */
static int son = -1;		/* the bar's arrows work (1) or not (0) */
static int wheel;		/* the wheel's steps not used yet */
static PALETTEENTRY pe[NPAL];	/* a screen with a palette: hpal's colors */
static int npe;
static unsigned char ppal[768];	/* the pal of the last picture */
static long lpos;		/* the page's place, as g_sbar had it last */

int g_bar = 0;

static void push(t, x, y, w, h, k)
int t, x, y, w, h, k;
{
	Event *e;

	/* a button held down: only the last place counts */
	if (t == E_MOVE && qn > 0 && q[(qi + qn - 1) % QN].type == E_MOVE)
		e = &q[(qi + qn - 1) % QN];
	else if (qn < QN)
		e = &q[(qi + qn++) % QN];
	else
		return;
	e->type = t;
	e->x = x;
	e->y = y;
	e->w = w;
	e->h = h;
	e->key = k;
}

static void scrl(y, unit)
int y, unit;
{
	push(E_SCROLL, 0, y, 0, 0, unit);
}

/* the scroll bar: a step, a page, a place */
static void vscroll(code, pos)
int code, pos;
{
	long v;

	switch (code) {
	case SB_LINEUP:
		scrl(-1, S_STEP);
		break;
	case SB_LINEDOWN:
		scrl(1, S_STEP);
		break;
	case SB_PAGEUP:
		scrl(-1, S_PAGE);
		break;
	case SB_PAGEDOWN:
		scrl(1, S_PAGE);
		break;
	case SB_TOP:
		scrl(0, S_TO);
		break;
	case SB_BOTTOM:
		scrl((int)smax, S_TO);
		break;
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
		trk = code == SB_THUMBTRACK;
		v = (long)pos * sk;
		if (v > smax)
			v = smax;
		scrl((int)v, S_TO);
		if (!trk)
			spos = -1;	/* g_sbar puts the box there */
		break;
	case SB_ENDSCROLL:
		trk = 0;
		spos = -1;
		SetFocus(hw);	/* the keys go to the page again */
		break;
	}
	/* the bar's own loop runs (the button is down): the page moves now */
	if (!inpoll) {
		inpoll = 1;
		ui_poll();
		inpoll = 0;
		polled = 1;
	}
	if (code == SB_ENDSCROLL || code == SB_THUMBPOSITION)
		g_sbar(lpos, smax, 0L);	/* the box where the page is */
}

/* the mouse button b went down (d 1) or up */
static void mouse(b, d, lp)
int b, d;
LPARAM lp;
{
	int x = (short)LOWORD(lp), y = (short)HIWORD(lp);

	if (d) {
		if (!btn)
			SetCapture(hw);
		btn |= 1 << b;
		SetFocus(hw);
	} else {
		btn &= ~(1 << b);
		if (!btn)
			ReleaseCapture();
	}
	push(d ? E_DOWN : E_UP, x, y, 0, 0, b);
}

static LRESULT CALLBACK wproc(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
	RECT r;
	int k;

	switch (m) {
	case WM_PAINT:
		if (GetUpdateRect(h, &r, FALSE))
			push(E_EXPOSE, r.left, r.top, r.right - r.left,
				r.bottom - r.top, 0);
		ValidateRect(h, (RECT *)0);
		return 0;
	case WM_ERASEBKGND:
		return 1;	/* main.c draws all of it */
	case WM_SIZE:
		if (wp == SIZE_MINIMIZED)
			return 0;
		if (sb)
			MoveWindow(sb, (int)LOWORD(lp) - sbw, 0, sbw,
				(int)HIWORD(lp), TRUE);
		k = (int)LOWORD(lp) - sbw;
		if (k != ww || (int)HIWORD(lp) != wh) {
			ww = k;
			wh = (int)HIWORD(lp);
			push(E_SIZE, 0, 0, ww, wh, 0);
		}
		return 0;
	case WM_CHAR:
		push(E_KEY, 0, 0, 0, 0, (int)(wp & 0377));
		return 0;
	case WM_KEYDOWN:
		switch (wp) {
		case VK_PRIOR:
			scrl(-1, S_PAGE);
			return 0;
		case VK_NEXT:
			scrl(1, S_PAGE);
			return 0;
		case VK_UP:
			scrl(-1, S_STEP);
			return 0;
		case VK_DOWN:
			scrl(1, S_STEP);
			return 0;
		case VK_HOME:
			scrl(0, S_TO);
			return 0;
		case VK_END:
			scrl(0x7fffffff, S_TO);
			return 0;
		case VK_DELETE:
			push(E_KEY, 0, 0, 0, 0, 0177);
			return 0;
		}
		break;
	case WM_LBUTTONDOWN:
		mouse(1, 1, lp);
		return 0;
	case WM_LBUTTONUP:
		mouse(1, 0, lp);
		return 0;
	case WM_MBUTTONDOWN:
		mouse(2, 1, lp);
		return 0;
	case WM_MBUTTONUP:
		mouse(2, 0, lp);
		return 0;
	case WM_RBUTTONDOWN:
		mouse(3, 1, lp);
		return 0;
	case WM_RBUTTONUP:
		mouse(3, 0, lp);
		return 0;
	case WM_MOUSEMOVE:
		if (btn)
			push(E_MOVE, (short)LOWORD(lp), (short)HIWORD(lp), 0, 0, 0);
		return 0;
	case 0x020a:		/* WM_MOUSEWHEEL (NT 4.0): 120 a step */
		wheel += (short)HIWORD(wp);
		k = wheel / 120;
		wheel -= k * 120;
		if (k)
			scrl(-k, S_STEP);
		return 0;
	case WM_VSCROLL:
		if ((HWND)lp == sb)
			vscroll((int)LOWORD(wp), (int)(short)HIWORD(wp));
		return 0;
	case WM_CLOSE:
		push(E_QUIT, 0, 0, 0, 0, 0);
		return 0;
	case WM_QUERYNEWPALETTE:
		if (!hpal)
			break;
		SelectPalette(dc, hpal, FALSE);
		if (RealizePalette(dc))
			InvalidateRect(h, (RECT *)0, FALSE);
		return TRUE;
	case WM_PALETTECHANGED:
		if (!hpal || (HWND)wp == h)
			break;
		SelectPalette(dc, hpal, FALSE);
		RealizePalette(dc);
		InvalidateRect(h, (RECT *)0, FALSE);
		return 0;
	}
	return DefWindowProc(h, m, wp, lp);
}

/* Started from the Program Manager, Archaic has a console of its own,
 * with nothing in it yet: it goes. From a command prompt, the console
 * stays, for -v and the messages. */
static void noconsole()
{
	CONSOLE_SCREEN_BUFFER_INFO ci;

	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci) &&
		ci.dwCursorPosition.X == 0 && ci.dwCursorPosition.Y == 0)
		FreeConsole();
}

/* =WxH+X+Y: as fb_geom in fb.c. 1 size, 2 place, 4 X from the right,
 * 8 Y from the bottom */
static char *num(s, v)
char *s;
int *v;
{
	int n = 0;

	while (*s >= '0' && *s <= '9')
		n = n * 10 + *s++ - '0';
	*v = n;
	return s;
}

static int geom(g, w, h, x, y)
char *g;
int *w, *h, *x, *y;
{
	int f = 0, neg;

	if (!g)
		return 0;
	if (*g == '=')
		++g;
	if (*g >= '0' && *g <= '9') {
		g = num(g, w);
		if (*g == 'x' || *g == 'X') {
			g = num(g + 1, h);
			f |= 1;
		}
	}
	if (*g == '+' || *g == '-') {
		neg = *g == '-';
		g = num(g + 1, x);
		if (neg)
			f |= 4;
		if (*g == '+' || *g == '-') {
			if (*g == '-')
				f |= 8;
			g = num(g + 1, y);
			f |= 2;
		}
	}
	return f;
}

/* The window: =WxH is its inside, with the scroll bar. With no size:
 * 800 x 600, less on a smaller screen (9/10 of it at most). */
int g_open(gs, w, h)
char *gs;
int *w, *h;
{
	static WNDCLASS wc;
	static STARTUPINFO si;
	static LOGPALETTE lp;
	TEXTMETRIC tm;
	RECT r;
	DWORD st = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
	int f, gw = 800, gh = 600, x = 0, y = 0, sw, sh, ow, oh, px, py, show;

	hinst = GetModuleHandle((char *)0);
	noconsole();
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = wproc;
	wc.hInstance = hinst;
	wc.hIcon = LoadIcon(hinst, "ARCHAIC");
	if (!wc.hIcon)
		wc.hIcon = LoadIcon((HINSTANCE)0, IDI_APPLICATION);
	wc.hCursor = LoadCursor((HINSTANCE)0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)0;
	wc.lpszClassName = "Archaic";
	if (!RegisterClass(&wc))
		return -1;
	sbw = GetSystemMetrics(SM_CXVSCROLL);
	sw = GetSystemMetrics(SM_CXSCREEN);
	sh = GetSystemMetrics(SM_CYSCREEN);
	f = geom(gs, &gw, &gh, &x, &y);
	if (gw < 200)
		gw = 200;
	if (gh < 100)
		gh = 100;
	r.left = r.top = 0;
	r.right = gw;
	r.bottom = gh;
	AdjustWindowRect(&r, st, FALSE);
	ow = r.right - r.left;
	oh = r.bottom - r.top;
	if (!(f & 1)) {		/* no size: 9/10 of the screen at most */
		if (ow > sw * 9 / 10) {
			gw -= ow - sw * 9 / 10;
			ow = sw * 9 / 10;
		}
		if (oh > sh * 9 / 10) {
			gh -= oh - sh * 9 / 10;
			oh = sh * 9 / 10;
		}
	}
	px = py = CW_USEDEFAULT;
	if (f & 2) {
		px = f & 4 ? sw - ow - x : x;
		py = f & 8 ? sh - oh - y : y;
	}
	hw = CreateWindow("Archaic", "Archaic", st, px, py, ow, oh,
		(HWND)0, (HMENU)0, hinst, (LPVOID)0);
	if (!hw)
		return -1;
	GetClientRect(hw, &r);
	sb = CreateWindow("SCROLLBAR", (char *)0, WS_CHILD | WS_VISIBLE |
		SBS_VERT, r.right - sbw, 0, sbw, r.bottom, hw, (HMENU)1, hinst,
		(LPVOID)0);
	dc = GetDC(hw);
	font = GetStockObject(ANSI_VAR_FONT);
	SelectObject(dc, font);
	SetBkMode(dc, OPAQUE);
	SetTextAlign(dc, TA_TOP | TA_LEFT | TA_NOUPDATECP);
	GetTextMetrics(dc, &tm);
	fh = tm.tmHeight;
	fa = tm.tmAscent;
	if (!GetCharWidth(dc, 0, 255, cwid))
		for (f = 0; f < 256; ++f)
			cwid[f] = tm.tmAveCharWidth;
	if (GetDeviceCaps(dc, RASTERCAPS) & RC_PALETTE) {
		/* black and white (DIBs of 1 bit), and the pictures' colors */
		lp.palVersion = 0x300;
		lp.palNumEntries = 1;
		hpal = CreatePalette(&lp);
		pe[1].peRed = pe[1].peGreen = pe[1].peBlue = 255;
		npe = 2;
		if (hpal) {
			ResizePalette(hpal, npe);
			SetPaletteEntries(hpal, 0, npe, pe);
		}
		if (hpal) {
			SelectPalette(dc, hpal, FALSE);
			RealizePalette(dc);
		}
	}
	g_sbar(0L, 0L, 0L);
	si.cb = sizeof si;
	GetStartupInfo(&si);
	show = si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWNORMAL;
	ShowWindow(hw, show);
	GetClientRect(hw, &r);
	ww = r.right - sbw;
	wh = r.bottom;
	*w = ww;
	*h = wh;
	return 0;
}

void g_close()
{
	if (hw)
		DestroyWindow(hw);
	hw = 0;
}

void g_title(s)
char *s;
{
	SetWindowText(hw, s);
}

void g_fill(x, y, w, h, c)
int x, y, w, h, c;
{
	PatBlt(dc, x, y, w, h, c ? BLACKNESS : WHITENESS);
}

void g_invert(x, y, w, h)
int x, y, w, h;
{
	PatBlt(dc, x, y, w, h, DSTINVERT);
}

void g_text(x, y, s, n, inv)
int x, y, n, inv;
char *s;
{
	COLORREF fg = inv ? RGB(255, 255, 255) : RGB(0, 0, 0);
	COLORREF bg = fg ^ 0xffffffL;

	if (fg != tfg)
		SetTextColor(dc, tfg = fg);
	if (bg != tbg)
		SetBkColor(dc, tbg = bg);
	TextOut(dc, x, y, s, n);
}

int g_textw(s, n)
char *s;
int n;
{
	int w = 0;

	while (n-- > 0)
		w += cwid[*s++ & 0377];
	return w;
}

void g_font(height, base)
int *height, *base;
{
	*height = fh;
	*base = fa;
}

/* A DIB of w x h pixels, bits a pixel: rows of 4 bytes each (bpl), from
 * the bottom up */
static struct {
	BITMAPINFOHEADER h;
	RGBQUAD c[256];
} bi;
static unsigned char *dib;
static long ndib;

static unsigned char *mkdib(w, h, bits, dbpl)
int w, h, bits, *dbpl;
{
	long n;

	*dbpl = ((w * bits + 31) >> 5) << 2;
	n = (long)*dbpl * h;
	if (n > ndib) {
		if (dib)
			free(dib);
		dib = (unsigned char *)malloc((size_t)n);
		ndib = dib ? n : 0;
		if (!dib)
			return 0;
	}
	bi.h.biSize = sizeof bi.h;
	bi.h.biWidth = w;
	bi.h.biHeight = h;
	bi.h.biPlanes = 1;
	bi.h.biBitCount = bits;
	bi.h.biCompression = BI_RGB;
	bi.h.biClrUsed = 0;
	return dib;
}

static void put(x, y, w, h, s, bpl, n, dbpl)
int x, y, w, h, bpl, n, dbpl;
unsigned char *s;
{
	unsigned char *d;
	int j, i;

	for (j = 0; j < h; ++j) {
		d = dib + (long)(h - 1 - j) * dbpl;
		for (i = 0; i < n; ++i)
			d[i] = s[i];
		s += bpl;
	}
	SetDIBitsToDevice(dc, x, y, w, h, 0, 0, 0, h, dib,
		(BITMAPINFO *)&bi, DIB_RGB_COLORS);
}

/* rows of 1-bit pixels, 1 black, high bit left */
void g_bits(x, y, w, h, bits, bpl)
int x, y, w, h, bpl;
unsigned char *bits;
{
	int db;

	if (w <= 0 || h <= 0 || !mkdib(w, h, 1, &db))
		return;
	bi.c[0].rgbRed = bi.c[0].rgbGreen = bi.c[0].rgbBlue = 255;
	bi.c[1].rgbRed = bi.c[1].rgbGreen = bi.c[1].rgbBlue = 0;
	bi.c[0].rgbReserved = bi.c[1].rgbReserved = 0;
	put(x, y, w, h, bits, bpl, (w + 7) >> 3, db);
}

/* how many colors the screen shows */
long g_colors()
{
	int b = GetDeviceCaps(dc, BITSPIXEL) * GetDeviceCaps(dc, PLANES);

	if (hpal)
		return 256L;
	if (b <= 1)
		return 2L;
	return b >= 24 ? 1L << 24 : 1L << b;
}

/* Our palette (addcols): black, white, and the colors of pal that w x h
 * pixels use. When it is full, a new pal (another picture's) takes its
 * place; for the same pal the colors that do not fit get the nearest
 * one. 0 if nothing changed */

static int addcols(pix, w, h, bpl, pal)
unsigned char *pix, *pal;
int w, h, bpl;
{
	static unsigned char use[256];
	unsigned char *b, *c;
	int i, j, n, again = 0, newp;

	for (i = 0; i < 768 && pal[i] == ppal[i]; ++i)
		;
	newp = i < 768;
	for (; i < 768; ++i)
		ppal[i] = pal[i];
	for (i = 0; i < 256; ++i)
		use[i] = 0;
	for (j = 0; j < h; ++j)
		for (b = pix + (long)j * bpl, i = 0; i < w; ++i)
			use[b[i]] = 1;
	for (n = npe;;) {
		for (i = 0; i < 256; ++i) {
			if (!use[i])
				continue;
			c = pal + 3 * i;
			for (j = 0; j < n; ++j)
				if (pe[j].peRed == c[0] && pe[j].peGreen == c[1] &&
					pe[j].peBlue == c[2])
					break;
			if (j < n)
				continue;
			if (n == NPAL)
				break;
			pe[n].peRed = c[0];
			pe[n].peGreen = c[1];
			pe[n].peBlue = c[2];
			pe[n].peFlags = 0;
			++n;
		}
		if (i == 256 || again || !newp)
			break;
		n = 2;		/* full: this picture's colors take its place */
		again = 1;
	}
	if (n == npe && !again)
		return 0;
	npe = n;
	ResizePalette(hpal, npe);
	SetPaletteEntries(hpal, 0, npe, pe);
	RealizePalette(dc);
	return 1;
}

/* rows of 8-bit color numbers; pal: the red, green, blue of each */
void g_pix(x, y, w, h, pix, bpl, pal)
int x, y, w, h, bpl;
unsigned char *pix, *pal;
{
	int db, i;

	if (w <= 0 || h <= 0 || !mkdib(w, h, 8, &db))
		return;
	if (hpal)
		addcols(pix, w, h, bpl, pal);
	for (i = 0; i < 256; ++i) {
		bi.c[i].rgbRed = pal[3 * i];
		bi.c[i].rgbGreen = pal[3 * i + 1];
		bi.c[i].rgbBlue = pal[3 * i + 2];
		bi.c[i].rgbReserved = 0;
	}
	put(x, y, w, h, pix, bpl, w, db);
}

/* a scroll: the pixels move. 0 if a part of them was not there
 * (covered, or not drawn yet): main.c then draws all of it */
int g_copy(x, y, w, h, dy)
int x, y, w, h, dy;
{
	RECT rs, rc, rv;
	HRGN u, v;
	int i;

	if (GetUpdateRect(hw, (RECT *)0, FALSE))
		return 0;
	for (i = 0; i < qn; ++i)
		if (q[(qi + i) % QN].type == E_EXPOSE)
			return 0;
	rs.left = rc.left = rv.left = x;
	rs.right = rc.right = rv.right = x + w;
	rs.top = y;
	rs.bottom = y + h;
	rc.top = dy < 0 ? y + dy : y;
	rc.bottom = dy < 0 ? y + h : y + h + dy;
	rv.top = dy < 0 ? y + h + dy : y;	/* the rows that come in */
	rv.bottom = dy < 0 ? y + h : y + dy;
	u = CreateRectRgn(0, 0, 0, 0);
	v = CreateRectRgnIndirect(&rv);
	i = u && v && ScrollDC(dc, 0, dy, &rs, &rc, u, (RECT *)0) &&
		CombineRgn(u, u, v, RGN_DIFF) == NULLREGION;
	if (u)
		DeleteObject(u);
	if (v)
		DeleteObject(v);
	return i;
}

void g_flush()
{
	GdiFlush();
}

int g_fd()
{
	return -1;
}

/* a clock in milliseconds (for the spinner) */
long g_ms()
{
	return (long)(GetTickCount() & 0x3fffffffL);
}

/* The page's place, on the scroll bar. It takes 16 bits: on a long page
 * a step of the bar is sk pixels */
void g_sbar(pos, max, vis)
long pos, max, vis;
{
	int on = max > 0;

	if (!sb)
		return;
	lpos = pos;
	if (on != son) {
		son = on;
		EnableScrollBar(sb, SB_CTL, on ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH);
	}
	if (!on) {
		smax = 0;
		SetScrollRange(sb, SB_CTL, 0, 0, TRUE);
		spos = -1;
		return;
	}
	if (max != smax) {
		smax = max;
		sk = max > SKMAX ? (max + SKMAX - 1) / SKMAX : 1;
		SetScrollRange(sb, SB_CTL, 0, (int)(max / sk), FALSE);
		spos = -1;
	}
	if (!trk && pos / sk != spos) {	/* not while its box is dragged */
		spos = pos / sk;
		SetScrollPos(sb, SB_CTL, (int)spos, TRUE);
	}
}

int g_event(e, wait)
Event *e;
int wait;
{
	MSG m;

	for (;;) {
		if (qn > 0) {
			*e = q[qi];
			qi = (qi + 1) % QN;
			--qn;
			return 1;
		}
		/* in the scroll bar's loop: its messages are its own */
		if (inpoll)
			return 0;
		if (PeekMessage(&m, (HWND)0, 0, 0, PM_REMOVE)) {
			if (m.message == WM_QUIT)
				push(E_QUIT, 0, 0, 0, 0, 0);
			TranslateMessage(&m);
			DispatchMessage(&m);
			/* the scroll bar's loop ran, and main.c moved the page
			 * in it: main.c now gets the slices that it needs */
			if (polled && !qn) {
				polled = 0;
				return 0;
			}
			polled = 0;
			continue;
		}
		if (!wait)
			return 0;
		if (wait == 2) {	/* 1/10 s at most */
			if (MsgWaitForMultipleObjects(0, (HANDLE *)0, FALSE, 100,
				QS_ALLINPUT) == WAIT_TIMEOUT)
				return 0;
		} else
			WaitMessage();
	}
}

/* update.c: the program's file */
int nt_self(s, n)
char *s;
int n;
{
	return GetModuleFileName((HINSTANCE)0, s, n) > 0;
}
