#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
	int *p = (int *) 0x80000000;

	int x;
	
	printf(1, "Hello xv6!\n");
	printf(1, "getpid() = %d\n", getpid());
	printf(1, "mygetpid() = %d\n", mygetpid());
	printf(1, "my_getpid() = %d\n", my_getpid());

	x = *p;
	
	printf(1, "x = %d\n", x);

  	exit();
}
