/* nmactcp.c - Archaic's network for the classic Mac OS: the TCP of
 * MacTCP (its driver, .IPP). Archaic has one connection at a time
 * (http.c), so this has one stream, with a receive buffer from NewPtr
 * once. Each call goes to the driver asynchronously. A wait longer than
 * a tick is spent in mac_wait (WaitNextEvent), so the other programs
 * run meanwhile, and the Mac sleeps when they have nothing to do. While
 * the connect waits, the window is drawn and Esc (Cmd-.) stops it
 * (ui_poll). Open Transport's MacTCP part works the same way.
 * An address such as 10.0.0.1 needs no name server; a name goes to the
 * MacTCP resolver (dnr.c). In the Snow emulator, the Mac's 127.0.0.1
 * is the host's own: WRP on the host is 127.0.0.1:8080.
 * The stream must go before Archaic quits, or MacTCP writes into
 * memory that is no longer Archaic's: atexit() releases it.
 * Each connection gets its own local port, the next one after the last.
 * MacTCP picks its ports from a small range, and gave a new connection
 * the port of the one that had just closed: Snow's NAT then dropped its
 * SYNs for 13 s, and a server in TIME_WAIT may refuse it too. */
#include "archaic.h"
#include <stdlib.h>
#include <Multiverse.h>
#include "dnr.h"

#define RBUF 16384L		/* the stream's buffer: MacTCP's window */
#define CWAIT 20		/* seconds to wait for a connect, a send */
#define CTICKS 120		/* ticks to wait for a close */
#define E_DEAD (-23033)		/* ipDestDeadErr: Multiverse.h has no name */
#define E_ROUTE (-23037)	/* ipRouteErr */
#define PLOW 20000L		/* Archaic's local ports: PLOW up to PLOW+PNUM-1 */
#define PNUM 20000L

static short ipp;		/* the driver's refnum; 0: not open */
static Ptr rbuf;		/* the stream's buffer */
static StreamPtr strm;		/* the stream; 0: none */
static int dnrok;		/* 1: the resolver is open */
static int bye;			/* 1: atexit() has the clean up */
static long lport;		/* the last local port; 0: none yet */
static TCPiopb pb, xpb;		/* MacTCP may write them after a wait: static */

static void zero(p, n)
char *p;
int n;
{
	while (n-- > 0)
		*p++ = 0;
}

/* start call code of p, on the stream */
static int tcall(p, code)
TCPiopb *p;
int code;
{
	p->ioCompletion = 0;
	p->ioCRefNum = ipp;
	p->csCode = code;
	p->tcpStream = strm;
	return PBControlAsync((ParmBlkPtr)p);
}

/* wait for call p: 0, or MacTCP's error. Most calls are done at once;
 * after 2 ticks the other programs run while it waits. */
static int twait(p)
TCPiopb *p;
{
	long t = (long)TickCount() + 2;

	while (p->ioResult > 0)
		if ((long)TickCount() < t)
			SystemTask();
		else
			mac_wait(1L);
	return p->ioResult;
}

/* call code of p, and wait for it */
static int tdo(p, code)
TCPiopb *p;
int code;
{
	int e = tcall(p, code);

	return e ? e : twait(p);
}

/* the stream away: also ends its connection, and a call that waits */
static void release()
{
	if (!strm)
		return;
	zero((char *)&xpb, (int)sizeof xpb);
	tdo(&xpb, TCPRelease);
	strm = 0;
}

static void cleanup()
{
	release();
	if (dnrok)
		dnr_close();
	dnrok = 0;
}

/* the address in s, if it is 4 numbers with dots; else 0 */
static unsigned long dotted(s)
char *s;
{
	unsigned long a = 0, v;
	int i;

	for (i = 0; i < 4; ++i) {
		if (*s < '0' || *s > '9')
			return 0;
		for (v = 0; *s >= '0' && *s <= '9'; ++s)
			v = v * 10 + (unsigned long)(*s - '0');
		if (v > 255)
			return 0;
		a = a << 8 | v;
		if (i < 3 && *s++ != '.')
			return 0;
	}
	return *s ? 0 : a;
}

/* why a call failed: MacTCP's error e in words */
static void why(err, e)
char *err;
int e;
{
	char *s = 0;

	switch (e) {
	case openFailed:
	case connectionTerminated:
		s = "connection refused";
		break;
	case commandTimeout:
		s = "no answer";
		break;
	case E_DEAD:
		s = "host unreachable";
		break;
	case E_ROUTE:
		s = "network unreachable";
		break;
	case ipNoCnfgErr:
	case ipBadCnfgErr:
	case ipBadLapErr:
	case ipLoadErr:
		s = "MacTCP is not set up";
		break;
	case insufficientResources:
	case memFullErr:
		s = "not enough memory";
		break;
	}
	if (s)
		scopy(err, s, 60);
	else
		sprintf(err, "MacTCP error %d", e);
}

