/* http.c - URLs and a small HTTP/1.0 client for Archaic.
 *
 * http_open() connects (through the port's n_ functions), sends the
 * request, reads the headers and follows redirects. Then http_getc()
 * gives the body one byte at a time. While the network has nothing,
 * http_getc() lets the user interface run (ui_poll), so the window can
 * be redrawn and a load can be stopped. ui_note() tells the user
 * interface what http_open() is doing: connecting, waiting for the
 * answer, the body.
 */
#include "archaic.h"
#ifdef unixpc
#include <sys/types.h>		/* time_t: the 3B1's time.h does not have it */
#endif
#ifdef sun
#include <sys/types.h>		/* nor SunOS 3's */
#endif
#include <time.h>

char http_url[MAXURL];
char http_type[64];
char http_err[160];
long http_len;
long http_got;
int http_status;

static int conn = -1;
static int beof;		/* the network has no more */
unsigned char *http_p, *http_e;	/* body bytes at hand: http_p up to http_e */
static unsigned char *wbase;	/* where http_p was at the last http_sync() */
/* what came from the network and is not used yet: a ring */
#define RING 16384		/* a power of 2 */
static char ring[RING];
static long rin, rout;		/* bytes put in, taken out */
static char req[2 * MAXURL + 300];
static char line[MAXURL + 40];
static char loc[MAXURL];
char http_host[128];		/* the server of the last http_open() */
/* what Archaic takes, for the server: pages, GIFs, and WRP's black and
 * white picture (ABM: see pic.c). With -f gif, main.c leaves ABM out. */
char *http_accept = "text/html, image/x-abm, image/gif";
int http_port;
int http_noconn;		/* 1: the last http_open() could not connect */
static char path[MAXURL];

long now()
{
	return (long)time((time_t *)0);
}

int lc(c)
int c;
{
	return c >= 'A' && c <= 'Z' ? c + 'a' - 'A' : c;
}

int strieq(a, b)
char *a, *b;
{
	while (*a && lc(*a) == lc(*b)) {
		++a;
		++b;
	}
	return *a == 0 && *b == 0;
}

/* the first n characters of a and b are the same, ignoring case */
int strnieq(a, b, n)
char *a, *b;
int n;
{
	for (; n > 0; --n, ++a, ++b) {
		if (lc(*a) != lc(*b))
			return 0;
		if (*a == 0)
			return 1;
	}
	return 1;
}

/* copy s to d, at most max - 1 characters */
void scopy(d, s, max)
char *d, *s;
int max;
{
	while (*s && max > 1) {
		*d++ = *s++;
		--max;
	}
	*d = 0;
}

static int slen(s)
char *s;
{
	int n;

	for (n = 0; s[n]; ++n)
		;
	return n;
}

/* append s to d (d has MAXURL bytes) */
static void sadd(d, s)
char *d, *s;
{
	int n = slen(d);

	scopy(d + n, s, MAXURL - n);
}

/* "http://host:port/path" to host, port and path */
static int url_split(url)
char *url;
{
	char *p = url;
	int i;

	if (!strnieq(p, "http://", 7))
		return -1;
	p += 7;
	for (i = 0; *p && *p != '/' && *p != ':' && *p != '?' && *p != '#'; ++p)
		if (i < (int)sizeof http_host - 1)
			http_host[i++] = *p;
	http_host[i] = 0;
	http_port = 80;
	if (*p == ':') {
		http_port = 0;
		for (++p; *p >= '0' && *p <= '9'; ++p)
			http_port = http_port * 10 + *p - '0';
	}
	if (*p == '/')
		scopy(path, p, MAXURL);
	else {
		path[0] = '/';
		scopy(path + 1, p, MAXURL - 1);
	}
	for (i = 0; path[i]; ++i)
		if (path[i] == '#')
			path[i] = 0;
	return http_host[0] && http_port > 0 ? 0 : -1;
}

