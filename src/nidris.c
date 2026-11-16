/* nidris.c - Archaic's network on Idris: TCP through netd (the SLIP
 * daemon) on its AF_UNIX socket, /tmp/netd or $NETD.
 *
 * Ask "tcp HOST PORT\n". netd answers one line, "ok ..." or "err why",
 * then the data comes. In what netd sends, ESC (0377) comes twice, and
 * ESC 0 is the end; the program closes the socket, never netd. What the
 * program writes goes out as it is.
 *
 * Idris sockets ignore FIONBIO: a read waits for data. So this reads
 * only what FIONREAD says is there. select() takes one long as the mask
 * and whole seconds.
 */
#include "archaic.h"
#include <socket.h>

#ifdef NOPROTO
char *getenv();
#else
#include <stdlib.h>
#endif

#define NESC 0377
#define BIT(n) ((unsigned long)1 << (n))

static unsigned char nbuf[1024];	/* from netd, ESCs not undone yet */
static int npos, nlen, nesc, neof;
static char req[200];

/* bytes waiting on fd, -1 if its other end has gone */
static int avail(fd)
int fd;
{
	int n = 0;

	if (ioctl(fd, FIONREAD, &n) < 0)
		return -1;
	return n;
}

/* wait at most a second for fd h, or for the display */
static void waitfor(h)
int h;
{
	static struct timeval tv;
	unsigned long rd;
	int d = g_fd(), n = h;

	rd = BIT(h);
	if (d >= 0 && d < 32) {
		rd |= BIT(d);
		if (d > n)
			n = d;
	}
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	select(n + 1, &rd, (unsigned long *)0, (unsigned long *)0, &tv);
}

int n_read(h, buf, n, wait)
int h, n, wait;
char *buf;
{
	int k, c;

	if (npos >= nlen && !neof) {
		k = avail(h);
		if (k == 0 && wait) {
			waitfor(h);
			k = avail(h);
		}
		if (k < 0)
			return -1;
		if (k == 0)
			return -2;
		if (k > (int)sizeof nbuf)
			k = sizeof nbuf;
		npos = 0;
		nlen = read(h, (char *)nbuf, k);
		if (nlen <= 0) {
			nlen = 0;
			neof = 1;
		}
	}
	for (k = 0; k < n && npos < nlen;) {
		c = nbuf[npos++];
		if (nesc) {
			nesc = 0;
			if (c != NESC) {	/* ESC 0: the end */
				neof = 1;
				break;
			}
		} else if (c == NESC) {
			nesc = 1;
			continue;
		}
		buf[k++] = c;
	}
	if (k > 0)
		return k;
	return neof ? 0 : -2;
}

int n_open(host, port, err)
char *host, *err;
int port;
{
	static struct sockaddr_un sa;
	char *path = getenv("NETD"), *d, c;
	int s, k, i, n;
	long t0;

	npos = nlen = nesc = neof = 0;
	if (!path || !*path)
		path = "/tmp/netd";
	d = (char *)&sa;
	for (i = 0; i < (int)sizeof sa; ++i)
		d[i] = 0;
	sa.sun_family = AF_UNIX;
	scopy(sa.sun_path, path, (int)sizeof sa.sun_path);
	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0 || connect(s, &sa, sizeof sa) < 0) {
		sprintf(err, "netd is not running (%.60s)", path);
		if (s >= 0)
			close(s);
		return -1;
	}
	sprintf(req, "tcp %.120s %d\n", host, port);
	for (n = 0; req[n]; ++n)
		;
	write(s, req, n);
	/* the answer line, a byte at a time: what follows stays in nbuf */
	t0 = now();
	k = 0;
	for (;;) {
		n = n_read(s, &c, 1, 1);
		if (n == 1) {
			if (c == '\n')
				break;
			if (k < 150)
				err[k++] = c;
			continue;
		}
		if (n != -2)
			break;
		if (ui_poll()) {
			k = 0;
			scopy(err, "stopped", 160);
			close(s);
			return -1;
		}
		if (now() - t0 > 90)
			break;
	}
	err[k] = 0;
	if (k >= 2 && err[0] == 'o' && err[1] == 'k') {
		err[0] = 0;
		return s;
	}
	close(s);
	if (strnieq(err, "err ", 4)) {
		for (i = 0; err[i + 4]; ++i)
			err[i] = err[i + 4];
		err[i] = 0;
	} else if (k == 0)
		sprintf(err, "no answer from netd");
	return -1;
}

int n_write(h, buf, n)
int h, n;
char *buf;
{
	int k;

	while (n > 0) {
		k = write(h, buf, n);
		if (k <= 0)
			return -1;
		buf += k;
		n -= k;
	}
	return 0;
}

void n_close(h)
int h;
{
	close(h);
	npos = nlen = nesc = neof = 0;
}
