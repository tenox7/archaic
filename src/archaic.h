/* archaic.h - Archaic, a very small web browser for old systems.
 *
 * It is a client of WRP, the Web Rendering Proxy
 * (github.com/tenox7/wrp): WRP runs a modern browser on a modern
 * machine and sends the page as a GIF picture with a click map. Archaic
 * shows WRP's form (text fields, buttons, menus), the GIF, and sends the
 * clicks back.
 *
 * The code is meant to build with very old C compilers too:
 * - K&R style function definitions; prototypes only through P(), so
 *   -DNOPROTO turns them off for a pre-ANSI compiler
 * - no const, enum, void *, struct assignment or structs passed by value
 * - parameters only int, long or pointers (no char, short or float)
 * - no arrays in local (auto) variables: Whitesmiths C on Idris gets
 *   a[n - 1] wrong for them. Make them static.
 * - plain char may be signed or unsigned: use unsigned char for bytes
 * - int may be 16 bits: use long for byte counts and pixel offsets
 * - only a few library calls: malloc, free, sprintf, printf, fprintf,
 *   getenv, time (and fopen, fwrite, fclose, fflush for the -t and -o
 *   test options). No string.h: the core has its own small helpers.
 *
 * One port = one display file (g_ functions) and one network file
 * (n_ functions) next to the portable core (main.c, html.c, http.c,
 * gif.c, pic.c). See AGENTS.md. The displays for modern systems (SDL, Cocoa)
 * may use newer C.
 */
#ifndef ARCHAIC_H
#define ARCHAIC_H

#include <stdio.h>

#ifdef NOPROTO
#define P(a) ()
#else
#define P(a) a
#endif

/* The version. The program carries it as "@(#)Archaic 0.10" (what(1)
 * shows it): WRP reads it in the files of the release, for the updates
 * (update.c). VERSION is the number alone. */
#ifndef IDENT
#define IDENT "@(#)Archaic 0.11"
#endif
extern char ident[];
#define VERSION (ident + 12)

/* WRP's address when none is given: WRP's default port. In the emulators
 * the host is 10.0.2.2 (slirp, QEMU's user network): Whitesmiths C
 * (_WSLAS) means Idris, gcc's unixpc the AT&T UNIX PC (3B1), SUNVIEW
 * (mksunview's build) SunOS 3 or 4 with SunView, __hpux HP-UX,
 * __DGUX__ DG/UX and _AIX AIX (WRP does not run there). */
#ifndef DEFURL
#ifdef _WSLAS
#define DEFURL "http://10.0.2.2:8080/"
#endif
#ifdef unixpc
#define DEFURL "http://10.0.2.2:8080/"
#endif
#ifdef u3b2
#define DEFURL "http://10.0.2.2:8080/"
#endif
#ifdef SUNVIEW
#define DEFURL "http://10.0.2.2:8080/"
#endif
#ifdef __hpux
#define DEFURL "http://10.0.2.2:8080/"
#endif
#ifdef __DGUX__
#define DEFURL "http://10.0.2.2:8080/"
#endif
#ifdef _AIX
#define DEFURL "http://10.0.2.2:8080/"
#endif
#ifndef DEFURL
#define DEFURL "http://127.0.0.1:8080/"
#endif
#endif

/* The system, for the updates (update.c): Archaic's file for it in the
 * GitHub release is archaic-NAME, and the program carries "@(#)Archaic
 * for NAME", which WRP checks. A build with no SYSID gets no updates:
 * macOS, Linux, a new port (cc -DSYSID='"@(#)Archaic for NAME"' gives
 * one). UPDPATH: where the program is put, if Archaic can't find its
 * file by its name and the search path. NORENAME: the system has no
 * rename(); NOUNISTD: no unistd.h (update.c includes it only for an
 * ANSI compiler, as the 3B1's gcc). */
