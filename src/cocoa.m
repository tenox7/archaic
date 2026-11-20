/* cocoa.m - Archaic's display for macOS (Cocoa, Mac OS X 10.6 and
 * later): a window that shows fb.c's screen in memory. On a Retina
 * screen each pixel is 2x2, with no smoothing. The colors: as many as
 * the screen shows (its depth).
 *
 * Built without ARC: retain and release by hand, as in old Cocoa.
 * Keys: the characters as typed; Return, Backspace, Tab, Esc and
 * Ctrl+letter. Page Up and Page Down scroll a window, the up and down
 * arrows and the wheel's steps a quarter of it (E_SCROLL); a trackpad
 * scrolls by pixels. Ctrl+click is the right button. Cmd-Q, Cmd-W and
 * the close button quit. Cocoa has no file to wait on: g_fd() is -1.
 *
 * The scroll bar is the Mac's own: an NSScroller at the right of the
 * window, always shown (the legacy style, not the overlay one). So g_bar
 * is 0, and g_sbar sets the knob. A click or a drag on it is worked here,
 * not in NSScroller's own loop, which would hold up Archaic until the
 * button is let go: so the page moves, and its slices come, while the
 * knob moves. A click in the slot scrolls a window (or jumps there, as
 * set in System Settings; Option the other way); held down, it repeats.
 * A clear view over the scroll bar takes its mouse, so the scroll bar is
 * a plain NSScroller: AppKit draws a subclass of it in an older style,
 * thinner, and pale (as if off) under the pointer. AppKit sends the
 * mouse to the view, so the window's corner still resizes it.
 */
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#include <string.h>
#include "fb.h"

#if !defined(MAC_OS_X_VERSION_10_12) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_12
#define NSWindowStyleMaskTitled NSTitledWindowMask
#define NSWindowStyleMaskClosable NSClosableWindowMask
#define NSWindowStyleMaskMiniaturizable NSMiniaturizableWindowMask
#define NSWindowStyleMaskResizable NSResizableWindowMask
#define NSEventMaskAny NSAnyEventMask
#define NSEventModifierFlagCommand NSCommandKeyMask
#define NSEventModifierFlagControl NSControlKeyMask
#define NSEventModifierFlagOption NSAlternateKeyMask
#define NSControlSizeRegular NSRegularControlSize
#endif
#define SB_UPLINE 4		/* NSScrollerDecrementLine: Mac OS X 10.6's arrows */
#define SB_DNLINE 5		/* NSScrollerIncrementLine */

#define NQ 64
static Event evq[NQ];		/* events from Cocoa, not given yet */
static int qin, qout;
static NSWindow *win;
static NSView *view;
static id del;
static CGContextRef bm;		/* fb as a Quartz bitmap */
static CGColorSpaceRef rgb;
static double wheel;		/* wheel steps or trackpad pixels not used yet */
static NSScroller *sbar;	/* the scroll bar */
static long smax;		/* the page goes 0 up to smax (g_sbar) */
static int spart;		/* the scroll bar's part held down, 0 none */
static double sgrab;		/* the knob: the pointer's place in it */
static NSPoint spt;		/* the pointer, in the scroll bar */
static NSPoint swin;		/* the pointer, in the window (for testPart:) */
static double srep;		/* when a held part does it again */
static int sheld;		/* the button went down on the scroll bar */
static NSView *sgrip;		/* the clear view over it */

int g_bar = 0;

static void put(type, x, y, k)
int type, x, y, k;
{
	Event *e;

	if ((qin + 1) % NQ == qout)
		return;
	e = &evq[qin];
	e->type = type;
	e->x = x;
	e->y = y;
	e->w = fbw;
	e->h = fbh;
	e->key = k;
	qin = (qin + 1) % NQ;
}

/* scroll y units (S_PIXEL, S_STEP, S_PAGE), down if y > 0: added to
 * the last event if that is a scroll of the same units not taken yet */
static void scrl(y, unit)
int y, unit;
{
	Event *e;

	if (qin != qout) {
		e = &evq[(qin + NQ - 1) % NQ];
		if (e->type == E_SCROLL && e->key == unit) {
			e->y += y;
			return;
		}
	}
	put(E_SCROLL, 0, y, unit);
}

