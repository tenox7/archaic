/* nsting.c - Archaic's network for TOS on the Atari: STinG's TCP (or
 * STiK's, the same calls), as with a NetUSBee or an EtherNEC card and
 * STinG's ENEC.STX. Built with tos.c, by mktos.
 *
 * STinG puts its driver list in the cookie "STiK"; get_dftab gives the
 * TCP layer's table of functions (TPL). They are Pure C functions with
 * cdecl: the arguments go on the stack in their own sizes (16-bit words
 * stay words, gcc would push longs), and they may change d2, which gcc
 * keeps. st_call() below pushes the words, saves the registers and
 * takes the result in d0.
 *
 * The connect does not hold up the window: TCP_open returns at once,
 * and Archaic asks TCP_info for the state, while ui_poll() draws and
 * Esc stops. A name (not an address) goes through STinG's resolve(),
 * which waits.
 */
#include "archaic.h"
#include <osbind.h>

#define CWAIT 20		/* seconds to wait for a connect */
#define OBSIZE 4096		/* TCP_open's buffer */

/* the TPL's functions: their places after module, author, version */
#define F_ERRTEXT 4
#define F_OPEN 7
#define F_CLOSE 8
#define F_SEND 9
#define F_WAITST 10
#define F_COUNT 16
#define F_BLOCK 19
#define F_RESOLVE 21
#define F_INFO 35

/* STinG's errors */
#define E_OBUFFULL -1
#define E_NODATA -2
#define E_EOF -3
#define E_RRESET -4
#define E_REFUSE -7
#define E_LISTEN -10
#define E_CONNECTFAIL -13
#define E_USERTIMEOUT -15
#define E_CNTIMEOUT -16
#define E_UNREACHABLE -24
#define E_FNAVAIL -32

#define TSYN_SENT 2
#define TESTABLISH 4

long st_call P((long fn, long n, unsigned short *w));
static int same9 P((char *d));

/* st_call(fn, n, w): call fn with the n words w[0] ... w[n - 1] on the
 * stack (w[0] at the lowest address, as the first argument) */
__asm__(
"	.text\n"
"	.even\n"
"_st_call:\n"
"	moveml	%d2-%d7/%a2-%a6,%sp@-\n"
"	movel	%sp@(48),%a0\n"
"	movel	%sp@(52),%d7\n"
"	movel	%sp@(56),%a1\n"
"	addl	%d7,%d7\n"
"	addal	%d7,%a1\n"
"	movel	%d7,%d1\n"
"	beqs	2f\n"
"1:	movew	%a1@-,%sp@-\n"
"	subql	#2,%d1\n"
"	bnes	1b\n"
"2:	jsr	%a0@\n"
"	addal	%d7,%sp\n"
"	moveml	%sp@+,%d2-%d7/%a2-%a6\n"
"	rts\n");

static long tpl;		/* STinG's TCP layer, 0 until found */
static unsigned short wa[16];	/* the arguments of the next call */
static int nw;
static long jar;		/* the cookie jar */

static long getjar()
{
	jar = *(long *)0x5a0L;
	return 0;
}

/* the cookie's value, 0 if none */
static long cookie(id)
long id;
{
	long *p;

	Supexec(getjar);
	for (p = (long *)jar; p && p[0]; p += 2)
		if (p[0] == id)
			return p[1];
	return 0;
}

static void w16(v)
long v;
{
	wa[nw++] = (unsigned short)v;
}

static void w32(v)
long v;
{
	wa[nw++] = (unsigned short)(v >> 16);
	wa[nw++] = (unsigned short)v;
}

/* call function f of the TPL with the words given; a 16-bit result */
static int st(f)
int f;
{
	long r = st_call(*(long *)(tpl + 12 + 4 * f), (long)nw, wa);

	nw = 0;
	return (short)r;
}

/* find STinG: 0, else -1 and why in err */
static int sting(err)
char *err;
{
	static char drv[] = "TRANSPORT_TCPIP";
	char *d = (char *)cookie(0x5354694bL);	/* 'STiK' */
	long t;

	if (!d || !same9(d)) {
		scopy(err, "STinG is not loaded", 60);
		return -1;
	}
	w32((long)drv);
	t = st_call(*(long *)(d + 10), (long)nw, wa);
	nw = 0;
	if (!t) {
		scopy(err, "STinG has no TCP", 60);
		return -1;
	}
	tpl = t;
	return 0;
}

/* d starts with "STiKmagic" */
static int same9(d)
char *d;
{
	static char m[] = "STiKmagic";
	int i;

	for (i = 0; i < 9; ++i)
		if (d[i] != m[i])
			return 0;
	return 1;
}

