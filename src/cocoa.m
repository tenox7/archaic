/* cocoa.m - Archaic's display for macOS (Cocoa, Mac OS X 10.6 and
 * later): a window that shows fb.c's screen in memory. On a Retina
 * screen each pixel is 2x2, with no smoothing.
 *
 * Built without ARC: retain and release by hand, as in old Cocoa.
 * Keys: the characters as typed; Return, Backspace, Tab, Esc and
 * Ctrl+letter. Page Up and Page Down scroll a window, the up and down
 * arrows and the wheel's steps a quarter of it (E_SCROLL); a trackpad
 * scrolls by pixels. Ctrl+click is the right button. Cmd-Q, Cmd-W and
 * the close button quit. Cocoa has no file to wait on: g_fd() is -1.
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
#endif

#define NQ 64
static Event evq[NQ];		/* events from Cocoa, not given yet */
static int qin, qout;
static NSWindow *win;
static NSView *view;
static id del;
static CGContextRef bm;		/* fb as a Quartz bitmap */
static CGColorSpaceRef rgb;
static double wheel;		/* wheel steps or trackpad pixels not used yet */

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
	r = NSMakeRect(0, 0, gw, gh);
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
	view = [[ArchaicView alloc] initWithFrame:r];
	[win setContentView:view];
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

void g_close()
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	[win setDelegate:nil];
	[win close];
	[win release];
	[view release];
	win = nil;
	view = nil;
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
	[view displayIfNeeded];
	[CATransaction flush];
	[pool release];
}

int g_fd()
{
	return -1;
}

int g_event(e, wait)
Event *e;
int wait;
{
	NSAutoreleasePool *pool;
	NSEvent *ev;

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
		pool = [[NSAutoreleasePool alloc] init];
		ev = [NSApp nextEventMatchingMask:NSEventMaskAny
			untilDate:wait ? [NSDate distantFuture] : [NSDate distantPast]
			inMode:NSDefaultRunLoopMode dequeue:YES];
		if (ev)
			[NSApp sendEvent:ev];
		[pool release];
		if (!ev && qout == qin)
			return 0;
	}
}
