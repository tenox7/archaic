/* sdl.c - Archaic's display for SDL 2 (macOS, Linux, Windows ...): a
 * window that shows fb.c's screen in memory. On a Retina screen each
 * pixel is 2x2, with no smoothing. The colors: as many as the screen
 * shows (its display mode).
 *
 * Keys: SDL's text events give the characters; Return, Backspace, Tab,
 * Esc, Delete and Ctrl+letter come as key events. Page Up and Page Down
 * scroll a window, the up and down arrows and the wheel's steps a
 * quarter of it (E_SCROLL); a trackpad scrolls by pixels. Ctrl+click is
 * the right button. Cmd-Q and the close button quit. SDL has no file to
 * wait on: g_fd() is -1. SDL has no scroll bars: Archaic draws its own
 * (g_bar).
 */
#include "fb.h"
#include <SDL.h>

static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Texture *tex;
static float wheel;		/* trackpad pixels not used yet */
static unsigned char text[64];	/* typed characters not given yet */
static int ntext, itext;

int g_bar = 1;

void g_sbar(pos, max, vis)
long pos, max, vis;
{
}

/* SDL's mouse button: 1 left, 2 middle, 3 right; 0 others */
static int button(b)
int b;
{
	return b == SDL_BUTTON_LEFT ? 1 : b == SDL_BUTTON_MIDDLE ? 2 :
		b == SDL_BUTTON_RIGHT ? 3 : 0;
}

static int mktex()
{
	if (tex)
		SDL_DestroyTexture(tex);
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB888,
		SDL_TEXTUREACCESS_STREAMING, fbw, fbh);
	return tex ? 0 : -1;
}

