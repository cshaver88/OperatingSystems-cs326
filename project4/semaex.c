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
	int *data_parent = (int *) (p1 + 8192);
	void *sem1_child = (void *) p2;
	int *data_child = (int *) (p2 + 8192);
	

	/* allocate 4 shared pages */
	mmap_alloc(p1, 4);

	/* init a semaphore on the 1st page */
	sema_init(sem1_parent, 1);
	
	/* put data into the 3rd page */
	*data_parent = 0;
	
	printf(1, "before fork\n");
	
	id = fork();
	
	if (id == 0) {
		mmap_attach(p2);
		printf(1, "child data = %d\n", *data_child);
		for (i = 0; i < 1000; i++) {
			sema_down(sem1_child);
			*data_child = *data_child + 1;
			sema_up(sem1_child);
		}
		printf(1, "child data = %d\n", *data_child);		
		mmap_detach();
		exit();
	}
	
	/* in parent */

	printf(1, "parent data = %d\n", *data_parent);
	for (i = 0; i < 1000; i++) {
		sema_down(sem1_parent);
		*data_parent = *data_parent + 1;
		sema_up(sem1_parent);
	}
	printf(1, "parent data = %d\n", *data_parent);

	wait();
	printf(1, "final data = %d\n", *data_parent);
	
	mmap_detach();
	exit();
}