/* the knob's place for the pointer at p: 0 at the top, 1 at the end */
static double knobat(p)
NSPoint p;
{
	NSRect slot = [sbar rectForPart:NSScrollerKnobSlot];
	NSRect knob = [sbar rectForPart:NSScrollerKnob];
	double room = slot.size.height - knob.size.height, d;

	if (room <= 0)
		return [sbar doubleValue];
	d = [sbar isFlipped] ? p.y - slot.origin.y : NSMaxY(slot) - p.y;
	d = (d - sgrab) / room;
	return d < 0 ? 0 : d > 1 ? 1 : d;
}

/* the page to place v of it (0 up to 1): a queued S_TO is moved */
static void sto(v)
double v;
{
	Event *e;
	int y = (int)(v * smax + 0.5);

	[sbar setDoubleValue:v];
	if (qin != qout) {
		e = &evq[(qin + NQ - 1) % NQ];
		if (e->type == E_SCROLL && e->key == S_TO) {
			e->y = y;
			return;
		}
	}
	put(E_SCROLL, 0, y, S_TO);
}

/* what part p of the scroll bar does, once */
static void sstep(p)
int p;
{
	if (p == NSScrollerDecrementPage)
		scrl(-1, S_PAGE);
	else if (p == NSScrollerIncrementPage)
		scrl(1, S_PAGE);
	else if (p == SB_UPLINE)
		scrl(-1, S_STEP);
	else if (p == SB_DNLINE)
		scrl(1, S_STEP);
}

/* a part held down that does it again (not the knob) */
static int srepeats()
{
	return spart && spart != NSScrollerKnob;
}

/* the button down on the scroll bar */
static void sdown(ev)
NSEvent *ev;
{
	NSRect knob;
	int part, jump;

	spart = 0;
	if (![sbar isEnabled])
		return;
	swin = [ev locationInWindow];
	spt = [sbar convertPoint:swin fromView:nil];
	part = (int)[sbar testPart:swin];	/* testPart: takes window points */
	if (part == NSScrollerDecrementPage || part == NSScrollerIncrementPage) {
		jump = [[NSUserDefaults standardUserDefaults]
			boolForKey:@"AppleScrollerPagingBehavior"];
		if ([ev modifierFlags] & NSEventModifierFlagOption)
			jump = !jump;
		if (jump) {		/* the knob's middle to the pointer */
			sgrab = [sbar rectForPart:NSScrollerKnob].size.height / 2;
			spart = NSScrollerKnob;
			sto(knobat(spt));
			return;
		}
	}
	if (part == NSScrollerKnob) {
		knob = [sbar rectForPart:NSScrollerKnob];
		sgrab = [sbar isFlipped] ? spt.y - knob.origin.y :
			NSMaxY(knob) - spt.y;
	} else if (part == NSScrollerDecrementPage ||
		part == NSScrollerIncrementPage || part == SB_UPLINE ||
		part == SB_DNLINE) {
		sstep(part);
		srep = [NSDate timeIntervalSinceReferenceDate] + 0.4;
	} else
		return;
	spart = part;
}

/* the button moved, or let go (up), after sdown */
static void sdrag(ev, up)
NSEvent *ev;
int up;
{
	if (!sheld)
		return;
	swin = [ev locationInWindow];
	spt = [sbar convertPoint:swin fromView:nil];
	if (spart == NSScrollerKnob)
		sto(knobat(spt));
	if (up)
		sheld = spart = 0;
}

static int mkbm()
{
	if (bm)
		CGContextRelease(bm);
	bm = CGBitmapContextCreate(fb, fbw, fbh, 8, fbw * sizeof *fb, rgb,
		kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host);
	return bm ? 0 : -1;
}

/* the window has a new size: a new screen in memory */
static void resized()
{
	NSRect r = [view bounds];
	int w = (int)r.size.width, h = (int)r.size.height;

	if (w == fbw && h == fbh)
		return;
	if (bm)
		CGContextRelease(bm);
	bm = 0;
	if (fb_size(w, h) < 0 || mkbm() < 0) {
		put(E_QUIT, 0, 0, 0);
		return;
	}
	put(E_SIZE, 0, 0, 0);
}

