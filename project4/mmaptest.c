#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
	int id;
	char *p1 = (char *) 0x40000000;
	char *p2 = (char *) 0x48000000;

	mmap_alloc(p1, 4);
	p1[0] = 'a';
	printf(1, "before fork\n");
	
	id = fork();
	
	if (id == 0) {
		mmap_attach(p2);
		printf(1, "%c\n", p2[0]);
		mmap_detach();
		exit();
	}
	wait();
	mmap_detach();
	exit();
}
