#include "types.h"
#include "stat.h"
#include "user.h"

/* Simple use of locks and condition variables. */

int
main(int argc, char *argv[])
{
	int id;
	//int i;
	char *p1 = (char *) 0x40000000; /* starting address for the shared memory region in the parent */
	char *p2 = (char *) 0x48000000; /* starting address for the shared memory region in the child */
	void *lock_parent = (void *) p1;
	void *cv_parent = (void *) (p1 + 4096);
	int *data_parent = (int *) (p1 + 8192);
	void *lock_child = (void *) p2;
	void *cv_child = (void *) (p2 + 4096);
	int *data_child = (int *) (p2 + 8192);
	

	/* allocate 4 shared pages */
	mmap_alloc(p1, 4);

	/* init a lock and cv */
	lock_init(lock_parent, 0);
	cond_init(cv_parent);
	
	/* put data into the 3rd page */
	*data_parent = 0;
	
	printf(1, "before fork\n");
	
	id = fork();
	
	if (id == 0) {
		mmap_attach(p2);
		printf(1, "child data = %d\n", *data_child);
		lock_acquire(lock_child);
		*data_child = *data_child + 1;
		cond_signal(cv_child);
		lock_release(lock_child);
		printf(1, "child data = %d\n", *data_child);		
		mmap_detach();
		exit();
	}
	
	/* in parent */

	printf(1, "parent data = %d\n", *data_parent);
	lock_acquire(lock_parent);
	cond_wait(cv_parent, lock_parent);
	*data_parent = *data_parent + 1;
	lock_release(lock_parent);
	printf(1, "parent data = %d\n", *data_parent);

	wait();
	printf(1, "final data = %d\n", *data_parent);
	
	mmap_detach();
	exit();
}
