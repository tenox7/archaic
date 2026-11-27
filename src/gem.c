/* gem.c - Archaic's display for GEM on the Atari (FreeMiNT with XaAES,
 * as on ARAnyM): a GEM window that shows fb.c's screen in memory,
 * through the VDI.
 *
 * The screen: vq_scrninfo() says how its pixels are made. Packed pixels
 * of 15 to 32 bits (ARAnyM's fVDI, graphics cards, the Falcon's 16 bits)
 * show the colors; 32-bit 0x00RRGGBB (ARAnyM) takes fb's rows as they
 * are. Other screens (2 to 256 colors, bit planes) show black and white,
 * and g_colors() is 2.
 *
 * The window has GEM's scroll bar (g_bar 0): its arrows scroll a
 * quarter of the window (E_SCROLL S_STEP), its grey part a window
 * (S_PAGE), the slider to a place (S_TO). Keys: the up and down arrows
 * scroll a quarter, with Shift a window; ClrHome goes to the top, Shift
 * ClrHome to the end. Characters come in the Atari's character set and
 * go to Archaic as ISO 8859-1. Ctrl+Q, the close box and AP_TERM quit.
 * GEM has no file to wait on: g_fd() is -1, and nsock.c waits 1/20 s
 * at most.
 */
#include "fb.h"
#include <gem.h>
#include <gemx.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define KIND (NAME | CLOSER | FULLER | MOVER | SIZER | UPARROW | DNARROW | VSLIDE)
#define BIG 2000000000L		/* S_TO past the end */
#define NQ 8			/* events from one evnt_multi */
#define MINW 200		/* the smallest work area */
#define MINH 100

#define M_MONO 0		/* black and white, with vrt_cpyfm */
#define M_RGB32 1		/* 0x00RRGGBB: fb's pixels as they are */
#define M_PACK 2		/* packed pixels: rt[], gt[], bt[] */

int g_bar = 0;

static short ap = -1;		/* the AES's id for Archaic */
static short vh;		/* the VDI workstation */
static short wh = -1;		/* the window */
static int sw, sh;		/* the screen's size */
static short wx, wy, wkw, wkh;	/* the work area, on the screen */
static int mode, planes, bpp, swap;	/* the screen's pixels (screen()) */
static unsigned long rt[256], gt[256], bt[256];	/* red, green, blue: pixel bits */
static MFDB scr;		/* fd_addr 0: the screen */
static unsigned char *tmp;	/* a part of fb in the screen's format */
static long ntmp;
static char wtitle[100];		/* the AES keeps a pointer to it */
static long smax = -1;		/* g_sbar's max */
static int lsize = -1, lpos = -1;	/* the slider as it was set */
static Event q[NQ];		/* events not given yet */
static int nq, iq;
static int held;		/* a mouse button is down */
static int down;		/* ... in the window: 1 left, 3 right */
static short mx, my;		/* the mouse, on the screen */

/* the Atari's characters 0200 to 0377 in ISO 8859-1; 0: none there */
static unsigned char latin[128] = {
	0307, 0374, 0351, 0342, 0344, 0340, 0345, 0347,	/* 0200 */
	0352, 0353, 0350, 0357, 0356, 0354, 0304, 0305,
	0311, 0346, 0306, 0364, 0366, 0362, 0373, 0371,	/* 0220 */
	0377, 0326, 0334, 0242, 0243, 0245, 0337, 0,
	0341, 0355, 0363, 0372, 0361, 0321, 0252, 0272,	/* 0240 */
	0277, 0, 0254, 0275, 0274, 0241, 0253, 0273,
	0343, 0365, 0330, 0370, 0, 0, 0300, 0303,	/* 0260 */
	0325, 0250, 0264, 0, 0266, 0251, 0256, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0300 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0247, 0, 0,	/* 0320 */
	0, 0, 0, 0, 0, 0, 0265, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 0340 */
	0, 0261, 0, 0, 0, 0, 0367, 0, 0260, 0, 0267, 0, 0, 0262, 0263, 0257	/* 0360 */
};

/* a channel's pixel bits for each value 0 to 255: n bits, bit i of
 * the n (the low first) at pixel bit pos[i] */
static void chan(t, n, pos)
unsigned long *t;
int n;
short *pos;
{
	int v, i, k;

	for (v = 0; v < 256; ++v) {
		t[v] = 0;
		for (i = 0; i < n; ++i) {
			k = 8 - n + i;
			if (k >= 0 && (v >> k & 1) && pos[i] >= 0 && pos[i] < 32)
				t[v] |= (unsigned long)1 << pos[i];
		}
	}
}

