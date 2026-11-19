/* fb.h - a screen in memory, for the displays that show the pixels
 * themselves (sdl.c, cocoa.m, headless.c). fb.c draws into it: g_fill, g_invert,
 * g_text, g_textw, g_font and g_bits. The display shows it in g_flush.
 */
#include "archaic.h"

#define FB_WHITE 0xffffff
#define FB_BLACK 0

extern unsigned int *fb;	/* fbw x fbh pixels, 0xRRGGBB, row by row */
extern int fbw, fbh;
extern int fbx0, fby0, fbx1, fby1;	/* changed since fb_done(): x0 <= x < x1 */

int fb_size P((int w, int h));	/* a new size, all white: 0 ok, -1 no memory */
void fb_done P((void));		/* the display has shown the changes */
int fb_geom P((char *g, int *w, int *h, int *x, int *y));	/* =WxH+X+Y */
int fb_shot P((char *file));	/* write the screen: PNG, or PBM for *.pbm */