@interface ArchaicView : NSView
@end

@implementation ArchaicView
- (BOOL)isFlipped { return YES; }
- (BOOL)isOpaque { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }

- (void)drawRect:(NSRect)r
{
	CGContextRef c = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
	CGImageRef im;

	[[NSColor whiteColor] set];
	NSRectFill(r);
	if (!bm)
		return;
	im = CGBitmapContextCreateImage(bm);
	if (!im)
		return;
	/* the view is upside down (flipped): turn the picture too */
	CGContextSaveGState(c);
	CGContextSetInterpolationQuality(c, kCGInterpolationNone);
	CGContextTranslateCTM(c, 0, fbh);
	CGContextScaleCTM(c, 1, -1);
	CGContextDrawImage(c, CGRectMake(0, 0, fbw, fbh), im);
	CGContextRestoreGState(c);
	CGImageRelease(im);
}

- (void)keyDown:(NSEvent *)ev
{
	NSString *s = [ev characters];
	NSUInteger i;
	int c;

	if ([ev modifierFlags] & NSEventModifierFlagCommand)
		return;
	for (i = 0; i < [s length]; ++i) {
		c = [s characterAtIndex:i];
		switch (c) {
		case NSPageUpFunctionKey:
			scrl(-1, S_PAGE);
			continue;
		case NSPageDownFunctionKey:
			scrl(1, S_PAGE);
			continue;
		case NSUpArrowFunctionKey:
			scrl(-1, S_STEP);
			continue;
		case NSDownArrowFunctionKey:
			scrl(1, S_STEP);
			continue;
		case 3:			/* Enter on the keypad */
			c = '\r';
			break;
		case NSDeleteFunctionKey:
			c = 0177;
			break;
		}
		if (c < 0400)
			put(E_KEY, 0, 0, c);
	}
}

- (void)click:(NSEvent *)ev button:(int)b
{
	NSPoint p = [self convertPoint:[ev locationInWindow] fromView:nil];

	put(E_DOWN, (int)p.x, (int)p.y, b);
}

- (void)mouseDown:(NSEvent *)ev
{
	[self click:ev button:([ev modifierFlags] & NSEventModifierFlagControl) ? 3 : 1];
}

- (void)rightMouseDown:(NSEvent *)ev { [self click:ev button:3]; }
- (void)otherMouseDown:(NSEvent *)ev { [self click:ev button:2]; }

/* a wheel: a quarter of the window a step; a trackpad: pixels */
- (void)scrollWheel:(NSEvent *)ev
{
	int n, unit = S_STEP;

#if defined(MAC_OS_X_VERSION_10_7) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
	if ([ev respondsToSelector:@selector(hasPreciseScrollingDeltas)] &&
		[ev hasPreciseScrollingDeltas]) {
		wheel += [ev scrollingDeltaY];
		unit = S_PIXEL;
	} else
#endif
		wheel += [ev deltaY];
	n = (int)wheel;
	wheel -= n;
	if (n)
		scrl(-n, unit);
}
@end

/* the clear view over the scroll bar: its mouse */
@interface ArchaicGrip : NSView
@end

@implementation ArchaicGrip
- (BOOL)acceptsFirstMouse:(NSEvent *)ev { return YES; }
- (void)mouseDown:(NSEvent *)ev { sheld = 1; sdown(ev); }
- (void)mouseDragged:(NSEvent *)ev { sdrag(ev, 0); }
- (void)mouseUp:(NSEvent *)ev { sdrag(ev, 1); }
- (void)scrollWheel:(NSEvent *)ev { [view scrollWheel:ev]; }
@end

@interface ArchaicDelegate : NSObject
@end

@implementation ArchaicDelegate
- (BOOL)windowShouldClose:(id)w
{
	put(E_QUIT, 0, 0, 0);
	return NO;
}

- (void)windowDidResize:(NSNotification *)n { resized(); }
- (void)quit:(id)sender { put(E_QUIT, 0, 0, 0); }
@end

