/* update.c - Archaic updates itself, through WRP.
 *
 * Archaic asks WRP for the newest Archaic for its system (SYSID, in
 * archaic.h): once a run, when WRP's first page is here and there is
 * nothing else to do (main.c), or at once with archaic -u:
 *
 *	GET /archaic-update?p=unixpc&v=0.8
 *
 * WRP takes Archaic's file for that system (archaic-unixpc) from the
 * GitHub release, and checks it: the old systems have no HTTPS. It
 * answers in plain text, a line each:
 *
 *	archaic-update 1
 *	version 0.9
 *	file archaic-unixpc
 *	size 67027
 *	crc 1f2e3d4c
 *	get archaic-update?p=unixpc&f=0.9
 *
 * With nothing to offer, a "note" line says why, and there is no
 * version. An older WRP answers with its page: no updates there.
 *
 * When the version is newer, and Archaic can write the folder of its
 * program (as root), it gets the file ("get", next to the question)
 * into that folder as archaic.PID, reads it again to check its size and
 * CRC-32, makes it executable, and puts it in the place of the program:
 * with rename(), or with link() and unlink() where there is no rename
 * (NORENAME). The program that runs stays as it is: the next start is
 * the new version. The old one stays as archaic.old. When Archaic can't
 * write the folder, it says that the new version is out.
 *
 * The program carries "@(#)Archaic VERSION" (main.c) and "@(#)Archaic
 * for SYSTEM" (here). WRP reads them in the release's file, so it does
 * not offer a file of an older version (a release also carries older
 * builds, for the systems not built again), or one for another system.
 *
 * Only Unix builds of the release have SYSID: the others get no updates.
 */
#include "archaic.h"

char upd_msg[300];		/* what upd_run() did, or why not */

#ifdef SYSID

#ifdef NOPROTO
char *getenv();
#else
#include <stdlib.h>
#endif
#ifdef __STDC__		/* the prototypes; the old compilers need none */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef NOUNISTD
#include <unistd.h>
#endif
#endif

char sysid[] = SYSID;
#define SYSNAME (sysid + 16)	/* the name, after "@(#)Archaic for " */

#define UMAX 8388608L		/* a larger file is not Archaic */

static char ans[1024];		/* WRP's answer */
static char nver[24];		/* in it: the newest version, */
static long nsize;		/*  its size, */
static unsigned long ncrc;	/*  CRC-32, */
static int hcrc;		/*  (there is one) */
static char nget[300];		/*  and where to get it; */
static char nnote[160];		/*  or why there is none */
static char uq[MAXURL];		/* the question */
static char uf[MAXURL];		/* the file's address */
static char self[300];		/* the program's file */
static char dir[300];		/* its folder */
static char tmpf[340], oldf[340];
static unsigned char blk[1024];
static unsigned long crctab[256];

static int ulen(s)
char *s;
{
	int n = 0;

	while (s[n])
		++n;
	return n;
}

/* the number at s: base 10, or 16 */
static unsigned long unum(s, base)
char *s;
int base;
{
	unsigned long n = 0;
	int c, d, k;

	for (k = 0; (c = *s) != 0 && k < 9; ++s, ++k) {
		if (c >= '0' && c <= '9')
			d = c - '0';
		else if (base == 16 && c >= 'a' && c <= 'f')
			d = c - 'a' + 10;
		else if (base == 16 && c >= 'A' && c <= 'F')
			d = c - 'A' + 10;
		else
			break;
		n = n * base + d;
	}
	return n & 0xffffffffL;
}

/* Is version a newer than version b? Their numbers, part by part:
 * 0.10 is newer than 0.9 */
static int newer(a, b)
char *a, *b;
{
	long x, y;
	int k;

	while (*a || *b) {
		x = y = 0;
		for (k = 0; *a >= '0' && *a <= '9'; ++a)
			if (++k < 9)
				x = x * 10 + *a - '0';
		for (k = 0; *b >= '0' && *b <= '9'; ++b)
			if (++k < 9)
				y = y * 10 + *b - '0';
		if (x != y)
			return x > y;
		while (*a && (*a < '0' || *a > '9'))
			++a;
		while (*b && (*b < '0' || *b > '9'))
			++b;
	}
	return 0;
}