int n_open(host, port, err)
char *host, *err;
int port;
{
	static unsigned char name[] = { 4, '.', 'I', 'P', 'P' };
	unsigned long a;
	int e, i;

	if (!bye) {
		atexit(cleanup);
		bye = 1;
	}
	if (!ipp && OpenDriver(name, &ipp) != noErr) {
		ipp = 0;
		scopy(err, "no MacTCP", 60);
		return -1;
	}
	if (!rbuf && !(rbuf = NewPtr(RBUF))) {
		scopy(err, "not enough memory", 60);
		return -1;
	}
	a = dotted(host);
	if (!a) {
		if (!dnrok && dnr_open() != noErr) {
			scopy(err, "no name resolver (MacTCP)", 60);
			return -1;
		}
		dnrok = 1;
		if (dnr_lookup(host, &a, 60L * 60) != noErr) {
			scopy(err, "unknown host", 60);
			return -1;
		}
	}
	release();
	zero((char *)&pb, (int)sizeof pb);
	pb.csParam.create.rcvBuff = rbuf;
	pb.csParam.create.rcvBuffLen = RBUF;
	e = tdo(&pb, TCPCreate);
	if (e) {
		why(err, e);
		return -1;
	}
	strm = pb.tcpStream;
	if (!lport)		/* a start that differs from run to run */
		lport = PLOW + (long)(TickCount() % PNUM);
	for (i = 0; i < 8; ++i) {	/* a port that another stream has: the next */
		lport = lport + 1 < PLOW + PNUM ? lport + 1 : PLOW;
		zero((char *)&pb.csParam, (int)sizeof pb.csParam);
		pb.csParam.open.validityFlags = timeoutValue | timeoutAction;
		pb.csParam.open.ulpTimeoutValue = CWAIT;
		pb.csParam.open.ulpTimeoutAction = 1;	/* abort */
		pb.csParam.open.commandTimeoutValue = CWAIT;
		pb.csParam.open.remoteHost = a;
		pb.csParam.open.remotePort = (unsigned short)port;
		pb.csParam.open.localPort = (unsigned short)lport;
		e = tcall(&pb, TCPActiveOpen);
		while (!e && pb.ioResult > 0) {
			mac_wait(2L);
			if (ui_poll()) {
				release();
				scopy(err, "stopped", 60);
				return -1;
			}
		}
		if (!e)
			e = pb.ioResult;
		if (e != duplicateSocket)
			break;
	}
	if (e) {
		why(err, e);
		release();
		return -1;
	}
	return 1;
}

/* the stream now: 1 data waits (*amt bytes), 0 the server has closed
 * and all is read, -2 nothing yet, -1 error */
static int tstat(amt)
long *amt;
{
	int e, s;

	zero((char *)&xpb, (int)sizeof xpb);
	e = tdo(&xpb, TCPStatus);
	if (e)
		return e == connectionDoesntExist ? 0 : -1;
	*amt = (long)xpb.csParam.status.amtUnreadData;
	if (*amt > 0)
		return 1;
	s = xpb.csParam.status.connectionState;
	/* 0 closed, 14 close wait (the server's FIN), 16 closing, 18 last
	 * ack, 20 time wait */
	return s == 0 || s >= 14 ? 0 : -2;
}

/* wait at most 2 ticks (or not at all), and less when an event comes:
 * so the spinner turns, and a key or a click comes soon. The other
 * programs run meanwhile (mac_wait). */
int n_read(h, buf, n, wait)
int h, n, wait;
char *buf;
{
	long amt = 0;
	int r, e;

	if (!strm)
		return -1;
	r = tstat(&amt);
	if (r == -2 && wait) {
		mac_wait(2L);
		r = tstat(&amt);
	}
	if (r != 1)
		return r;
	if (amt > n)
		amt = n;
	zero((char *)&pb.csParam, (int)sizeof pb.csParam);
	pb.csParam.receive.commandTimeoutValue = 2;	/* the data is here */
	pb.csParam.receive.rcvBuff = buf;
	pb.csParam.receive.rcvBuffLen = (unsigned short)amt;
	e = tdo(&pb, TCPRcv);
	if (e)
		return e == connectionClosing ? 0 : -1;
	return (int)pb.csParam.receive.rcvBuffLen;
}

int n_write(h, buf, n)
int h, n;
char *buf;
{
	static wdsEntry wds[2];
	int k;

	if (!strm)
		return -1;
	while (n > 0) {
		k = n > 8192 ? 8192 : n;
		wds[0].length = (unsigned short)k;
		wds[0].ptr = buf;
		wds[1].length = 0;
		wds[1].ptr = 0;
		zero((char *)&pb.csParam, (int)sizeof pb.csParam);
		pb.csParam.send.validityFlags = timeoutValue | timeoutAction;
		pb.csParam.send.ulpTimeoutValue = CWAIT;
		pb.csParam.send.ulpTimeoutAction = 1;
		pb.csParam.send.pushFlag = 1;
		pb.csParam.send.wdsPtr = (Ptr)wds;
		if (tdo(&pb, TCPSend))
			return -1;
		buf += k;
		n -= k;
	}
	return 0;
}

/* close, waiting at most CTICKS for the server; then the stream goes */
void n_close(h)
int h;
{
	long t;

	if (!strm)
		return;
	zero((char *)&pb.csParam, (int)sizeof pb.csParam);
	pb.csParam.close.validityFlags = timeoutValue | timeoutAction;
	pb.csParam.close.ulpTimeoutValue = 5;
	pb.csParam.close.ulpTimeoutAction = 1;
	if (!tcall(&pb, TCPClose)) {
		t = (long)TickCount() + CTICKS;
		while (pb.ioResult > 0 && (long)TickCount() < t)
			mac_wait(1L);
	}
	release();
}
