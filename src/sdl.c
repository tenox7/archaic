/* sdl.c - Archaic's display for SDL 2 (macOS, Linux, Windows ...): a
 * window that shows fb.c's screen in memory. On a Retina screen each
 * pixel is 2x2, with no smoothing.
 *
 * Keys: SDL's text events give the characters; Return, Backspace, Tab,
 * Esc, Delete and Ctrl+letter come as key events. Page Up, Page Down,
 * the up and down arrows and the mouse wheel scroll: they come as a
 * right click at the top or the bottom of the page, as in x11.c and
 * unixpc.c. Ctrl+click is the right button. Cmd-Q and the close button
 * quit. SDL has no file to wait on: g_fd() is -1.
 */
#include "fb.h"
#include <SDL.h>

static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Texture *tex;
static float wheel;		/* wheel steps not used yet */
static unsigned char text[64];	/* typed characters not given yet */
static int ntext, itext;

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

/* scroll: a right click at the top (up) or the bottom (down) */
static int roll(e, down)
Event *e;
int down;
{
	e->type = E_DOWN;
	e->key = 3;
	e->x = 0;
	e->y = down ? fbh - 30 : 0;
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
	SDL_Event ev;
	int k, w, h;

	for (;;) {
		if (itext < ntext)
			return key(e, text[itext++]);
		if (wait ? !SDL_WaitEvent(&ev) : !SDL_PollEvent(&ev))
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
			case SDLK_UP:
				return roll(e, 0);
			case SDLK_PAGEDOWN:
			case SDLK_DOWN:
				return roll(e, 1);
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (ev.button.button > SDL_BUTTON_RIGHT)
				break;
			e->type = E_DOWN;
			e->x = ev.button.x;
			e->y = ev.button.y;
			e->key = ev.button.button == SDL_BUTTON_LEFT ? 1 :
				ev.button.button == SDL_BUTTON_MIDDLE ? 2 : 3;
			return 1;
		case SDL_MOUSEWHEEL:
			/* 3 steps of the wheel scroll a page */
			wheel += ev.wheel.preciseY;
			if (wheel >= 3 || wheel <= -3) {
				k = wheel < 0;
				wheel = 0;
				return roll(e, k);
			}
			break;
		}
	}
}
