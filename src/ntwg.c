/* ntwg.c - Archaic's network for WIN/TCP of The Wollongong Group (TWG),
 * the TCP/IP of the AT&T UNIX PC (3B1), and for WIN/3B, the same on the
 * AT&T 3B2 (headers in /usr/include, sys/in.h). It has BSD sockets, as in
 * 4.2BSD:
 * - the headers are in /usr/ethernet/include and the library is -lnet.
 *   TWG's READ_ME: compile with -DSYSTEM5 -DSHORT_IDENT
 * - select() takes bit masks, not fd_set. It cannot wait for the window
 *   too, so n_read waits at most 0.1 s: then the window gets its turn
 * - on the 3B1, gethostbyname() reads /etc/hosts only (no DNS). WIN/3B's
 *   also asks the name server of /etc/resolv.conf. An address such as
 *   10.0.2.2 goes through inet_addr() first
 */
#include "archaic.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#ifdef u3b2
#include <sys/in.h>		/* WIN/3B on the 3B2 has it here */
#else
#include <netinet/in.h>
#endif
#include <netdb.h>

unsigned long inet_addr();

int n_open(host, port, err)
char *host, *err;
int port;
{
	static struct sockaddr_in sa;
	struct hostent *he;
	unsigned long a;
	char *p, *d;
	int s, i;

	d = (char *)&sa;
	for (i = 0; i < (int)sizeof sa; ++i)
		d[i] = 0;
	sa.sin_family = AF_INET;
	sa.sin_port = htons((unsigned short)port);
	a = inet_addr(host);
	if (a != (unsigned long)-1)
		sa.sin_addr.s_addr = a;
	else {
		he = gethostbyname(host);
		if (!he) {
#ifdef u3b2
			sprintf(err, "%.60s: unknown host", host);
#else
			sprintf(err, "%.60s: unknown host (not in /etc/hosts)", host);
#endif
			return -1;
		}
		p = (char *)he->h_addr;
		d = (char *)&sa.sin_addr;
		for (i = 0; i < he->h_length && i < (int)sizeof sa.sin_addr; ++i)
			d[i] = p[i];
	}
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		sprintf(err, "no socket (is the network up?)");
		return -1;
	}
	if (connect(s, (struct sockaddr *)&sa, sizeof sa) < 0) {
		sprintf(err, "can't connect to %.60s port %d", host, port);
		close(s);
		return -1;
	}
	return s;
}

/* wait at most 0.1 s (or not at all) */
int n_read(h, buf, n, wait)
int h, n, wait;
char *buf;
{
	static struct timeval tv;
	long r;
	int m;

	r = 1L << h;
	tv.tv_sec = 0;
	tv.tv_usec = wait ? 100000L : 0L;
	m = select(h + 1, &r, (long *)0, (long *)0, &tv);
	if (m <= 0 || !(r & (1L << h)))
		return -2;
	m = recv(h, buf, n, 0);
	return m < 0 ? -1 : m;
}

int n_write(h, buf, n)
int h, n;
char *buf;
{
	int k;

	while (n > 0) {
		k = send(h, buf, n, 0);
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