#ifndef SYSID
#ifdef _WSLAS
#define SYSID "@(#)Archaic for idris"
#define UPDPATH "/usr/X/bin/archaic"
#define NORENAME
#define NOUNISTD
#endif
#ifdef unixpc
#define SYSID "@(#)Archaic for unixpc"
#define NORENAME
#define NOUNISTD
#endif
#ifdef u3b2
#define SYSID "@(#)Archaic for 3b2"
#define NORENAME
#define NOUNISTD
#endif
#ifdef SUNVIEW
#ifdef sparc
#define SYSID "@(#)Archaic for sun4"
#endif
#ifdef mc68020
#define SYSID "@(#)Archaic for sun3"
#endif
#endif
#ifdef __hpux
#define SYSID "@(#)Archaic for hpux9"
#endif
#ifdef __DGUX__
#define SYSID "@(#)Archaic for dgux"
#endif
#ifdef _AIX
#ifdef _IBMR2
#define SYSID "@(#)Archaic for aix3"
#endif
#ifdef ps2
#define SYSID "@(#)Archaic for aixps2"
#endif
#endif
#ifdef ultrix
#define SYSID "@(#)Archaic for ultrix4"
#endif
#ifndef UPDPATH
#define UPDPATH "/usr/local/bin/archaic"
#endif
#endif
#ifdef SYSID
extern char sysid[];		/* update.c: SYSID, */
#define SYSNAME (sysid + 16)	/*  and the name after "@(#)Archaic for " */
#endif

/* limits: all memory is static or comes from malloc once per image */
#define MAXITEM 600		/* words, fields, buttons, images on a page */
#define MAXOPT 120		/* <OPTION>s on a page */
#define MAXFORM 4		/* <FORM>s on a page */
#define MAXIMG 8		/* <IMG>s on a page */
#define POOLSZ 12000		/* text of a page */
#define MAXVAL 300		/* longest text field value */
#define MAXURL 1600		/* longest URL or form data */
#define MAXPAGE 30000		/* longest HTML page (bytes) */
#define MAXTITLE 80
#define MAXW 2048		/* widest image */

/* page items */
#define I_TEXT 1		/* a word */
#define I_BR 2			/* line break */
#define I_PARA 3		/* paragraph break */
#define I_FIELD 4		/* <INPUT TYPE=TEXT> or PASSWORD */
#define I_BUTTON 5		/* <INPUT TYPE=SUBMIT> */
#define I_HIDDEN 6		/* <INPUT TYPE=HIDDEN> */
#define I_SELECT 7		/* <SELECT> */
#define I_IMAGE 8		/* <IMG> */

typedef struct item {
	int type;
	int x, y, w, h;		/* where the layout put it */
	int sp;			/* white space before it */
	int form;		/* its <FORM>, or -1 */
	char *href;		/* the link it is in, or 0 */
	char *name;		/* field, button, select: NAME */
	char *text;		/* word, button label, image SRC */
	char *val;		/* field: value (MAXVAL bytes) */
	int size;		/* field: SIZE in characters */
	int pass;		/* field: TYPE=PASSWORD */
	int opt, nopt, sel;	/* select: first option, count, selected */
	int img;		/* image: index in imgs[], or -1 */
	int iw, ih;		/* image: WIDTH, HEIGHT */
	int ismap;		/* image: ISMAP */
} Item;

typedef struct opt {
	char *val;
	char *text;
	int dis;		/* DISABLED */
} Opt;

typedef struct form {
	char *action;
	int post;		/* METHOD=POST */
} Form;

typedef struct image {
	int w, h;		/* 0 until the GIF header has come */
	int bpl;		/* bytes in a row */
	int depth;		/* the pixels: 1 or 8 bits (set before pic_load) */
	unsigned char *bits;	/* rows of pixels. 1 bit: 1 = black, high bit
				 * left. 8 bits: a color number each */
	unsigned char *pal;	/* 8 bits: 256 colors, 3 bytes each (red,
				 * green, blue 0 to 255); 1 bit: 0 */
	int kind;		/* what it came as: P_GIF, P_ABM ... (pic_load) */
} Image;

/* html.c: the page */
extern Item items[];
extern int nitems;
extern Opt opts[];
extern int nopts;
extern Form forms[];
extern int nforms;
extern char title[];
int html_parse P((char *buf, int len));

/* http.c: URLs and HTTP/1.0 */
extern char http_url[];		/* where the answer came from, after redirects */
extern char http_type[];	/* Content-Type */
extern char http_err[];		/* why http_open failed */
extern long http_len;		/* Content-Length, -1 if none */
extern long http_got;		/* body bytes read so far */
extern int http_status;
extern char http_host[];	/* the server of the last http_open() */
extern int http_port;
extern int http_noconn;		/* 1: http_open could not connect (http_err: why) */
extern char *http_accept;	/* the request's Accept: what Archaic takes */
extern char *http_note;		/* a note for WRP's log, for the next request */
extern long http_fast;		/* X-Fast-Length: the body's size in the type
				 * that decodes fastest, when the server
				 * chose among more than one; else 0 */
