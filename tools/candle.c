/* candle - draws Archaic's logo: a candle, 1 bit a pixel.
 *
 * Usage: candle [-s size] [-p prefix] > src/candle.h
 *
 * Frame 0 is the candle with no flame. Frames 1 to 8 are the candle
 * lit, the flame moving left and right: Archaic shows them while it
 * loads, a frame each step of the spinner.
 *
 * The drawing is in a space of 32 x 32. It is drawn 8 times as large,
 * then made small: a pixel is black if more than half of its 8 x 8 is.
 * The wax, the wick and the middle of the dish are on whole pixels, so
 * that they stay sharp when small. -s gives the size in pixels: default
 * 20, which makes a candle 19 pixels high, as WRP's buttons in Archaic
 * (32: the size of the design). The table keeps only the
 * black part of the frames: the columns and rows that are black in one
 * of them. -p also writes each frame so, as a PBM file: prefix0.pbm ...
 * prefix8.pbm.
 *
 * Writes the frames as a C table on standard output. A host tool:
 *	cc -o candle tools/candle.c -lm
 *	./candle > src/candle.h
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SS 8		/* supersampling */
#define MAXS 64
#define FRAMES 9	/* 0: no flame; 1 to 8: lit */

/* the candle */
#define TOP 17.0	/* the top of the wax */
#define WAXW 7.0	/* the wax's width */
#define FLR 4.4		/* the radius of the flame's round base */

/* the flame's key frames: the tip's sway, more height, the base wider,
 * the body's lean */
static double keys[8][4] = {
	{ 0, 0, 0, 0 }, { 2.2, 1.5, 0.4, 0.6 }, { 3.2, -0.5, -0.3, 1.0 },
	{ 1.0, 2.2, 0.2, 0.3 }, { -1.5, 0.8, -0.2, -0.5 },
	{ -3.0, -0.8, 0.4, -1.0 }, { -1.8, 1.8, -0.1, -0.6 },
	{ 0.6, -1.2, 0.3, 0.2 }
};

#define SIZE 20		/* the size Archaic uses */

static int size = SIZE, big;
static unsigned char cv[MAXS * SS][MAXS * SS];	/* 1 black, 0 white */

/* a point of the 32 x 32 space on the big canvas. Less half a big pixel:
 * the tests take a big pixel's corner, and so they get its middle. */
static double P(double v)
{
	return v * SS * size / 32.0 - 0.5;
}

static void rect(double x0, double y0, double x1, double y1, int c)
{
	int x, y;

	x0 = P(x0), y0 = P(y0), x1 = P(x1), y1 = P(y1);
	for (y = 0; y < big; ++y)
		for (x = 0; x < big; ++x)
			if (x >= x0 && x <= x1 && y >= y0 && y <= y1)
				cv[y][x] = c;
}

/* the ellipse in the box x0, y0, x1, y1 */
static void ellipse(double x0, double y0, double x1, double y1, int c)
{
	double cx, cy, rx, ry, dx, dy;
	int x, y;

	x0 = P(x0), y0 = P(y0), x1 = P(x1), y1 = P(y1);
	cx = (x0 + x1) / 2, cy = (y0 + y1) / 2;
	rx = (x1 - x0) / 2 + 0.25, ry = (y1 - y0) / 2 + 0.25;
	for (y = 0; y < big; ++y)
		for (x = 0; x < big; ++x) {
			dx = (x - cx) / rx, dy = (y - cy) / ry;
			if (dx * dx + dy * dy <= 1)
				cv[y][x] = c;
		}
}

/* a polygon of n points (pt: x, y, x, y ...), with its edges */
static void poly(double *pt, int n, int c)
{
	static double q[16];
	double xa, ya, xb, yb, t;
	int x, y, i, j, in;

	for (i = 0; i < 2 * n; ++i)
		q[i] = P(pt[i]);
	for (y = 0; y < big; ++y)
		for (x = 0; x < big; ++x) {
			in = 0;
			for (i = 0, j = n - 1; i < n; j = i++) {
				xa = q[2 * i], ya = q[2 * i + 1];
				xb = q[2 * j], yb = q[2 * j + 1];
				if ((ya > y) != (yb > y) &&
					x < (xb - xa) * (y - ya) / (yb - ya) + xa)
					in = !in;
				/* on the edge, within half a big pixel */
				t = (xb - xa) * (xb - xa) + (yb - ya) * (yb - ya);
				if (t > 0) {
					t = ((x - xa) * (xb - xa) + (y - ya) * (yb - ya)) / t;
					if (t >= 0 && t <= 1 &&
						fabs((x - xa - t * (xb - xa))) <= 0.5 &&
						fabs((y - ya - t * (yb - ya))) <= 0.5)
						in = 2;
				}
				if (in == 2)
					break;
			}
			if (in)
				cv[y][x] = c;
		}
}