int g_open(geom, w, h)
char *geom;
int *w, *h;
{
	SDL_DisplayMode dm;
	int gw = 800, gh = 600, x = 0, y = 0, f;
	int px = SDL_WINDOWPOS_CENTERED, py = SDL_WINDOWPOS_CENTERED;

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
	SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return -1;
	f = fb_geom(geom, &gw, &gh, &x, &y);
	if (gw < 200)
		gw = 200;
	if (gh < 100)
		gh = 100;
	if (f & 2) {
		px = x;
		py = y;
		if ((f & 12) && SDL_GetDesktopDisplayMode(0, &dm) == 0) {
			if (f & 4)
				px = dm.w - gw - x;
			if (f & 8)
				py = dm.h - gh - y;
		}
	}
	win = SDL_CreateWindow("Archaic", px, py, gw, gh,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	if (!win)
		return -1;
	ren = SDL_CreateRenderer(win, -1, 0);
	if (!ren)
		return -1;
	SDL_GetWindowSize(win, &gw, &gh);
	if (fb_size(gw, gh) < 0 || mktex() < 0)
		return -1;
	SDL_StartTextInput();
	*w = gw;
	*h = gh;
	return 0;
}

/* the screen's colors: 2 to the bits of its pixels */
long g_colors()
{
	SDL_DisplayMode dm;
	int i = win ? SDL_GetWindowDisplayIndex(win) : 0, b;

	if (SDL_GetCurrentDisplayMode(i < 0 ? 0 : i, &dm) < 0)
		return 1L << 24;
	b = SDL_BITSPERPIXEL(dm.format);
	return b < 1 ? 2L : b > 24 ? 1L << 24 : 1L << b;
}

void g_close()
{
	if (tex)
		SDL_DestroyTexture(tex);
	if (ren)
		SDL_DestroyRenderer(ren);
	if (win)
		SDL_DestroyWindow(win);
	tex = 0;
	ren = 0;
	win = 0;
	SDL_Quit();
}

/* the title is ISO 8859-1, SDL wants UTF-8 */
void g_title(s)
char *s;
{
	static char t[400];
	int c, n = 0;

	for (; *s && n < (int)sizeof t - 3; ++s) {
		c = *s & 0377;
		if (c < 0200)
			t[n++] = c;
		else {
			t[n++] = 0300 | c >> 6;
			t[n++] = 0200 | (c & 077);
		}
	}
	t[n] = 0;
	SDL_SetWindowTitle(win, t);
}

void g_flush()
{
	SDL_Rect r;

	if (fbx0 < fbx1) {
		r.x = fbx0;
		r.y = fby0;
		r.w = fbx1 - fbx0;
		r.h = fby1 - fby0;
		SDL_UpdateTexture(tex, &r, fb + (long)fby0 * fbw + fbx0,
			fbw * (int)sizeof *fb);
		fb_done();
	}
	SDL_RenderClear(ren);
	SDL_RenderCopy(ren, tex, (SDL_Rect *)0, (SDL_Rect *)0);
	SDL_RenderPresent(ren);
}

int g_fd()
{
	return -1;
}

/* a clock in milliseconds (for the spinner) */
long g_ms()
{
	return (long)(SDL_GetTicks() & 0x3fffffffL);
}

/* scroll y units (S_PIXEL, S_STEP, S_PAGE), down if y > 0 */
static int scrl(e, y, unit)
Event *e;
int y, unit;
{
	e->type = E_SCROLL;
	e->x = 0;
	e->y = y;
	e->key = unit;
	return 1;
}

static int key(e, k)
Event *e;
int k;
{
	e->type = E_KEY;
	e->key = k;
	return 1;
}

/* SDL's text is UTF-8: keep the characters up to 255 (ISO 8859-1) */
static void typed(s)
char *s;
{
	unsigned char *p = (unsigned char *)s;
	int c;

	ntext = itext = 0;
	while (*p && ntext < (int)sizeof text) {
		c = *p++;
		if (c >= 0300 && c < 0340 && (*p & 0300) == 0200) {
			c = (c & 037) << 6 | (*p++ & 077);
		} else if (c >= 0200) {
			while ((*p & 0300) == 0200)
				++p;
			continue;
		}
		if (c >= ' ' && c != 0177 && c < 0400)
			text[ntext++] = c;
	}
}

int g_event(e, wait)
Event *e;
int wait;
{
	SDL_Event ev, nx;
	int k, w, h;

	for (;;) {
		if (itext < ntext)
			return key(e, text[itext++]);
		if (wait == 2 ? !SDL_WaitEventTimeout(&ev, 100) :
			wait ? !SDL_WaitEvent(&ev) : !SDL_PollEvent(&ev))
			return 0;
		switch (ev.type) {
		case SDL_QUIT:
			e->type = E_QUIT;
			return 1;
		case SDL_WINDOWEVENT:
			if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
				SDL_GetWindowSize(win, &w, &h);
				if (w == fbw && h == fbh)
					break;
				if (fb_size(w, h) < 0 || mktex() < 0) {
					e->type = E_QUIT;
					return 1;
				}
				e->type = E_SIZE;
				e->w = w;
				e->h = h;
				return 1;
			}
			if (ev.window.event == SDL_WINDOWEVENT_EXPOSED)
				g_flush();
			break;
		case SDL_TEXTINPUT:
			typed(ev.text.text);
			break;
		case SDL_KEYDOWN:
			k = ev.key.keysym.sym;
			if (ev.key.keysym.mod & KMOD_GUI) {
				if (k == SDLK_q || k == SDLK_w) {
					e->type = E_QUIT;
					return 1;
				}
				break;
			}
			if ((ev.key.keysym.mod & KMOD_CTRL) && k >= SDLK_a && k <= SDLK_z)
				return key(e, k - SDLK_a + 1);
			switch (k) {
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				return key(e, '\r');
			case SDLK_BACKSPACE:
				return key(e, '\b');
			case SDLK_DELETE:
				return key(e, 0177);
			case SDLK_TAB:
				return key(e, '\t');
			case SDLK_ESCAPE:
				return key(e, 033);
			case SDLK_PAGEUP:
				return scrl(e, -1, S_PAGE);
			case SDLK_PAGEDOWN:
				return scrl(e, 1, S_PAGE);
			case SDLK_UP:
				return scrl(e, -1, S_STEP);
			case SDLK_DOWN:
				return scrl(e, 1, S_STEP);
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			if (!button(ev.button.button))
				break;
			e->type = ev.type == SDL_MOUSEBUTTONDOWN ? E_DOWN : E_UP;
			e->x = ev.button.x;
			e->y = ev.button.y;
			e->key = button(ev.button.button);
			return 1;
		case SDL_MOUSEMOTION:
			if (!ev.motion.state)	/* no button down */
				break;
			/* only the last place counts */
			while (SDL_PeepEvents(&nx, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT,
				SDL_LASTEVENT) == 1 && nx.type == SDL_MOUSEMOTION &&
				nx.motion.state)
				SDL_PollEvent(&ev);
			e->type = E_MOVE;
			e->x = ev.motion.x;
			e->y = ev.motion.y;
			e->key = 0;
			return 1;
		case SDL_MOUSEWHEEL:
			/* A wheel gives whole steps: a quarter of the window
			 * each. A trackpad gives parts of steps: 10 pixels a step. */
			if (ev.wheel.preciseY == (float)ev.wheel.y) {
				if (ev.wheel.y)
					return scrl(e, -ev.wheel.y, S_STEP);
				break;
			}
			wheel += ev.wheel.preciseY * 10;
			k = (int)wheel;
			wheel -= k;
			if (k)
				return scrl(e, -k, S_PIXEL);
			break;
		}
	}
}