/* an address like 10.0.2.2: 1 and *ip, else 0 */
static int dotted(s, ip)
char *s;
unsigned long *ip;
{
	unsigned long a = 0, v;
	int n;

	for (n = 0; n < 4; ++n) {
		if (*s < '0' || *s > '9')
			return 0;
		for (v = 0; *s >= '0' && *s <= '9'; ++s)
			v = v * 10 + *s - '0';
		if (v > 255)
			return 0;
		a = a << 8 | v;
		if (n < 3 && *s++ != '.')
			return 0;
	}
	if (*s)
		return 0;
	*ip = a;
	return 1;
}

/* STinG's error e in words */
static void why(err, e)
char *err;
int e;
{
	char *s = 0;

	switch (e) {
	case E_REFUSE:
	case E_RRESET:
		s = "connection refused";
		break;
	case E_CNTIMEOUT:
		s = "no answer";
		break;
	case E_UNREACHABLE:
		s = "host unreachable";
		break;
	case E_CONNECTFAIL:
		s = "no connection";
		break;
	}
	if (s)
		scopy(err, s, 60);
	else
		sprintf(err, "STinG error %d", e);
}

/* the connection's state (TCPIB: request, state, ...), or -1 if STinG
 * can't say */
static int state(h)
int h;
{
	static unsigned short ib[8];
	int r;

	ib[0] = 0;
	ib[1] = 1;		/* TCPI_state */
	ib[2] = 0;
	w16((long)h);
	w32((long)ib);
	r = st(F_INFO);
	return r < 0 ? -1 : ib[2];
}

int n_open(host, port, err)
char *host, *err;
int port;
{
	static unsigned long ip;
	long t0;
	int h, r, s, ok = 0;

	if (!tpl && sting(err) < 0)
		return -1;
	if (!dotted(host, &ip)) {
		w32((long)host);
		w32(0L);
		w32((long)&ip);
		w16(1L);
		if (st(F_RESOLVE) <= 0) {
			scopy(err, "unknown host", 60);
			return -1;
		}
	}
	w32((long)ip);
	w16((long)port);
	w16(0L);
	w16((long)OBSIZE);
	h = st(F_OPEN);
	if (h < 0) {
		why(err, h);
		return -1;
	}
	t0 = now();
	for (;;) {
		s = state(h);
		if (s < 0) {	/* STiK: no TCP_info, wait a second */
			w16((long)h);
			w16((long)TESTABLISH);
			w16(1L);
			r = st(F_WAITST);
			if (r == 0) {
				ok = 1;
				break;
			}
			if (r != E_CNTIMEOUT && r != E_USERTIMEOUT) {
				why(err, r);
				break;
			}
		} else if (s >= TESTABLISH) {
			ok = 1;
			break;
		} else if (s < TSYN_SENT) {	/* closed: why? */
			w16((long)h);
			r = st(F_COUNT);
			why(err, r < 0 && r != E_NODATA ? r : E_REFUSE);
			break;
		}
		if (ui_poll()) {
			scopy(err, "stopped", 60);
			break;
		}
		if (now() - t0 >= CWAIT) {
			scopy(err, "no answer", 60);
			break;
		}
	}
	if (!ok) {
		n_close(h);
		return -1;
	}
	return h;
}

/* wait at most 1/20 s (or not at all) for bytes */
int n_read(h, buf, n, wait)
int h, n, wait;
char *buf;
{
	long t0 = g_ms();
	int r;

	for (;;) {
		w16((long)h);
		r = st(F_COUNT);
		if (r > 0)
			break;
		if (r == E_EOF)
			return 0;
		if (r < 0 && r != E_NODATA && r != E_LISTEN)
			return -1;
		if (!wait || t0 < 0 || g_ms() - t0 >= 50 || g_ms() < t0)
			return -2;
	}
	if (n > r)
		n = r;
	w16((long)h);
	w32((long)buf);
	w16((long)n);
	r = st(F_BLOCK);
	if (r == E_NODATA)
		return -2;
	return r < 0 ? -1 : r;
}

int n_write(h, buf, n)
int h, n;
char *buf;
{
	long t0 = now();
	int k, r;

	while (n > 0) {
		k = n > OBSIZE / 2 ? OBSIZE / 2 : n;
		w16((long)h);
		w32((long)buf);
		w16((long)k);
		r = st(F_SEND);
		if (r == E_OBUFFULL) {	/* no room yet */
			if (now() - t0 >= CWAIT)
				return -1;
			continue;
		}
		if (r < 0)
			return -1;
		buf += k;
		n -= k;
	}
	return 0;
}

void n_close(h)
int h;
{
	static short res;

	w16((long)h);
	w16(0L);		/* no wait */
	w32((long)&res);
	st(F_CLOSE);
}
