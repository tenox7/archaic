/* cfacts - what this C compiler and linker do. Build with cfacts2.c. */
#include <stdio.h>

int abcdefg1();
int abcdefg2();
int abcdefghijklmno1();
int abcdefghijklmno2();
int abcdefghijklmnopqrstuvwxyz1();
int abcdefghijklmnopqrstuvwxyz2();
int Abcdefg1();

static struct pt { int x, y; } pa, pb;

int main()
{
	char c;
	unsigned char u;

	printf("short %d int %d long %d ptr %d\n", (int)sizeof(short),
		(int)sizeof(int), (int)sizeof(long), (int)sizeof(char *));
	printf("names 8: %d %d  16: %d %d  28: %d %d  case: %d (want 1 2 3 4 5 6 7)\n",
		abcdefg1(), abcdefg2(), abcdefghijklmno1(), abcdefghijklmno2(),
		abcdefghijklmnopqrstuvwxyz1(), abcdefghijklmnopqrstuvwxyz2(),
		Abcdefg1());
	c = (char)200;
	u = 200;
	printf("char signed: %d  uchar: %d\n", c < 0, u);
	pa.x = 3;
	pa.y = 4;
	pb = pa;
	printf("struct assign: %d %d\n", pb.x, pb.y);
#ifdef __STDC__
	printf("__STDC__ defined\n");
#else
	printf("__STDC__ not defined\n");
#endif
	printf("-1 >> 1 = %d, 7 / -2 = %d, 7 %% -2 = %d\n", -1 >> 1, 7 / -2, 7 % -2);
	return 0;
}