int http_open P((char *url, char *post));
int http_getc P((void));
/* the body's bytes with no call for each: see http.c */
extern unsigned char *http_p, *http_e;
int http_more P((void));
void http_sync P((void));
#define HGETC() (http_p < http_e ? *http_p++ : http_more())
void http_close P((void));
void http_pump P((void));	/* take what the network has, without waiting */
long now P((void));		/* seconds */
long nowms P((long *res));	/* a clock in ms, off by *res at most */
void url_join P((char *base, char *ref, char *out));
void url_enc P((char *out, char *s));
int lc P((int c));
int strieq P((char *a, char *b));
int strnieq P((char *a, char *b, int n));
void scopy P((char *d, char *s, int max));

/* pic.c: a picture from the body. A GIF: the pixels as im->depth says
 * (set it before). WRP's ABM: 1 bit. 0 ok, -1 not a picture or broken */
int pic_load P((Image *im));
#define P_GIF 1			/* Image's kind: a GIF, */
#define P_ABM 2			/*  WRP's ABM, */
#define P_AB3 3			/*  ABM packed for a slow line (ABM3), */
#define P_AB8 4			/*  WRP's AB8 (colors); 0: not a picture */
void pic_rows P((Image *im, int y, int n));	/* http_sync(), then img_rows() */
void pic_fill P((unsigned char *p, long n, int c));	/* n bytes to c */
#define pic_wipe(p, n) pic_fill(p, n, 0)		/* n bytes to 0 */
/* n bytes from s to d, the first one first (d after s: they may overlap) */
void pic_copy P((unsigned char *d, unsigned char *s, long n));
/* gif.c: a GIF, after its first 2 bytes "GI" */
int gif_load P((Image *im));

/* main.c: called from the other files */
extern int stop;		/* set to end the load that is running */
int ui_poll P((void));		/* handle events while the network waits */
void img_rows P((Image *im, int y, int n));	/* rows y.. y+n-1 are new */
void say P((char *s));		/* status line */
void ui_note P((int phase));	/* http.c: what http_open() does now, */
#define H_CONNECT 1		/*  connecting to http_host, */
#define H_WAIT 2		/*  waiting for the answer, */
#define H_BODY 3		/*  the headers have come; 0: no load */
void ui_what P((char *s));	/* update.c: what comes ("": show nothing), */
void ui_prog P((void));		/*  and how much of it has come */

/* update.c: ask WRP for the newest Archaic, install it if it can */
int upd_run P((char *wrp, char *prog));
extern char upd_msg[];		/* what it did, or why not */
#define U_NONE 0		/* this is the newest */
#define U_ERR 1			/* no answer, or WRP has no file for this system */
#define U_OUT 2			/* a newer one is out: Archaic can't install it */
#define U_DONE 3		/* installed, for the next start */
#define U_FAIL 4		/* the install failed */
#define U_STOP 5		/* stopped */

/* events from the display */
#define E_NONE 0
#define E_KEY 1			/* key: character in key */
#define E_DOWN 2		/* mouse button key (1 left 2 middle 3 right) at x, y */
#define E_EXPOSE 3		/* draw x, y, w, h again */
#define E_SIZE 4		/* the window is now w x h */
#define E_QUIT 5		/* the window was closed */
#define E_SCROLL 6		/* scroll y units of key, down if y > 0: */
#define S_PIXEL 0		/*  pixels (a trackpad) */
#define S_STEP 1		/*  a quarter of the window (the wheel, the arrows) */
#define S_PAGE 2		/*  a window less 2 lines (Page Up, Page Down) */
#define S_TO 3			/*  to place y of the page (0 up to g_sbar's max) */
#define E_UP 7			/* mouse button key let go at x, y */
#define E_MOVE 8		/* the mouse moved to x, y with a button down */

typedef struct event {
	int type;
	int x, y, w, h;
	int key;
} Event;