/* ref (maybe relative) seen on page base, as a full URL in out */
void url_join(base, ref, out)
char *base, *ref, *out;
{
	char *p;
	int n, i;

	while (*ref == ' ' || *ref == '\t' || *ref == '\n' || *ref == '\r')
		++ref;
	/* a scheme ("http:", "mailto:"...): already absolute */
	for (p = ref; (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z'); ++p)
		;
	if (p > ref && *p == ':') {
		scopy(out, ref, MAXURL);
		return;
	}
	if (ref[0] == '/' && ref[1] == '/') {
		scopy(out, "http:", MAXURL);
		sadd(out, ref);
		return;
	}
	/* p: the end of "http://host:port" in base */
	p = base;
	if (strnieq(p, "http://", 7))
		p += 7;
	while (*p && *p != '/' && *p != '?' && *p != '#')
		++p;
	n = p - base;
	if (n > MAXURL - 2)
		n = MAXURL - 2;
	for (i = 0; i < n; ++i)
		out[i] = base[i];
	out[n] = 0;
	if (*ref == '/') {
		sadd(out, ref);
		return;
	}
	/* the path of base, without its query and fragment */
	for (i = n; base[i] && base[i] != '?' && base[i] != '#' && i < MAXURL - 2; ++i)
		out[i] = base[i];
	out[i] = 0;
	if (out[n] == 0) {
		out[n] = '/';
		out[n + 1] = 0;
	}
	if (*ref == '?' || *ref == '#' || *ref == 0) {
		if (*ref == '?')
			sadd(out, ref);
		return;
	}
	/* a relative path: replace the last part of the base path */
	for (i = slen(out); i > n && out[i - 1] != '/'; --i)
		;
	out[i] = 0;
	sadd(out, ref);
}

/* append s to out (MAXURL bytes) as form data: a b&c to a+b%26c */
void url_enc(out, s)
char *out, *s;
{
	static char hex[] = "0123456789ABCDEF";
	int n = slen(out), c;

	for (; *s && n < MAXURL - 4; ++s) {
		c = *s & 0377;
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') || c == '-' || c == '_' ||
			c == '.' || c == '*')
			out[n++] = c;
		else if (c == ' ')
			out[n++] = '+';
		else {
			out[n++] = '%';
			out[n++] = hex[c >> 4];
			out[n++] = hex[c & 15];
		}
	}
	out[n] = 0;
}

/* put what the network has into the ring: 1 = some, 0 = nothing yet
 * (or no room), -1 = the end */
static int take(wait)
int wait;
{
	int n, room, at;

	if (beof)
		return -1;
	room = RING - (int)(rin - rout);
	at = (int)(rin & (RING - 1));
	if (room > RING - at)
		room = RING - at;
	if (room <= 0)
		return 0;
	n = n_read(conn, ring + at, room, wait);
	if (n > 0) {
		rin += n;
		return 1;
	}
	if (n == -2)
		return 0;
	beof = 1;		/* 0: the end, -1: an error */
	return -1;
}

/* Take what the network has now. Netd (Idris) wants its data read soon,
 * and an 8 MHz 68000 decodes a picture slower than SLIP brings it. */
void http_pump()
{
	if (conn >= 0)
		while (take(0) > 0)
			;
}

/* wait for more: 0 ok, -1 at the end (or stopped) */
static int fill()
{
	long t0 = now();
	int r;

	for (;;) {
		if (stop) {
			sprintf(http_err, "stopped");
			beof = 1;
			return -1;
		}
		r = take(1);
		if (r > 0) {
			ui_poll();	/* data keeps coming: the user still counts */
			return 0;
		}
		if (r < 0)
			return -1;
		if (ui_poll())
			continue;	/* stop is set: the top of the loop ends it */
		if (now() - t0 > 120) {
			sprintf(http_err, "no answer for 2 minutes");
			beof = 1;
			return -1;
		}
	}
}

static int rawc()
{
	int c;

	if (rout >= rin && fill() < 0)
		return -1;
	c = ring[(int)(rout & (RING - 1))] & 0377;
	++rout;
	return c;
}

/* one header line, without CR LF: its length, -1 at the end */
static int hline()
{
	int c, n = 0;

	while ((c = rawc()) >= 0 && c != '\n')
		if (c != '\r' && n < (int)sizeof line - 1)
			line[n++] = c;
	line[n] = 0;
	return c < 0 && n == 0 ? -1 : n;
}

/* the value of header line "Name: value" */
static char *hval(n)
int n;
{
	char *p = line + n;

	while (*p == ' ' || *p == '\t')
		++p;
	return p;
}

int http_open(url, post)
char *url, *post;
{
	int redir, n, i;
	char *p;

	http_close();
	scopy(http_url, url, MAXURL);
	for (redir = 0;; ++redir) {
		http_err[0] = 0;
		http_noconn = 0;
		http_type[0] = 0;
		http_len = -1;
		http_got = 0;
		http_status = 0;
		loc[0] = 0;
		if (url_split(http_url) < 0) {
			sprintf(http_err, "can't use this URL (only http://)");
			return -1;
		}
		ui_note(H_CONNECT);
		conn = n_open(http_host, http_port, http_err);
		if (conn < 0) {
			http_noconn = 1;
			ui_note(0);
			return -1;
		}
		rin = rout = 0;
		beof = 0;
		http_p = http_e = wbase = (unsigned char *)ring;
		if (http_port == 80)
			sprintf(line, "%s", http_host);
		else
			sprintf(line, "%s:%d", http_host, http_port);
		if (post)
			sprintf(req, "POST %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: Archaic/%s\r\nAccept: %s\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n%s",
				path, line, VERSION, http_accept, slen(post), post);
		else
			sprintf(req, "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: Archaic/%s\r\nAccept: %s\r\n\r\n",
				path, line, VERSION, http_accept);
		if (n_write(conn, req, slen(req)) < 0) {
			sprintf(http_err, "can't send to %s", http_host);
			http_close();
			return -1;
		}
		ui_note(H_WAIT);
		/* HTTP/1.0 200 OK */
		if (hline() < 0) {
			if (!http_err[0])
				sprintf(http_err, "no answer from %s", http_host);
			http_close();
			return -1;
		}
		for (p = line; *p && *p != ' '; ++p)
			;
		while (*p == ' ')
			++p;
		while (*p >= '0' && *p <= '9')
			http_status = http_status * 10 + *p++ - '0';
		if (!strnieq(line, "HTTP/", 5) || http_status == 0) {
			sprintf(http_err, "not HTTP: %.60s", line);
			http_close();
			return -1;
		}
		while ((n = hline()) > 0) {
			if (strnieq(line, "content-length:", 15)) {
				http_len = 0;
				for (p = hval(15); *p >= '0' && *p <= '9'; ++p)
					http_len = http_len * 10 + *p - '0';
			} else if (strnieq(line, "content-type:", 13)) {
				p = hval(13);
				for (i = 0; p[i] && p[i] != ';' && p[i] != ' ' &&
					i < (int)sizeof http_type - 1; ++i)
					http_type[i] = lc(p[i]);
				http_type[i] = 0;
			} else if (strnieq(line, "location:", 9))
				url_join(http_url, hval(9), loc);
		}
		if (n < 0 && stop) {
			http_close();
			return -1;
		}
		if ((http_status == 301 || http_status == 302 ||
			http_status == 303 || http_status == 307 ||
			http_status == 308) && loc[0] && redir < 5) {
			http_close();
			scopy(http_url, loc, MAXURL);
			if (http_status != 307 && http_status != 308)
				post = 0;
			continue;
		}
		ui_note(H_BODY);
		return 0;
	}
}

/* The body comes through a window on the buffer: the bytes from http_p
 * up to http_e are at hand, and HGETC() (archaic.h) takes the next one
 * with no call. http_sync() counts the bytes taken so far (http_got) and
 * gives their room back to the buffer; call it before img_rows(), which
 * reads the network. */
void http_sync()
{
	int n = (int)(http_p - wbase);

	rout += n;
	http_got += n;
	wbase = http_p;
}

/* the window is used up: the next byte, -1 at the end of the body. The
 * window is then all the bytes that lie together in the buffer. */
int http_more()
{
	long n;
	int at;

	http_sync();
	http_p = http_e = wbase = (unsigned char *)ring;
	if (conn < 0 || (http_len >= 0 && http_got >= http_len))
		return -1;
	if (rout >= rin && fill() < 0)
		return -1;
	at = (int)(rout & (RING - 1));
	n = rin - rout;
	if (n > RING - at)
		n = RING - at;
	if (http_len >= 0 && n > http_len - http_got)
		n = http_len - http_got;
	wbase = http_p = (unsigned char *)ring + at;
	http_e = http_p + (int)n;
	return *http_p++;
}

/* the next byte of the body, -1 at its end */
int http_getc()
{
	int c = HGETC();

	http_sync();
	return c;
}

void http_close()
{
	if (conn >= 0) {
		n_close(conn);
		ui_note(0);
	}
	conn = -1;
	http_p = http_e = wbase = (unsigned char *)ring;
}