static void tri(double ax, double ay, double bx, double by, double cx,
	double cy, int c)
{
	double p[6];

	p[0] = ax, p[1] = ay, p[2] = bx, p[3] = by, p[4] = cx, p[5] = cy;
	poly(p, 3, c);
}

/* the flame over cx, the bottom of its round base at y */
static void flame(double cx, double y, double r, int k)
{
	double sway = keys[k][0], tall = keys[k][1], squash = keys[k][2];
	double lean = keys[k][3], s = r / 5, bx = cx + lean * s;
	double cy = y - r, ri = r * 0.48;
	double ix = bx + lean * s * 0.3, iy = cy + r * 0.25;

	/* the flame: a round base and a pointed tip */
	ellipse(bx - r - squash * s, cy - r, bx + r + squash * s, cy + r, 1);
	tri(bx - r * 0.97, cy - r * 0.15, bx + r * 0.97, cy - r * 0.15,
		cx + sway * s * 1.4, cy - r * 2.1 - tall * s, 1);
	/* the white flame inside it */
	ellipse(ix - ri, iy - ri, ix + ri, iy + ri, 0);
	tri(ix - ri, iy, ix + ri, iy, cx + sway * s * 0.7,
		cy - r * 0.9 - tall * s * 0.5, 0);
	/* the dark core round the wick */
	ellipse(ix - ri * 0.35, iy - ri * 0.1, ix + ri * 0.35, iy + ri * 0.9, 1);
}

/* whole pixels: columns a to b, rows c to d */
static void pix(int a, int c, int b, int d, int col)
{
	int x, y;

	for (y = c * SS; y < (d + 1) * SS; ++y)
		for (x = a * SS; x < (b + 1) * SS; ++x)
			if (x >= 0 && y >= 0 && x < big && y < big)
				cv[y][x] = col;
}

static int round1(double v)	/* to whole pixels, at least 1 */
{
	int n = (int)floor(v + 0.5);

	return n < 1 ? 1 : n;
}

static void draw(int f)
{
	double u = size / 32.0, p[8], dx, dy, mx;
	int ww, wl, wr, wt, wb, t, wh, c;

	/* The wax, the wick and the flame's base are on whole pixels:
	 * else at small sizes the wick goes, and the flame floats. */
	ww = round1(WAXW * u) | 1;	/* odd: the wick in the middle */
	wl = round1(16 * u) - ww / 2;
	wr = wl + ww - 1;
	c = wl + ww / 2;		/* the wick's column */
	wt = round1(TOP * u);		/* the wax's top row */
	wb = round1(27 * u);
	t = round1(1.4 * u);		/* its black edge */
	wh = round1(2 * u);		/* the wick's height */
	if (wh < 2)
		wh = 2;
	/* the dish's middle in the middle of a pixel row: so at small sizes
	 * its rows above and below are shorter, and it is round. Its middle
	 * is under the wick. */
	dy = (floor(29 * u) + 0.5) / u;
	mx = (c + 0.5) / u;

	memset(cv, 0, sizeof cv);
	ellipse(mx - 11, dy - 2, mx + 11, dy + 2, 1);	/* the dish */
	rect(mx - 6, 26, mx + 6, 28.5, 1);
	pix(wl, wt, wr, wb, 1);			/* the wax: black edges, */
	pix(wl + t, wt + t, wr - t, round1(26 * u) - 1, 0);	/* white inside, */
	dx = (wl + t) / u, dy = (wt + t) / u;	/* a drip at the left */
	p[0] = dx, p[1] = dy;
	p[2] = dx + 2.6, p[3] = dy;
	p[4] = dx + 1.6, p[5] = dy + 3.6;
	p[6] = dx + 0.6, p[7] = dy + 5.1;
	poly(p, 4, 1);
	pix(c, wt - wh, c, wt - 1, 1);		/* the wick */
	if (f > 0)		/* the flame's base on the wick: its last row shows */
		flame((c + 0.5) / u, (wt - 1) / u, FLR, f - 1);
}