/* the display: one of x10.c, x11.c, unixpc.c, sunview.c, sdl.c, cocoa.m ...
 * Colors: 0 white, 1 black. x, y are the top left corner, also for
 * text. sdl.c and cocoa.m draw with fb.c (a screen in memory). g_fd:
 * the display's file, for the network's select(); -1 if it has none.
 * g_colors: how many colors the display shows, 2 if only black and
 * white (1L << 24 for true color). A display with more than 2 draws the
 * pictures' colors with g_pix: rows of 8-bit color numbers, and pal,
 * the red, green and blue (0 to 255) of each of the 256. Archaic calls
 * g_pix only if g_colors is more than 2.
 * g_ms: a clock in milliseconds, from any start (for the spinner); -1
 * if the system has none: the spinner then steps each time the network
 * waits (n_read).
 * g_event: wait 0 = don't wait, 1 = wait for an event, 2 = wait at most
 * about 1/10 s (the repeat of Archaic's scroll bar); a display that
 * can't wait for a time waits as for 1.
 * g_copy: move the part x, y, w, h of the window dy rows down (up if
 * dy < 0), for a scroll: Archaic then draws only the rows that come in.
 * 0 if it did not (a part of the window was covered, or the display
 * can't): Archaic then draws all of it again.
 *
 * The scroll bar: a system that has scroll bars (Cocoa, SunView, Win32,
 * the 3B1's window frame) uses its own, and sets g_bar to 0. Archaic then calls
 * g_sbar when the page moves: the page is at place pos, 0 up to max, and
 * vis pixels of it show. The display sends E_SCROLL for its scroll bar.
 * A system with none (SDL, X11 with no toolkit, X10) sets g_bar to 1:
 * Archaic draws its own at the right of the window, in the style of the
 * Atari ST's GEM, and uses E_UP and E_MOVE to drag it. */
/* REMOTE (dmd.c, a terminal on a slow line): the display keeps the
 * slices of the page's picture itself. g_slice passes the body that
 * comes on to it, for its slot s (the slice at page row y0), and fills
 * im's size and kind: 0 when the display has all of it, -1 if it was
 * stopped or came short, -2 if the display can't take this picture type,
 * -3 if it has no memory for it. g_view: the
 * frame is at x, y, w x h and its top row is page row oy: the display
 * draws its rows a up to b - 1 again. g_drop: slot s is free. g_spin:
 * the display turns the spinner at x, y itself (on), or takes it away.
 * g_gauge: for the next g_slice, the display shows how much of it has
 * come in a bar at x, y, w x h. g_hole: the display shows no rows of a
 * slice that comes at x, y, w x h (an open list is there); w 0: none. */
#ifdef REMOTE
int g_slice P((int s, long y0, Image *im));
void g_view P((int x, int y, int w, int h, long oy, int a, int b));
void g_drop P((int s));
void g_spin P((int on, int x, int y));
void g_gauge P((int x, int y, int w, int h));
void g_hole P((int x, int y, int w, int h));
#endif
extern int g_bar;
void g_sbar P((long pos, long max, long vis));
int g_open P((char *geom, int *w, int *h));
void g_close P((void));
void g_title P((char *s));
void g_fill P((int x, int y, int w, int h, int c));
void g_invert P((int x, int y, int w, int h));
void g_text P((int x, int y, char *s, int n, int inv));
int g_textw P((char *s, int n));
void g_font P((int *height, int *base));
void g_bits P((int x, int y, int w, int h, unsigned char *bits, int bpl));
long g_colors P((void));
long g_ms P((void));
void g_pix P((int x, int y, int w, int h, unsigned char *pix, int bpl,
	unsigned char *pal));
int g_copy P((int x, int y, int w, int h, int dy));
void g_flush P((void));
int g_event P((Event *e, int wait));
int g_fd P((void));
/* The Mac (mac.c) has cooperative multitasking: Archaic must give the
 * other programs their time. mac_wait: they run, at most t ticks (less
 * when an event comes for Archaic: it goes to g_event's queue).
 * YIELD in long work (img_rows: a picture decodes): mac_wait(0) each
 * 1/15 s. Elsewhere YIELD is nothing. */
#ifdef macintosh
void mac_wait P((long t));
void mac_yield P((void));
#define YIELD mac_yield()
#else
#define YIELD
#endif

/* the network: one of nidris.c (Idris netd), nsock.c (BSD sockets), ...
 * n_read: up to n bytes. wait 1: wait a short time (nsock.c 1/20 s,
 * ntwg.c 1/10 s, nidris.c 1 s), and less when the display has input;
 * wait 0: don't wait. Returns the number of
 * bytes, -2 = nothing yet, 0 = the end, -1 = error. n_open may call
 * ui_poll() while it waits, and gives up when that returns 1. */
int n_open P((char *host, int port, char *err));
int n_read P((int h, char *buf, int n, int wait));
int n_write P((int h, char *buf, int n));
void n_close P((int h));

#endif
