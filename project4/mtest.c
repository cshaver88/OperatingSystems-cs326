#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
	char *p1 = (char *) 0x40000000;
	char *p2 = (char *) 0x48000000;
	int i;
	
	printf(1, "Hello xv6!\n");

	palloc((void *) p1, (void *) p2);

	for (i = 0; i < 10; i++) {
		printf(1, "%c", p2[i]);
	}

	printf(1, "\n");

	for (i = 0; i < 10; i++) {
		p1[i] = 'x';
	}

	for (i = 0; i < 10; i++) {
		printf(1, "%c", p2[i]);
	}

	printf(1, "\n");

  	exit();
}