/* the menu bar: the application menu with Quit (Cmd-Q) and Close (Cmd-W) */
static void menus()
{
	NSMenu *bar = [[NSMenu alloc] init], *m = [[NSMenu alloc] init];
	NSMenuItem *app = [[NSMenuItem alloc] init], *it;

	it = [[NSMenuItem alloc] initWithTitle:@"Close" action:@selector(quit:)
		keyEquivalent:@"w"];
	[it setTarget:del];
	[m addItem:it];
	[it release];
	it = [[NSMenuItem alloc] initWithTitle:@"Quit Archaic"
		action:@selector(quit:) keyEquivalent:@"q"];
	[it setTarget:del];
	[m addItem:it];
	[it release];
	[app setSubmenu:m];
	[m release];
	[bar addItem:app];
	[app release];
	[NSApp setMainMenu:bar];
	[bar release];
}

int g_open(geom, w, h)
char *geom;
int *w, *h;
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSRect r, vis, fr;
	NSView *box;
	CGFloat sw = 15;		/* the scroll bar's width */
	int gw = 800, gh = 600, x = 0, y = 0, f;

	[NSApplication sharedApplication];
	if ([NSApp respondsToSelector:@selector(setActivationPolicy:)])
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
	del = [[ArchaicDelegate alloc] init];
	menus();
	[NSApp finishLaunching];

	f = fb_geom(geom, &gw, &gh, &x, &y);
	if (gw < 200)
		gw = 200;
	if (gh < 100)
		gh = 100;
#if defined(MAC_OS_X_VERSION_10_7) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
	if ([NSScroller respondsToSelector:
		@selector(scrollerWidthForControlSize:scrollerStyle:)])
		sw = [NSScroller scrollerWidthForControlSize:NSControlSizeRegular
			scrollerStyle:NSScrollerStyleLegacy];
#endif
	r = NSMakeRect(0, 0, gw + sw, gh);
	win = [[NSWindow alloc] initWithContentRect:r
		styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
			NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
		backing:NSBackingStoreBuffered defer:NO];
	if (!win) {
		[pool release];
		return -1;
	}
	[win setReleasedWhenClosed:NO];
	[win setTitle:@"Archaic"];
	if (f & 2) {
		/* X11's place: from the top left (or right, or bottom) */
		vis = [[NSScreen mainScreen] visibleFrame];
		fr = [win frame];
		[win setFrameTopLeftPoint:NSMakePoint(
			f & 4 ? NSMaxX(vis) - fr.size.width - x : NSMinX(vis) + x,
			f & 8 ? NSMinY(vis) + y + fr.size.height : NSMaxY(vis) - y)];
	} else
		[win center];
	/* the page (fb), and the scroll bar at its right */
	box = [[NSView alloc] initWithFrame:r];
	view = [[ArchaicView alloc] initWithFrame:NSMakeRect(0, 0, gw, gh)];
	[view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	sbar = [[NSScroller alloc] initWithFrame:NSMakeRect(gw, 0, sw, gh)];
	[sbar setAutoresizingMask:NSViewMinXMargin | NSViewHeightSizable];
#if defined(MAC_OS_X_VERSION_10_7) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
	if ([sbar respondsToSelector:@selector(setScrollerStyle:)])
		[sbar setScrollerStyle:NSScrollerStyleLegacy];
#endif
	[sbar setEnabled:NO];
	sgrip = [[ArchaicGrip alloc] initWithFrame:[sbar frame]];
	[sgrip setAutoresizingMask:NSViewMinXMargin | NSViewHeightSizable];
	[box addSubview:view];
	[box addSubview:sbar];
	[box addSubview:sgrip];
	[win setContentView:box];
	[box release];
	[win setDelegate:del];
	[win makeFirstResponder:view];
	rgb = CGColorSpaceCreateDeviceRGB();
	if (fb_size(gw, gh) < 0 || mkbm() < 0) {
		[pool release];
		return -1;
	}
	[win makeKeyAndOrderFront:nil];
	[NSApp activateIgnoringOtherApps:YES];
	[pool release];
	*w = gw;
	*h = gh;
	return 0;
}

