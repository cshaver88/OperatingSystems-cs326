#include "syscall.h"
#include "traps.h"

#define SYSCALL(name) \
  .globl name; \
  name: \
    movl $SYS_ ## name, %eax; \
    int $T_SYSCALL; \
    ret

SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)
SYSCALL(mygetpid)
SYSCALL(my_getpid)
SYSCALL(halt)
SYSCALL(palloc)
SYSCALL(mmap_alloc)
SYSCALL(mmap_attach)
SYSCALL(mmap_detach)
SYSCALL(sema_init)
SYSCALL(sema_down)
SYSCALL(sema_up)
SYSCALL(lock_init)
SYSCALL(lock_acquire)
SYSCALL(lock_release)
SYSCALL(cond_init)
SYSCALL(cond_wait)
SYSCALL(cond_signal)

