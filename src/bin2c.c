/* bin2c.c - 3B2 build tool: standard input as a C table on standard
 * output. mk3b2 makes dmdm.h with it: the program for the DMD 5620
 * terminal (archaic.m, from dmdterm.c), which goes into archaic. */
#include <stdio.h>

main()
{
	long n = 0;
	int c;

	printf("static unsigned char termm[] = {\n");
	while ((c = getchar()) != EOF) {
		printf("%d,", c);
		if (++n % 16 == 0)
			printf("\n");
	}
	printf("0 };\n#define TERMN %ld\n", n);
	return 0;
}