/* CRC-32 (IEEE: as zlib and WRP), on n more bytes at p. Start with c =
 * 0xffffffff; at the end, c ^ 0xffffffff is the CRC. */
static unsigned long ucrc(c, p, n)
unsigned long c;
unsigned char *p;
int n;
{
	unsigned long t;
	int i, k;

	if (!crctab[1])
		for (i = 0; i < 256; ++i) {
			t = (unsigned long)i;
			for (k = 0; k < 8; ++k)
				t = t & 1 ? (unsigned long)0xedb88320L ^ (t >> 1) : t >> 1;
			crctab[i] = t & 0xffffffffL;
		}
	while (n-- > 0)
		c = crctab[(int)((c ^ *p++) & 0377)] ^ (c >> 8);
	return c & 0xffffffffL;
}

/* WRP's answer (n bytes in ans), a line each: "key value". 0 if it is
 * not an answer to Archaic */
static int uparse(n)
int n;
{
	char *p = ans, *e = ans + n, *k, *v;
	int first = 1, i;

	nver[0] = nget[0] = nnote[0] = 0;
	nsize = -1;
	ncrc = 0;
	hcrc = 0;
	ans[n] = 0;
	while (p < e) {
		k = p;
		while (p < e && *p != '\n')
			++p;
		*p++ = 0;
		for (i = ulen(k); i > 0 && (k[i - 1] == '\r' || k[i - 1] == ' '); --i)
			k[i - 1] = 0;
		for (v = k; *v && *v != ' '; ++v)
			;
		if (*v)
			*v++ = 0;
		if (first) {
			if (!strieq(k, "archaic-update") || unum(v, 10) != 1)
				return 0;
			first = 0;
		} else if (strieq(k, "version")) {
			for (i = 0; i < (int)sizeof nver - 1 && ((v[i] >= '0' &&
				v[i] <= '9') || (v[i] >= 'a' && v[i] <= 'z') ||
				(v[i] >= 'A' && v[i] <= 'Z') || v[i] == '.'); ++i)
				nver[i] = v[i];
			nver[i] = 0;
		} else if (strieq(k, "size"))
			nsize = (long)unum(v, 10);
		else if (strieq(k, "crc")) {
			ncrc = unum(v, 16);
			hcrc = 1;
		} else if (strieq(k, "get"))
			scopy(nget, v, (int)sizeof nget);
		else if (strieq(k, "note"))
			scopy(nnote, v, (int)sizeof nnote);
	}
	return !first;
}

/* Is p there, and a program? */
static int isprog(p)
char *p;
{
	return access(p, 1) == 0;
}

/* the program's file: as it was started (a), with its folder, else
 * where the search path finds it, else where it is put (UPDPATH).
 * 0 if none */
static int findself(a)
char *a;
{
	char *d, *p;
	int n, k;

	for (p = a; *p && *p != '/'; ++p)
		;
	if (*p) {
		scopy(self, a, (int)sizeof self);
		return isprog(self);
	}
	k = ulen(a);
	/* dir:dir..., and on Idris |dir/|dir/... */
	for (d = getenv("PATH"); d && *d;) {
		for (n = 0; d[n] && d[n] != ':' && d[n] != '|'; ++n)
			;
		if (n + k + 3 < (int)sizeof self) {
			if (n == 0)
				scopy(self, ".", (int)sizeof self);
			else
				scopy(self, d, n + 1);
			if (self[ulen(self) - 1] != '/')
				scopy(self + ulen(self), "/", 2);
			scopy(self + ulen(self), a, k + 1);
			if (isprog(self))
				return 1;
		}
		d += n;
		if (*d)
			++d;
	}
#ifdef UPDPATH
	scopy(self, UPDPATH, (int)sizeof self);
	return isprog(self);
#else
	return 0;
#endif
}

