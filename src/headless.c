/* headless.c - Archaic's display with no window, for tests and
 * automation. It draws into fb.c's screen in memory, the same pixels as
 * sdl.c and cocoa.m, and reads commands from standard input, one per
 * line. It reads the next command only when Archaic has nothing else to
 * do, so a command that loads a page ends before the next one starts.
 * At the end of the input, Archaic quits.
 *
 *	click X Y [B]	mouse button B (1 left, 2 middle, 3 right) at X, Y
 *	key K		a character, or return, tab, esc, backspace,
 *			delete, space, pgup, pgdn, up, down, ctrl-X
 *	type TEXT	the rest of the line, as keys
 *	scroll up|down	as Page Up and Page Down
 *	size W H	the window is now W x H
 *	shot FILE	write the screen to FILE: PNG, or PBM for *.pbm
 *	sleep MS	wait MS milliseconds
 *	quit
 * Empty lines and lines that start with # do nothing. Standard output
 * gets "title TEXT" for each page, and "shot FILE WxH" for each shot.
 *
 *	printf 'click 20 12\ntype example.com\nkey return\nshot a.png\n' |
 *		archaic-headless -v 127.0.0.1:8080
 */
#include "fb.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

static char line[1024];
static char *text;		/* type: characters not given yet */

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
	e->key = k & 0377;
	return 1;
}

int g_open(geom, w, h)
char *geom;
int *w, *h;
{
	int gw = 800, gh = 600, x, y;

	fb_geom(geom, &gw, &gh, &x, &y);
	if (gw < 200)
		gw = 200;
	if (gh < 100)
		gh = 100;
	if (fb_size(gw, gh) < 0)
		return -1;
	*w = gw;
	*h = gh;
	return 0;
}

void g_close()
{
	fflush(stdout);
}

void g_title(s)
char *s;
{
	printf("title %s\n", s);
	fflush(stdout);
}

void g_flush()
{
	fb_done();
}

int g_fd()
{
	return -1;
}

/* a key by its name */
static int keyname(e, s)
Event *e;
char *s;
{
	static char *names[] = { "return", "\r", "enter", "\r", "tab", "\t",
		"esc", "\033", "backspace", "\b", "delete", "\177", "space", " ",
		0 };
	int i;

	if (s[0] && !s[1])
		return key(e, s[0]);
	if (strncmp(s, "ctrl-", 5) == 0 && s[5] && !s[6])
		return key(e, s[5] & 037);
	if (!strcmp(s, "pgup") || !strcmp(s, "up"))
		return roll(e, 0);
	if (!strcmp(s, "pgdn") || !strcmp(s, "down"))
		return roll(e, 1);
	for (i = 0; names[i]; i += 2)
		if (!strcmp(s, names[i]))
			return key(e, names[i + 1][0]);
	fprintf(stderr, "archaic: no key %s\n", s);
	return 0;
}

/* one command: 1 if it gives an event */
static int command(e, s)
Event *e;
char *s;
{
	struct timeval tv;
	char *a;
	int n, x, y, b, w, h;

	while (*s == ' ' || *s == '\t')
		++s;
	if (!*s || *s == '#')
		return 0;
	for (a = s; *a && *a != ' ' && *a != '\t'; ++a)
		;
	n = a - s;
	while (*a == ' ' || *a == '\t')
		++a;
	if (n == 5 && !strncmp(s, "click", 5)) {
		b = 1;
		if (sscanf(a, "%d %d %d", &x, &y, &b) < 2) {
			fprintf(stderr, "archaic: click X Y [B]\n");
			return 0;
		}
		e->type = E_DOWN;
		e->x = x;
		e->y = y;
		e->key = b;
		return 1;
	}
	if (n == 3 && !strncmp(s, "key", 3))
		return keyname(e, a);
	if (n == 4 && !strncmp(s, "type", 4)) {
		text = a;
		return 0;
	}
	if (n == 6 && !strncmp(s, "scroll", 6))
		return roll(e, !strcmp(a, "down"));
	if (n == 4 && !strncmp(s, "size", 4)) {
		if (sscanf(a, "%d %d", &w, &h) != 2 || w < 200 || h < 100 ||
			fb_size(w, h) < 0) {
			fprintf(stderr, "archaic: size W H\n");
			return 0;
		}
		e->type = E_SIZE;
		e->w = w;
		e->h = h;
		return 1;
	}
	if (n == 4 && !strncmp(s, "shot", 4)) {
		if (fb_shot(a) < 0)
			fprintf(stderr, "archaic: can't write %s\n", a);
		else
			printf("shot %s %dx%d\n", a, fbw, fbh);
		fflush(stdout);
		return 0;
	}
	if (n == 5 && !strncmp(s, "sleep", 5)) {
		n = atoi(a);
		tv.tv_sec = n / 1000;
		tv.tv_usec = n % 1000 * 1000L;
		select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &tv);
		return 0;
	}
	if (n == 4 && !strncmp(s, "quit", 4)) {
		e->type = E_QUIT;
		return 1;
	}
	fprintf(stderr, "archaic: no command %s\n", s);
	return 0;
}

/* commands only when Archaic waits for the user (wait 1): never while a
 * page loads */
int g_event(e, wait)
Event *e;
int wait;
{
	int n;

	if (!wait)
		return 0;
	for (;;) {
		if (text && *text) {
			if (*text == '\n') {
				text = 0;
				continue;
			}
			return key(e, *text++);
		}
		text = 0;
		if (!fgets(line, (int)sizeof line, stdin)) {
			e->type = E_QUIT;
			return 1;
		}
		n = strlen(line);
		if (n > 0 && line[n - 1] == '\n')
			line[--n] = 0;
		if (command(e, line))
			return 1;
	}
}
