/* nsock.c - Archaic's network for BSD sockets (Unix, macOS, Linux,
 * SunOS, HP-UX). An address such as 10.0.2.2 goes through inet_addr()
 * first: SunOS 4's gethostbyname() knows only names. The connect does
 * not hold up the window (FIONBIO): it is drawn while Archaic waits,
 * Esc stops the wait, and after CWAIT seconds Archaic gives up.
 * A/UX (AUX, with mac.c): a wait for the network is a wait in the
 * Toolbox's WaitNextEvent (mac_sel), so the Finder and the other Mac
 * programs run meanwhile; select() then only looks, with no wait. */
#include "archaic.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#ifdef _AIX
#include <sys/select.h>	/* AIX 3: fd_set is here */
#endif
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#ifndef NOUNISTD
#include <unistd.h>
#endif
#ifdef NOPROTO
unsigned long inet_addr();
#else
#include <arpa/inet.h>
#endif
#ifndef errno
extern int errno;
#endif

#ifndef FD_SET		/* 4.2BSD (SunOS 3): an fd_set of one int, no macros */
#define FD_ZERO(p) ((p)->fds_bits[0] = 0)
#define FD_SET(n, p) ((p)->fds_bits[0] |= 1 << (n))
#define FD_ISSET(n, p) ((p)->fds_bits[0] & 1 << (n))
#endif

#define CWAIT 20		/* seconds to wait for a connect */

/* the socket's calls wait for the network (on 0), or not (on 1) */
static void nbio(s, on)
int s, on;
{
#ifdef FIONBIO
	ioctl(s, FIONBIO, (char *)&on);
#endif
}

/* why the connect failed: the error e in words */
static void why(err, e)
char *err;
int e;
{
	char *s = 0;

#ifdef ECONNREFUSED
	if (e == ECONNREFUSED)
		s = "connection refused";
#endif
#ifdef ETIMEDOUT
	if (e == ETIMEDOUT)
		s = "no answer";
#endif
#ifdef ENETUNREACH
	if (e == ENETUNREACH)
		s = "network unreachable";
#endif
#ifdef EHOSTUNREACH
	if (e == EHOSTUNREACH)
		s = "host unreachable";
#endif
	if (s)
		scopy(err, s, 60);
	else
		sprintf(err, "error %d", e);
}

/* the connect goes on: wait for it, and let the user act meanwhile
 * (ui_poll: the window is drawn, Esc stops). 0 connected, -1 stopped
 * or no answer (err says), else the error */
static int cwait(s, err)
int s;
char *err;
{
	fd_set r, w;
	struct timeval tv;
	long t0 = now();
	int d, e;
	unsigned int n;

	for (;;) {
		d = g_fd();
		FD_ZERO(&r);
		FD_ZERO(&w);
		FD_SET(s, &w);
		if (d >= 0)
			FD_SET(d, &r);
		tv.tv_sec = 0;
		tv.tv_usec = 100000L;
#ifdef AUX
		mac_sel(s, 1, 6L);
		tv.tv_usec = 0;
#endif
		if (select((s > d ? s : d) + 1, &r, &w, (fd_set *)0, &tv) > 0 &&
			FD_ISSET(s, &w))
			break;
		if (ui_poll()) {
			scopy(err, "stopped", 60);
			return -1;
		}
		if (now() - t0 >= CWAIT) {
			scopy(err, "no answer", 60);
			return -1;
		}
	}
	e = 0;
#ifdef SO_ERROR
	n = sizeof e;
	if (getsockopt(s, SOL_SOCKET, SO_ERROR, (char *)&e, &n) < 0)
		e = errno;
#endif
	return e;
}

int n_open(host, port, err)
char *host, *err;
int port;
{
	static struct sockaddr_in sa;
	struct hostent *he;
	char *p, *d;
	int s, i, e;

	d = (char *)&sa;
	for (i = 0; i < (int)sizeof sa; ++i)
		d[i] = 0;
	sa.sin_family = AF_INET;
	sa.sin_port = htons((unsigned short)port);
	sa.sin_addr.s_addr = inet_addr(host);
	if (sa.sin_addr.s_addr == 0xffffffffL) {
		he = gethostbyname(host);
		if (!he) {
			scopy(err, "unknown host", 60);
			return -1;
		}
		p = (char *)he->h_addr;
		d = (char *)&sa.sin_addr;
		for (i = 0; i < he->h_length && i < (int)sizeof sa.sin_addr; ++i)
			d[i] = p[i];
	}
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		scopy(err, "no socket", 60);
		return -1;
	}
	nbio(s, 1);
	e = 0;
	if (connect(s, (struct sockaddr *)&sa, sizeof sa) < 0) {
		e = errno;
#ifdef EINPROGRESS
		if (e == EINPROGRESS)
			e = cwait(s, err);
#endif
	}
	if (e) {
		if (e > 0)
			why(err, e);
		close(s);
		return -1;
	}
	nbio(s, 0);
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
#ifdef AUX
	if (wait)
		mac_sel(h, 0, 3L);
	tv.tv_usec = 0;
#endif
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
