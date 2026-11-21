/* nsock.c - Archaic's network for BSD sockets (Unix, macOS, Linux,
 * SunOS 4). An address such as 192.168.1.10 goes through inet_addr()
 * first: SunOS 4's gethostbyname() knows only names. */
#include "archaic.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#ifndef NOUNISTD
#include <unistd.h>
#endif
#ifdef NOPROTO
unsigned long inet_addr();
#else
#include <arpa/inet.h>
#endif

int n_open(host, port, err)
char *host, *err;
int port;
{
	static struct sockaddr_in sa;
	struct hostent *he;
	char *p, *d;
	int s, i;

	d = (char *)&sa;
	for (i = 0; i < (int)sizeof sa; ++i)
		d[i] = 0;
	sa.sin_family = AF_INET;
	sa.sin_port = htons((unsigned short)port);
	sa.sin_addr.s_addr = inet_addr(host);
	if (sa.sin_addr.s_addr == 0xffffffffL) {
		he = gethostbyname(host);
		if (!he) {
			sprintf(err, "%.60s: unknown host", host);
			return -1;
		}
		p = (char *)he->h_addr;
		d = (char *)&sa.sin_addr;
		for (i = 0; i < he->h_length && i < (int)sizeof sa.sin_addr; ++i)
			d[i] = p[i];
	}
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		sprintf(err, "no socket");
		return -1;
	}
	if (connect(s, (struct sockaddr *)&sa, sizeof sa) < 0) {
		sprintf(err, "can't connect to %.60s port %d", host, port);
		close(s);
		return -1;
	}
	return s;
}

/* wait at most 1/20 s (or not at all), also for the display: so a
 * display with no file to wait on (sdl.c, cocoa.m) gets its turn, and
 * the spinner turns (main.c). */
int n_read(h, buf, n, wait)
int h, n, wait;
char *buf;
{
	fd_set r;
	struct timeval tv;
	int d = g_fd(), m;

	FD_ZERO(&r);
	FD_SET(h, &r);
	if (d >= 0 && wait)
		FD_SET(d, &r);
	tv.tv_sec = 0;
	tv.tv_usec = wait ? 50000L : 0;
	m = select((h > d ? h : d) + 1, &r, (fd_set *)0, (fd_set *)0, &tv);
	if (m <= 0 || !FD_ISSET(h, &r))
		return -2;
	m = read(h, buf, n);
	return m < 0 ? -1 : m;
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
}