/* the folder of self */
static void dirof()
{
	int i;

	scopy(dir, self, (int)sizeof dir);
	for (i = ulen(dir); i > 0 && dir[i - 1] != '/'; --i)
		;
	if (i == 0)
		scopy(dir, ".", (int)sizeof dir);
	else
		dir[i > 1 ? i - 1 : 1] = 0;	/* "/archaic": "/" */
}

/* get the file into tmpf. 0 if all of it came, else -1 (upd_msg says why) */
static int ufetch()
{
	int fd, n = 0, c;
	long got = 0;

	url_join(uq, nget, uf);
	if (http_open(uf, (char *)0) < 0) {
		sprintf(upd_msg, "Archaic %s: %.100s", nver, http_err);
		return -1;
	}
	if (http_status != 200 || (http_len >= 0 && http_len != nsize)) {
		sprintf(upd_msg, "Archaic %s: WRP sends HTTP %d, %ld bytes", nver,
			http_status, http_len);
		http_close();
		return -1;
	}
	fd = creat(tmpf, 0644);
	if (fd < 0) {
		sprintf(upd_msg, "Archaic %s: unable to write %.200s", nver, tmpf);
		http_close();
		return -1;
	}
	while (got < nsize && (c = HGETC()) >= 0) {
		blk[n++] = c;
		if (++got == nsize || n == (int)sizeof blk) {
			if (write(fd, (char *)blk, n) != n) {
				close(fd);
				sprintf(upd_msg, "Archaic %s: unable to write %.200s (is the disk full?)",
					nver, tmpf);
				http_close();
				return -1;
			}
			n = 0;
			http_sync();
			ui_prog();
		}
	}
	http_sync();
	http_close();
	if (close(fd) < 0 || got != nsize) {
		if (got != nsize)
			sprintf(upd_msg, "Archaic %s: %ld of %ld bytes came%s%.100s",
				nver, got, nsize, http_err[0] ? ": " : "", http_err);
		else
			sprintf(upd_msg, "Archaic %s: unable to write %.200s", nver,
				tmpf);
		return -1;
	}
	return 0;
}

/* Is file f, as it is on the disk, WRP's file: its size and CRC? */
static int ucheck(f)
char *f;
{
	unsigned long c = 0xffffffffL;
	long got = 0;
	int fd, n;

	fd = open(f, 0);
	if (fd < 0)
		return 0;
	while ((n = read(fd, (char *)blk, (int)sizeof blk)) > 0) {
		got += n;
		c = ucrc(c, blk, n);
	}
	close(fd);
	return n == 0 && got == nsize && ((c ^ 0xffffffffL) & 0xffffffffL) == ncrc;
}

/* Are a and b the same name? */
static int ueq(a, b)
char *a, *b;
{
	while (*a && *a == *b)
		++a, ++b;
	return *a == *b;
}

/* tmpf in the place of self; the old one stays as archaic.old.
 * 0 ok, -1 if not (upd_msg says why) */
static int uinstall()
{
#ifdef NORENAME
	int kept = 0;
#endif

	if (chmod(tmpf, 0755) < 0) {
		sprintf(upd_msg, "Archaic %s: unable to make %.200s executable",
			nver, tmpf);
		return -1;
	}
	sprintf(oldf, "%s/archaic.old", dir);
	if (!ueq(oldf, self)) {
		unlink(oldf);
#ifdef NORENAME
		kept = link(self, oldf) == 0;
#else
		link(self, oldf);
#endif
	}
#ifdef NORENAME
	/* self goes, then tmpf gets its name: only with the old one kept
	 * as archaic.old, to put it back if the link fails */
	if (!kept || unlink(self) < 0) {
		sprintf(upd_msg, "Archaic %s: unable to replace %.200s", nver, self);
		return -1;
	}
	if (link(tmpf, self) < 0) {
		link(oldf, self);
		sprintf(upd_msg, "Archaic %s: unable to replace %.200s", nver, self);
		return -1;
	}
	unlink(tmpf);
#else
	if (rename(tmpf, self) < 0) {
		sprintf(upd_msg, "Archaic %s: unable to replace %.200s", nver, self);
		return -1;
	}
#endif
	return 0;
}

