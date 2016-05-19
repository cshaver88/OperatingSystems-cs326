#include "types.h"
#include "stat.h"
#include "user.h"

/* This program creates two semaphores that are used to synchronize the parent with the child.
   First we need to create a shared memory region for the semaphores and any additional data.
   We create one semaphore on one shared page and the other on another shared page.
   We use the semaphores to ping-pong between the parent and child process.
*/   

int
main(int argc, char *argv[])
{
	int id;
	int i;
	char *p1 = (char *) 0x40000000; /* starting address for the shared memory region in the parent */
	char *p2 = (char *) 0x48000000; /* starting address for the shared memory region in the child */
	void *sem1_parent = (void *) p1;
	void *sem2_parent = (void *) p1 + 4096;
	char *data_parent = p1 + 8192;
	void *sem1_child = (void *) p2;
	void *sem2_child = (void *) p2 + 4096;
	char *data_child = p2 + 8192;
	

	/* allocate 4 shared pages */
	mmap_alloc(p1, 4);

	/* init a semaphore on the 1st page */
	sema_init(sem1_parent, 1);
	/* init a semaphore on the 2nd page */
	sema_init(sem2_parent, 0);
	
	/* put data into the 3rd page */
	data_parent[0] = 'a';
	
	printf(1, "before fork\n");
	
	id = fork();
	
	if (id == 0) {
		mmap_attach(p2);
		printf(1, "child data[0] = %c\n", data_child[0]);
		for (i = 0; i < 5; i++) {
			sema_down(sem2_child);
			printf(1, "In child\n");
			sema_up(sem1_child);
		}
		mmap_detach();
		exit();
	}
	
	/* in parent */
	for (i = 0; i < 5; i++) {
		sema_down(sem1_parent);
		printf(1, "In parent\n");
		sema_up(sem2_parent);
	}
	wait();
	mmap_detach();
	exit();
}