/* how the screen's pixels are made: mode, planes, bpp, swap */
static void screen()
{
	static short ex[57], si[273];
	int i;

	vq_extnd(vh, 1, ex);
	planes = ex[4];
	mode = M_MONO;
	if (planes != 15 && planes != 16 && planes != 24 && planes != 32)
		return;
	for (i = 0; i < 273; ++i)
		si[i] = 0;
	vq_scrninfo(vh, si);	/* an old VDI leaves it all 0 */
	if (si[0] != 2 || si[1] != 2 || si[8] < 1 || si[9] < 1 || si[10] < 1)
		return;		/* not packed pixels with no palette */
	chan(rt, si[8], si + 16);
	chan(gt, si[9], si + 32);
	chan(bt, si[10], si + 48);
	swap = (si[14] & 0x80) != 0;	/* Intel's byte order */
	bpp = planes == 15 ? 2 : planes / 8;
	mode = M_PACK;
	if (planes != 32 || swap)
		return;
	for (i = 0; i < 256; ++i)
		if (rt[i] != (unsigned long)i << 16 || gt[i] != (unsigned long)i << 8 ||
			bt[i] != (unsigned long)i)
			return;
	mode = M_RGB32;
}

/* the work area on the screen */
static void work()
{
	wind_get(wh, WF_WORKXYWH, &wx, &wy, &wkw, &wkh);
}