/* Ask WRP (at the address wrp) for the newest Archaic, and install it if
 * it is newer and Archaic can write the folder of its program (started
 * as prog). upd_msg says what it did, or why not. U_NONE: this is the
 * newest; U_ERR: no answer, or WRP has no file for this system; U_OUT: a
 * newer one is out, Archaic can't install it; U_DONE: installed for the
 * next start; U_FAIL: the install failed; U_STOP: stopped. */
int upd_run(wrp, prog)
char *wrp, *prog;
{
	int n, c, r;

	upd_msg[0] = 0;
	ui_what("");		/* the question: nothing on the status line */
	sprintf(ans, "archaic-update?p=%.20s&v=%.20s", SYSNAME, VERSION);
	url_join(wrp, ans, uq);
	if (http_open(uq, (char *)0) < 0) {
		if (http_noconn)
			sprintf(upd_msg, "Unable to connect to WRP at %.60s:%d: %.100s",
				http_host, http_port, http_err);
		else
			sprintf(upd_msg, "Update: %.150s", http_err);
		return stop ? U_STOP : U_ERR;
	}
	for (n = 0; n < (int)sizeof ans - 1 && (c = http_getc()) >= 0;)
		ans[n++] = c;
	r = http_status;
	http_close();
	if (stop)
		return U_STOP;
	if (r != 200 || !uparse(n)) {
		scopy(upd_msg, "This WRP has no updates for Archaic: it is an older WRP",
			(int)sizeof upd_msg);
		return U_ERR;
	}
	if (!nver[0]) {
		sprintf(upd_msg, "WRP has no update: %.150s", nnote[0] ? nnote :
			"no reason given");
		return U_ERR;
	}
	if (!newer(nver, VERSION)) {
		sprintf(upd_msg, "Archaic %s is the newest", VERSION);
		return U_NONE;
	}
	if (nsize <= 0 || nsize > UMAX || !hcrc || !nget[0]) {
		sprintf(upd_msg, "Archaic %s is out, but WRP's answer is not complete",
			nver);
		return U_FAIL;
	}
	if (!findself(prog)) {
		sprintf(upd_msg, "Archaic %s is out. To install it, start it as root, by its full name",
			nver);
		return U_OUT;
	}
	dirof();
	if (access(dir, 2) < 0) {
		if (getuid() != 0) {
			sprintf(upd_msg, "Archaic %s is out. To install it, start archaic as root",
				nver);
			return U_OUT;
		}
		sprintf(upd_msg, "Archaic %s is out, but Archaic can't write %.200s",
			nver, dir);
		return U_FAIL;
	}
	if (ucheck(self)) {	/* another Archaic has installed it */
		sprintf(upd_msg, "Archaic %s is installed. Start archaic again to use it: %.200s",
			nver, self);
		return U_DONE;
	}
	sprintf(tmpf, "%s/archaic.%ld", dir, (long)getpid() % 100000L);
	sprintf(ans, "Archaic %s", nver);
	ui_what(ans);
	r = ufetch();
	ui_what("");
	if (r == 0 && !ucheck(tmpf)) {
		sprintf(upd_msg, "Archaic %s came broken (its size or CRC): it is not installed",
			nver);
		r = -1;
	}
	if (r == 0 && uinstall() == 0) {
		sprintf(upd_msg, "Archaic %s is installed. Start archaic again to use it: %.200s",
			nver, self);
		return U_DONE;
	}
	unlink(tmpf);
	return stop ? U_STOP : U_FAIL;
}

#else	/* no SYSID: a build that the release does not have */

int upd_run(wrp, prog)
char *wrp, *prog;
{
	scopy(upd_msg, "This Archaic gets no updates: it is not a build of the release (no SYSID)",
		(int)sizeof upd_msg);
	return U_ERR;
}

#endif