/* the screen's colors: 2 to the bits of its pixels (of gray, or of
 * all three colors) */
long g_colors()
{
	NSScreen *sc = [win screen] ? [win screen] : [NSScreen mainScreen];
	NSWindowDepth d = [sc depth];
	long b = NSBitsPerPixelFromDepth(d);

	if (NSNumberOfColorComponents(NSColorSpaceFromDepth(d)) < 3)
		b = NSBitsPerSampleFromDepth(d);
	else if (b > 24)
		b = 3 * NSBitsPerSampleFromDepth(d);
	return b < 1 ? 2L : b > 24 ? 1L << 24 : 1L << b;
}

void g_close()
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	[win setDelegate:nil];
	[win close];
	[win release];
	[view release];
	[sbar release];
	[sgrip release];
	win = nil;
	view = nil;
	sbar = nil;
	sgrip = nil;
	if (bm)
		CGContextRelease(bm);
	bm = 0;
	[pool release];
}

/* the title is ISO 8859-1 */
void g_title(s)
char *s;
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSString *t = [[NSString alloc] initWithBytes:s length:strlen(s)
		encoding:NSISOLatin1StringEncoding];

	if (t) {
		[win setTitle:t];
		[t release];
	}
	[pool release];
}

void g_flush()
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	if (fbx0 < fbx1) {
		[view setNeedsDisplayInRect:NSMakeRect(fbx0, fby0,
			fbx1 - fbx0, fby1 - fby0)];
		fb_done();
	}
	[[win contentView] displayIfNeeded];
	[CATransaction flush];
	[pool release];
}

int g_fd()
{
	return -1;
}

/* a clock in milliseconds (for the spinner) */
long g_ms()
{
	return (long)([[NSProcessInfo processInfo] systemUptime] * 1000.0);
}

/* the page is at pos, 0 up to max; vis pixels of it show */
void g_sbar(pos, max, vis)
long pos, max, vis;
{
	NSAutoreleasePool *pool;

	smax = max;
	if (!sbar)
		return;
	pool = [[NSAutoreleasePool alloc] init];
	if (max <= 0) {
		[sbar setEnabled:NO];
		[sbar setDoubleValue:0];
		[sbar setKnobProportion:1];
	} else {
		[sbar setEnabled:YES];
		[sbar setDoubleValue:(double)pos / max];
		[sbar setKnobProportion:(double)vis / (vis + max)];
	}
	[pool release];
}

int g_event(e, wait)
Event *e;
int wait;
{
	NSAutoreleasePool *pool;
	NSEvent *ev;
	NSDate *until;
	double t, u;

	for (;;) {
		if (qout != qin) {
			e->type = evq[qout].type;
			e->x = evq[qout].x;
			e->y = evq[qout].y;
			e->w = evq[qout].w;
			e->h = evq[qout].h;
			e->key = evq[qout].key;
			qout = (qout + 1) % NQ;
			return 1;
		}
		/* a part of the scroll bar held down: again every 1/10 s,
		 * while the pointer stays on it */
		t = [NSDate timeIntervalSinceReferenceDate];
		if (srepeats() && !([NSEvent pressedMouseButtons] & 1))
			spart = 0;	/* its mouseUp went elsewhere */
		if (srepeats() && t >= srep) {
			srep = t + 0.1;
			if ((int)[sbar testPart:swin] == spart)
				sstep(spart);
			continue;
		}
		u = !wait ? 0 : wait == 2 ? t + 0.1 : -1;
		if (srepeats() && (u < 0 || srep < u))
			u = srep;
		pool = [[NSAutoreleasePool alloc] init];
		until = u < 0 ? [NSDate distantFuture] : u == 0 ?
			[NSDate distantPast] :
			[NSDate dateWithTimeIntervalSinceReferenceDate:u];
		ev = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:until
			inMode:NSDefaultRunLoopMode dequeue:YES];
		if (ev)
			[NSApp sendEvent:ev];
		[pool release];
		if (!ev && qout == qin && !(srepeats() &&
			[NSDate timeIntervalSinceReferenceDate] >= srep))
			return 0;
	}
}