static Event *put(type)
int type;
{
	Event *e = &q[nq < NQ ? nq++ : NQ - 1];

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

/* room for n bytes in tmp */
static int room(n)
long n;
{
	if (n <= ntmp)
		return 0;
	if (tmp)
		free((char *)tmp);
	tmp = (unsigned char *)malloc((unsigned long)n);
	ntmp = tmp ? n : 0;
	return tmp ? 0 : -1;
}

/* fb's part x, y, w, h into tmp, in the screen's format; m says how.
 * 0 ok, -1 no memory */
static int conv(m, x, y, w, h)
MFDB *m;
int x, y, w, h;
{
	int tw = (w + 15) & ~15, i, j, n;
	long bpl;
	unsigned int *s, c;
	unsigned long v;
	unsigned char *d;

	if (mode == M_MONO)
		bpl = tw / 8;
	else
		bpl = (long)tw * bpp;
	if (room(bpl * h) < 0)
		return -1;
	m->fd_addr = (char *)tmp;
	m->fd_w = tw;
	m->fd_h = h;
	m->fd_wdwidth = tw / 16;
	m->fd_stand = mode == M_MONO;
	m->fd_nplanes = mode == M_MONO ? 1 : planes;
	m->fd_r1 = m->fd_r2 = m->fd_r3 = 0;
	for (j = 0; j < h; ++j) {
		s = fb + (long)(y + j) * fbw + x;
		d = tmp + bpl * j;
		if (mode == M_RGB32) {
			memcpy((char *)d, (char *)s, (unsigned long)w * 4);
			continue;
		}
		if (mode == M_MONO) {
			for (i = 0; i < bpl; ++i)
				d[i] = 0;
			for (i = 0; i < w; ++i) {
				c = s[i];
				if ((c >> 16 & 255) * 30 + (c >> 8 & 255) * 59 +
					(c & 255) * 11 < 12800)
					d[i >> 3] |= 0x80 >> (i & 7);
			}
			continue;
		}
		for (i = 0; i < w; ++i) {
			c = s[i];
			v = rt[c >> 16 & 255] | gt[c >> 8 & 255] | bt[c & 255];
			if (swap)
				for (n = 0; n < bpp; ++n, v >>= 8)
					*d++ = (unsigned char)v;
			else
				for (n = bpp - 1; n >= 0; --n)
					*d++ = (unsigned char)(v >> (n * 8));
		}
	}
	return 0;
}

/* fb's part x, y, w, h (in the window) to the screen, in the parts of
 * the window that show */
static void show(x, y, w, h)
int x, y, w, h;
{
	static MFDB m;
	static short pxy[8], col[2];
	static short rx, ry, rw, rh;
	int ox = 0, oy = 0, x0, y0, x1, y1;

	if (x < 0) {
		w += x;
		x = 0;
	}
	if (y < 0) {
		h += y;
		y = 0;
	}
	if (x + w > fbw)
		w = fbw - x;
	if (y + h > fbh)
		h = fbh - y;
	if (wh < 0 || w <= 0 || h <= 0)
		return;
	if (mode == M_RGB32 && fbw % 16 == 0) {
		m.fd_addr = (char *)fb;
		m.fd_w = fbw;
		m.fd_h = fbh;
		m.fd_wdwidth = fbw / 16;
		m.fd_stand = 0;
		m.fd_nplanes = 32;
		m.fd_r1 = m.fd_r2 = m.fd_r3 = 0;
	} else {
		if (conv(&m, x, y, w, h) < 0)
			return;
		ox = x;
		oy = y;
	}
	col[0] = 1;		/* black for the 1 bits */
	col[1] = 0;
	wind_update(BEG_UPDATE);
	graf_mouse(M_OFF, (MFORM *)0);
	wind_get(wh, WF_FIRSTXYWH, &rx, &ry, &rw, &rh);
	while (rw > 0 && rh > 0) {
		x0 = rx > wx + x ? rx : wx + x;
		y0 = ry > wy + y ? ry : wy + y;
		x1 = rx + rw < wx + x + w ? rx + rw : wx + x + w;
		y1 = ry + rh < wy + y + h ? ry + rh : wy + y + h;
		if (x0 < 0)
			x0 = 0;
		if (y0 < 0)
			y0 = 0;
		if (x1 > sw)
			x1 = sw;
		if (y1 > sh)
			y1 = sh;
		if (x0 < x1 && y0 < y1) {
			pxy[0] = x0 - wx - ox;
			pxy[1] = y0 - wy - oy;
			pxy[2] = x1 - 1 - wx - ox;
			pxy[3] = y1 - 1 - wy - oy;
			pxy[4] = x0;
			pxy[5] = y0;
			pxy[6] = x1 - 1;
			pxy[7] = y1 - 1;
			if (mode == M_MONO)
				vrt_cpyfm(vh, MD_REPLACE, pxy, &m, &scr, col);
			else
				vro_cpyfm(vh, S_ONLY, pxy, &m, &scr);
		}
		wind_get(wh, WF_NEXTXYWH, &rx, &ry, &rw, &rh);
	}
	graf_mouse(M_ON, (MFORM *)0);
	wind_update(END_UPDATE);
}

int g_open(geom, w, h)
char *geom;
int *w, *h;
{
	static short wi[11], wo[57];
	static short d, dx, dy, dw, dh, bx, by, bw, bh;
	int gw = 800, gh = 600, x = 0, y = 0, f, i;

	ap = appl_init();
	if (ap < 0)
		return -1;
	shel_write(9, 1, 0, (char *)0, (char *)0);	/* Archaic knows AP_TERM */
	menu_register(ap, "  Archaic");
	vh = graf_handle(&d, &d, &d, &d);
	for (i = 0; i < 10; ++i)
		wi[i] = 1;
	wi[10] = 2;		/* raster coordinates */
	v_opnvwk(wi, &vh, wo);
	if (vh <= 0) {
		appl_exit();
		return -1;
	}
	sw = wo[0] + 1;
	sh = wo[1] + 1;
	screen();
	wind_get(0, WF_WORKXYWH, &dx, &dy, &dw, &dh);	/* the desktop */
	f = fb_geom(geom, &gw, &gh, &x, &y);
	if (gw < MINW)
		gw = MINW;
	if (gh < MINH)
		gh = MINH;
	wind_calc(WC_BORDER, KIND, 0, 0, gw, gh, &bx, &by, &bw, &bh);
	if (bw > dw) {
		gw -= bw - dw;
		bw = dw;
	}
	if (bh > dh) {
		gh -= bh - dh;
		bh = dh;
	}
	if (f & 2) {
		bx = f & 4 ? dx + dw - bw - x : dx + x;
		by = f & 8 ? dy + dh - bh - y : dy + y;
	} else {
		bx = dx + (dw - bw) / 2;
		by = dy + (dh - bh) / 2;
	}
	if (bx < dx)
		bx = dx;
	if (by < dy)
		by = dy;
	wh = wind_create(KIND, dx, dy, dw, dh);
	if (wh < 0) {
		v_clsvwk(vh);
		appl_exit();
		return -1;
	}
	strcpy(wtitle, "Archaic");
	wind_set_str(wh, WF_NAME, wtitle);
	wind_set(wh, WF_VSLSIZE, 1000, 0, 0, 0);
	wind_set(wh, WF_VSLIDE, 0, 0, 0, 0);
	wind_open(wh, bx, by, bw, bh);
	work();
	graf_mouse(ARROW, (MFORM *)0);
	if (fb_size(wkw, wkh) < 0)
		return -1;
	*w = wkw;
	*h = wkh;
	return 0;
}

/* how many colors the screen shows */
long g_colors()
{
	if (mode == M_MONO)
		return 2L;
	return planes >= 24 ? 1L << 24 : 1L << planes;
}

void g_close()
{
	if (wh >= 0) {
		wind_close(wh);
		wind_delete(wh);
		wh = -1;
	}
	if (vh > 0)
		v_clsvwk(vh);
	vh = 0;
	if (ap >= 0)
		appl_exit();
	ap = -1;
}

/* the title is ISO 8859-1, GEM's is the Atari's characters */
void g_title(s)
char *s;
{
	int c, i, n = 0;

	for (; *s && n < (int)sizeof wtitle - 1; ++s) {
		c = *s & 0377;
		if (c >= 0200) {
			for (i = 0; i < 128 && latin[i] != c; ++i)
				;
			c = i < 128 ? i + 0200 : '?';
		}
		wtitle[n++] = c;
	}
	wtitle[n] = 0;
	if (wh >= 0)
		wind_set_str(wh, WF_NAME, wtitle);
}

void g_flush()
{
	if (fbx0 < fbx1 && fby0 < fby1)
		show(fbx0, fby0, fbx1 - fbx0, fby1 - fby0);
	fb_done();
}

int g_fd()
{
	return -1;
}

/* a clock in milliseconds (for the spinner) */
long g_ms()
{
	struct timeval tv;

	gettimeofday(&tv, (struct timezone *)0);
	return (tv.tv_sec * 1000L + tv.tv_usec / 1000) & 0x3fffffffL;
}

/* the page is at pos (0 to max), vis pixels of it show: the slider,
 * in GEM's 0 to 1000 */
void g_sbar(pos, max, vis)
long pos, max, vis;
{
	long a = vis, b = max + vis;
	int s, p;

	smax = max;
	if (wh < 0 || vis <= 0)
		return;
	while (b > 1000000L) {
		a /= 10;
		b /= 10;
	}
	s = b > 0 ? (int)(a * 1000 / b) : 1000;
	if (s < 1)
		s = 1;
	if (s > 1000)
		s = 1000;
	a = pos;
	b = max;
	while (b > 1000000L) {
		a /= 10;
		b /= 10;
	}
	p = b > 0 ? (int)(a * 1000 / b) : 0;
	if (p < 0)
		p = 0;
	if (p > 1000)
		p = 1000;
	if (s != lsize)
		wind_set(wh, WF_VSLSIZE, s, 0, 0, 0);
	if (p != lpos)
		wind_set(wh, WF_VSLIDE, p, 0, 0, 0);
	lsize = s;
	lpos = p;
}

/* the window has a new size: a new fb, and E_SIZE */
static void sized()
{
	Event *e;

	work();
	if (wkw == fbw && wkh == fbh)
		return;
	if (fb_size(wkw, wkh) < 0) {
		put(E_QUIT);
		return;
	}
	e = put(E_SIZE);
	e->w = wkw;
	e->h = wkh;
}

/* WM_SIZED: the window to x, y, w, h, with at least MINW x MINH inside */
static void resize(x, y, w, h)
int x, y, w, h;
{
	static short a, b, c, d;

	wind_calc(WC_WORK, KIND, x, y, w, h, &a, &b, &c, &d);
	if (c < MINW)
		c = MINW;
	if (d < MINH)
		d = MINH;
	wind_calc(WC_BORDER, KIND, a, b, c, d, &a, &b, &c, &d);
	wind_set(wh, WF_CURRXYWH, a, b, c, d);
	sized();
}

/* WM_FULLED: the full size, or back */
static void full()
{
	static short x, y, w, h, fx, fy, fw, fh;

	wind_get(wh, WF_CURRXYWH, &x, &y, &w, &h);
	wind_get(wh, WF_FULLXYWH, &fx, &fy, &fw, &fh);
	if (x == fx && y == fy && w == fw && h == fh)
		wind_get(wh, WF_PREVXYWH, &fx, &fy, &fw, &fh);
	wind_set(wh, WF_CURRXYWH, fx, fy, fw, fh);
	sized();
}

static void mesg(m)
short *m;
{
	long y;

	switch (m[0]) {
	case WM_REDRAW:
		if (m[3] == wh)
			show(m[4] - wx, m[5] - wy, m[6], m[7]);
		break;
	case WM_TOPPED:
		if (m[3] == wh)
			wind_set(wh, WF_TOP, 0, 0, 0, 0);
		break;
	case WM_CLOSED:
	case AP_TERM:
		put(E_QUIT);
		break;
	case WM_MOVED:
		if (m[3] == wh) {
			wind_set(wh, WF_CURRXYWH, m[4], m[5], m[6], m[7]);
			work();
		}
		break;
	case WM_SIZED:
		if (m[3] == wh)
			resize(m[4], m[5], m[6], m[7]);
		break;
	case WM_FULLED:
		if (m[3] == wh)
			full();
		break;
	case WM_ARROWED:
		if (m[3] != wh)
			break;
		switch (m[4]) {
		case WA_UPPAGE:
			scrl(-1, S_PAGE);
			break;
		case WA_DNPAGE:
			scrl(1, S_PAGE);
			break;
		case WA_UPLINE:
			scrl(-1, S_STEP);
			break;
		case WA_DNLINE:
			scrl(1, S_STEP);
			break;
		}
		break;
	case WM_VSLID:
		if (m[3] != wh || smax < 0)
			break;
		y = smax > 1000000L ? smax / 1000 * m[4] : smax * m[4] / 1000;
		scrl((int)y, S_TO);
		break;
	}
}

/* a key: k is the scan code << 8 | the character, ks the shift keys */
static void keyb(k, ks)
int k, ks;
{
	int sc = k >> 8 & 0377, c = k & 0377, sh = ks & (K_LSHIFT | K_RSHIFT);
	Event *e;

	switch (sc) {
	case 0x48:		/* up */
		scrl(-1, sh ? S_PAGE : S_STEP);
		return;
	case 0x50:		/* down */
		scrl(1, sh ? S_PAGE : S_STEP);
		return;
	case 0x49:		/* Page Up, Page Down (a PC keyboard) */
		scrl(-1, S_PAGE);
		return;
	case 0x51:
		scrl(1, S_PAGE);
		return;
	case 0x47:		/* ClrHome */
		scrl(sh ? (int)BIG : 0, S_TO);
		return;
	case 0x61:		/* Undo */
		c = 033;
		break;
	}
	if (c == 0)
		return;
	if (c >= 0200 && (c = latin[c - 0200]) == 0)
		return;
	e = put(E_KEY);
	e->key = c;
}

/* a mouse event at x, y on the screen */
static void at(type, x, y, key)
int type, x, y, key;
{
	Event *e = put(type);

	e->x = x - wx;
	e->y = y - wy;
	e->key = key;
}

/* the buttons are b now, the mouse at x, y on the screen */
static void mouse(b, x, y)
int b, x, y;
{
	if (!held) {
		if (!(b & 3))
			return;
		held = 1;
		if (x < wx || x >= wx + wkw || y < wy || y >= wy + wkh ||
			wind_find(x, y) != wh)
			return;	/* not in the window: no E_DOWN, no E_UP */
		down = b & 1 ? 1 : 3;
		at(E_DOWN, x, y, down);
	} else if (!(b & 3)) {
		held = 0;
		if (down)
			at(E_UP, x, y, down);
		down = 0;
	}
}

int g_event(e, wait)
Event *e;
int wait;
{
	static short m[16], x, y, b, ks, k, n;
	Event *p;
	int fl, ev;

	for (;;) {
		if (iq < nq) {
			p = &q[iq++];
			e->type = p->type;
			e->x = p->x;
			e->y = p->y;
			e->w = p->w;
			e->h = p->h;
			e->key = p->key;
			return 1;
		}
		nq = iq = 0;
		fl = MU_MESAG | MU_KEYBD | MU_BUTTON;
		if (down)
			fl |= MU_M1;	/* the mouse leaves where it was */
		if (wait != 1)
			fl |= MU_TIMER;
		/* no button held: wait for one down (0x101: not state 0);
		 * held: wait for all up */
		ev = evnt_multi(fl, held ? 1 : 0x101, 3, 0,
			1, mx, my, 1, 1, 0, 0, 0, 0, 0,
			m, wait == 2 ? 100L : 0L, &x, &y, &b, &ks, &k, &n);
		if (ev & MU_MESAG)
			mesg(m);
		if (ev & MU_KEYBD)
			keyb(k & 0xffff, ks);
		if (ev & MU_BUTTON)
			mouse(b, x, y);
		else if ((ev & MU_M1) && down)
			at(E_MOVE, x, y, 0);
		mx = x;
		my = y;
		if (iq < nq)
			continue;
		if (wait != 1 && (ev & MU_TIMER))
			return 0;
	}
}
