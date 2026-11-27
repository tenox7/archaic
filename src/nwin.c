/* nwin.c - Archaic's network for Windows NT: Winsock 1.1 (WSOCK32.DLL),
 * as NT 3.1 has it. As nsock.c: an address such as 10.0.2.2 goes
 * through inet_addr() first; the connect does not hold up the window
 * (FIONBIO), Esc stops the wait, and after CWAIT seconds Archaic gives
 * up. Winsock says that a connect failed in select()'s third set. */
#include "archaic.h"
#include <windows.h>
#include <winsock.h>

#define CWAIT 20		/* seconds to wait for a connect */

static int wsup;		/* WSAStartup done */

/* why it failed: the error e in words */
static void why(err, e)
char *err;
int e;
{
	char *s = 0;

	if (e == WSAECONNREFUSED)
		s = "connection refused";
	else if (e == WSAETIMEDOUT)
		s = "no answer";
	else if (e == WSAENETUNREACH)
		s = "network unreachable";
	else if (e == WSAEHOSTUNREACH)
		s = "host unreachable";
	if (s)
		scopy(err, s, 60);
	else
		sprintf(err, "error %d", e);
}

/* the connect goes on: wait for it, and let the user act meanwhile
 * (ui_poll: the window is drawn, Esc stops). 0 connected, -1 stopped
 * or no answer (err says), else the error */
static int cwait(s, err)
SOCKET s;
char *err;
{
	fd_set w, x;
	struct timeval tv;
	long t0 = now();
	int e, n;

	for (;;) {
		FD_ZERO(&w);
		FD_ZERO(&x);
		FD_SET(s, &w);
		FD_SET(s, &x);
		tv.tv_sec = 0;
		tv.tv_usec = 100000L;
		if (select(0, (fd_set *)0, &w, &x, &tv) > 0) {
			if (FD_ISSET(s, &w))
				return 0;
			e = 0;
			n = sizeof e;
			if (getsockopt(s, SOL_SOCKET, SO_ERROR, (char *)&e, &n) ==
				SOCKET_ERROR || !e)
				e = WSAECONNREFUSED;
			return e;
		}
		if (ui_poll()) {
			scopy(err, "stopped", 60);
			return -1;
		}
		if (now() - t0 >= CWAIT) {
			scopy(err, "no answer", 60);
			return -1;
		}
	}
}

int n_open(host, port, err)
char *host, *err;
int port;
{
	static struct sockaddr_in sa;
	static WSADATA wd;
	struct hostent *he;
	char *p, *d;
	SOCKET s;
	u_long on = 1;
	int i, e;

	if (!wsup) {
		if (WSAStartup(0x0101, &wd) != 0) {
			scopy(err, "no Winsock (is TCP/IP installed?)", 60);
			return -1;
		}
		wsup = 1;
	}
	d = (char *)&sa;
	for (i = 0; i < (int)sizeof sa; ++i)
		d[i] = 0;
	sa.sin_family = AF_INET;
	sa.sin_port = htons((u_short)port);
	sa.sin_addr.s_addr = inet_addr(host);
	if (sa.sin_addr.s_addr == INADDR_NONE) {
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
	if (s == INVALID_SOCKET) {
		scopy(err, "no socket", 60);
		return -1;
	}
	ioctlsocket(s, FIONBIO, &on);
	e = 0;
	if (connect(s, (struct sockaddr *)&sa, sizeof sa) == SOCKET_ERROR) {
		e = WSAGetLastError();
		if (e == WSAEWOULDBLOCK || e == WSAEINPROGRESS)
			e = cwait(s, err);
	}
	if (e) {
		if (e > 0)
			why(err, e);
		closesocket(s);
		return -1;
	}
	on = 0;
	ioctlsocket(s, FIONBIO, &on);
	return (int)s;
}

/* wait at most 1/20 s (or not at all): the window has no file to wait
 * on, so it gets its turn then, and the spinner turns (main.c) */
int n_read(h, buf, n, wait)
int h, n, wait;
char *buf;
{
	fd_set r;
	struct timeval tv;
	int m;

	FD_ZERO(&r);
	FD_SET((SOCKET)h, &r);
	tv.tv_sec = 0;
	tv.tv_usec = wait ? 50000L : 0;
	if (select(0, &r, (fd_set *)0, (fd_set *)0, &tv) <= 0)
		return -2;
	m = recv((SOCKET)h, buf, n, 0);
	return m == SOCKET_ERROR ? -1 : m;
}

int n_write(h, buf, n)
int h, n;
char *buf;
{
	int k;

	while (n > 0) {
		k = send((SOCKET)h, buf, n, 0);
		if (k == SOCKET_ERROR || k <= 0)
			return -1;
		buf += k;
		n -= k;
	}
	return 0;
}

void n_close(h)
int h;
{
	closesocket((SOCKET)h);
}
