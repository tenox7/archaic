/* hget - fetch one file over HTTP/1.0 through netd (Idris)
 *
 * Usage: hget host port path file
 *
 * Build in Idris, next to netc.h (from /src/net):
 *	c hget.c
 *	sh lk hget		(lk from /src/net)
 * Bootstrap tool: it brings the browser's sources in from the Mac
 * (tools/idris.py serve) much faster than typing them in.
 */
#include "netc.h"

int main(int ac, char **av)
{
	static char req[300], ans[200];
	static unsigned char b[512];
	int s, n, fd, i, hdr, nl;
	long total;

	if (ac != 5) {
		fprintf(stderr, "usage: hget host port path file\n");
		return 2;
	}
	sprintf(req, "tcp %s %s\n", av[1], av[2]);
	s = netreq(req, ans, sizeof ans);
	if (s < 0) {
		fprintf(stderr, "hget: %s\n", ans);
		return 1;
	}
	sprintf(req, "GET %s HTTP/1.0\r\n\r\n", av[3]);
	write(s, req, strlen(req));
	fd = creat(av[4], 0644);
	if (fd < 0) {
		fprintf(stderr, "hget: can't create %s\n", av[4]);
		close(s);
		return 1;
	}
	hdr = 1;
	nl = 0;
	total = 0;
	while (nwait(s, 60L) > 0) {
		n = nread(s, b, sizeof b);
		if (n == 0)
			break;
		if (n < 0)
			continue;
		i = 0;
		while (hdr && i < n) {		/* the headers end with an empty line */
			if (b[i] == '\n') {
				if (nl)
					hdr = 0;
				nl = 1;
			} else if (b[i] != '\r')
				nl = 0;
			++i;
		}
		if (i < n) {
			write(fd, b + i, n - i);
			total += n - i;
		}
	}
	close(fd);
	close(s);
	printf("%s: %ld bytes\n", av[4], total);
	return 0;
}
