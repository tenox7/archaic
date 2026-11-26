/* heap.c - Idris: how much memory a program file asks for, for its heap
 * and stack. It is the 4 bytes at 16 of the file, the high byte first.
 *	heap file	print it, in KB
 *	heap file kb	set it
 * lka needs it: Idris's linker asks for 128 KB, and that is too little
 * to link Archaic. A copy of the linker that asks for 512 KB links it.
 * Build: c -o heap heap.c (mk does it).
 */
main(argc, argv)
int argc;
char **argv;
{
	static unsigned char b[4];
	long kb;
	int fd;

	if (argc < 2 || (fd = open(argv[1], argc > 2 ? 2 : 0)) < 0) {
		printf("usage: heap file [kb]\n");
		return 1;
	}
	lseek(fd, 16L, 0);
	if (read(fd, b, 4) != 4) {
		printf("heap: can't read %s\n", argv[1]);
		return 1;
	}
	kb = ((long)b[0] << 14) + ((long)b[1] << 6) + (b[2] >> 2);
	if (argc < 3) {
		printf("%s: %ld KB\n", argv[1], kb);
		return 0;
	}
	kb = atoi(argv[2]);
	b[0] = (unsigned char)(kb >> 14);
	b[1] = (unsigned char)(kb >> 6);
	b[2] = (unsigned char)(kb << 2);
	b[3] = 0;
	lseek(fd, 16L, 0);
	if (write(fd, b, 4) != 4) {
		printf("heap: can't write %s\n", argv[1]);
		return 1;
	}
	return 0;
}