/* the frame, small: 1 black, a byte a pixel */
static void shrink(unsigned char *px)
{
	int x, y, i, j, n;

	for (y = 0; y < size; ++y)
		for (x = 0; x < size; ++x) {
			n = 0;
			for (j = 0; j < SS; ++j)
				for (i = 0; i < SS; ++i)
					n += cv[y * SS + j][x * SS + i];
			px[y * size + x] = 2 * n > SS * SS;
		}
}

int main(int ac, char **av)
{
	static unsigned char px[FRAMES][MAXS * MAXS], b[MAXS * MAXS / 8];
	static char name[300], opt[20];
	char *pre = 0;
	int x0, y0, x1, y1, w, h, bpl, f, y, x, i;
	FILE *fp;

	for (i = 1; i < ac; ++i)
		if (!strcmp(av[i], "-s") && i + 1 < ac)
			size = atoi(av[++i]);
		else if (!strcmp(av[i], "-p") && i + 1 < ac)
			pre = av[++i];
		else {
			fprintf(stderr, "usage: candle [-s size] [-p prefix]\n");
			return 2;
		}
	if (size < 8 || size > MAXS) {
		fprintf(stderr, "candle: size 8 to %d\n", MAXS);
		return 2;
	}
	big = size * SS;
	x0 = y0 = size, x1 = y1 = -1;	/* the black part of all frames */
	for (f = 0; f < FRAMES; ++f) {
		draw(f);
		shrink(px[f]);
		for (y = 0; y < size; ++y)
			for (x = 0; x < size; ++x)
				if (px[f][y * size + x]) {
					if (x < x0) x0 = x;
					if (x > x1) x1 = x;
					if (y < y0) y0 = y;
					if (y > y1) y1 = y;
				}
	}
	w = x1 - x0 + 1, h = y1 - y0 + 1, bpl = (w + 7) / 8;
	if (size != SIZE)
		sprintf(opt, " -s %d", size);

	printf("/* Archaic's logo: a candle, %d x %d pixels (the black part of a\n"
		" * picture of %d x %d), 1 bit a pixel: 1 black, the high bit left.\n"
		" * Frame 0 has no flame; frames 1 to 8 are lit, the flame moving\n"
		" * left and right. Made by tools/candle.c:\n"
		" *\tcc -o candle tools/candle.c -lm\n"
		" *\t./candle%s > src/candle.h */\n", w, h, size, size, opt);
	printf("#define CANW %d\n#define CANH %d\n#define CANF %d\n", w, h,
		FRAMES);
	printf("static unsigned char candle[CANF][%d] = {\n", h * bpl);
	for (f = 0; f < FRAMES; ++f) {
		memset(b, 0, sizeof b);
		for (y = 0; y < h; ++y)
			for (x = 0; x < w; ++x)
				if (px[f][(y0 + y) * size + x0 + x])
					b[y * bpl + x / 8] |= 0x80 >> (x & 7);
		printf("{\t/* %d */\n", f);
		for (y = 0; y < h; ++y) {
			printf("\t");
			for (i = 0; i < bpl; ++i)
				printf("0x%02x,", b[y * bpl + i]);
			printf("\t/* ");
			for (x = 0; x < w; ++x)
				putchar(b[y * bpl + x / 8] & 0x80 >> (x & 7) ?
					'#' : '.');
			printf(" */\n");
		}
		printf("}%s\n", f + 1 < FRAMES ? "," : "");
		if (!pre)
			continue;
		sprintf(name, "%.280s%d.pbm", pre, f);
		if (!(fp = fopen(name, "wb"))) {
			perror(name);
			return 1;
		}
		fprintf(fp, "P4\n%d %d\n", w, h);
		fwrite(b, 1, h * bpl, fp);
		fclose(fp);
	}
	printf("};\n");
	return 0;
}
