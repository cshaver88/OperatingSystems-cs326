//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "file.h"
#include "fcntl.h"
#include "spinlock.h"
#include "list.h"

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;
  struct file *f;

  if(argint(n, &fd) < 0)
    return -1;
  if(fd < 0 || fd >= NOFILE || (f=proc->ofile[fd]) == 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(pf)
    *pf = f;
  return 0;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
fdalloc(struct file *f)
{
  int fd;

  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd] == 0){
      proc->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}

int
sys_dup(void)
{
  struct file *f;
  int fd;
  
  if(argfd(0, 0, &f) < 0)
    return -1;
  if((fd=fdalloc(f)) < 0)
    return -1;
  filedup(f);
  return fd;
}

int
sys_read(void)
{
  struct file *f;
  int n;
  char *p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return fileread(f, p, n);
}

int
sys_write(void)
{
  struct file *f;
  int n;
  char *p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return filewrite(f, p, n);
}

int
sys_close(void)
{
  int fd;
  struct file *f;
  
  if(argfd(0, &fd, &f) < 0)
    return -1;
  proc->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

int
sys_fstat(void)
{
  struct file *f;
  struct stat *st;
  
  if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
    return -1;
  return filestat(f, st);
}

// Create the path new as a link to the same inode as old.
int
sys_link(void)
{
  char name[DIRSIZ], *new, *old;
  struct inode *dp, *ip;

  if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
    return -1;

  begin_op();
  if((ip = namei(old)) == 0){
    end_op();
    return -1;
  }

  ilock(ip);
  if(ip->type == T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }

  ip->nlink++;
  iupdate(ip);
  iunlock(ip);

  if((dp = nameiparent(new, name)) == 0)
    goto bad;
  ilock(dp);
  if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
    iunlockput(dp);
    goto bad;
  }
  iunlockput(dp);
  iput(ip);

  end_op();

  return 0;

bad:
  ilock(ip);
  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);
  end_op();
  return -1;
}

// Is the directory dp empty except for "." and ".." ?
static int
isdirempty(struct inode *dp)
{
  int off;
  struct dirent de;

  for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      panic("isdirempty: readi");
    if(de.inum != 0)
      return 0;
  }
  return 1;
}

//PAGEBREAK!
int
sys_unlink(void)
{
  struct inode *ip, *dp;
  struct dirent de;
  char name[DIRSIZ], *path;
  uint off;

  if(argstr(0, &path) < 0)
    return -1;

  begin_op();
  if((dp = nameiparent(path, name)) == 0){
    end_op();
    return -1;
  }

  ilock(dp);

  // Cannot unlink "." or "..".
  if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
    goto bad;

  if((ip = dirlookup(dp, name, &off)) == 0)
    goto bad;
  ilock(ip);

  if(ip->nlink < 1)
    panic("unlink: nlink < 1");
  if(ip->type == T_DIR && !isdirempty(ip)){
    iunlockput(ip);
    goto bad;
  }

  memset(&de, 0, sizeof(de));
  if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
    panic("unlink: writei");
  if(ip->type == T_DIR){
    dp->nlink--;
    iupdate(dp);
  }
  iunlockput(dp);

  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);

  end_op();

  return 0;

bad:
  iunlockput(dp);
  end_op();
  return -1;
}

static struct inode*
create(char *path, short type, short major, short minor)
{
  uint off;
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if((dp = nameiparent(path, name)) == 0)
    return 0;
  ilock(dp);

  if((ip = dirlookup(dp, name, &off)) != 0){
    iunlockput(dp);
    ilock(ip);
    if(type == T_FILE && ip->type == T_FILE)
      return ip;
    iunlockput(ip);
    return 0;
  }

  if((ip = ialloc(dp->dev, type)) == 0)
    panic("create: ialloc");

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  iupdate(ip);

  if(type == T_DIR){  // Create . and .. entries.
    dp->nlink++;  // for ".."
    iupdate(dp);
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      panic("create dots");
  }

  if(dirlink(dp, name, ip->inum) < 0)
    panic("create: dirlink");

  iunlockput(dp);

  return ip;
}

int
sys_open(void)
{
  char *path;
  int fd, omode;
  struct file *f;
  struct inode *ip;

  if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
    return -1;

  begin_op();

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op();
      return -1;
    }
  } else {
    if((ip = namei(path)) == 0){
      end_op();
      return -1;
    }
    ilock(ip);
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  end_op();

  f->type = FD_INODE;
  f->ip = ip;
  f->off = 0;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
  return fd;
}

int
sys_mkdir(void)
{
  char *path;
  struct inode *ip;

  begin_op();
  if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

int
sys_mknod(void)
{
  struct inode *ip;
  char *path;
  int len;
  int major, minor;
  
  begin_op();
  if((len=argstr(0, &path)) < 0 ||
     argint(1, &major) < 0 ||
     argint(2, &minor) < 0 ||
     (ip = create(path, T_DEV, major, minor)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

int
sys_chdir(void)
{
  char *path;
  struct inode *ip;

  begin_op();
  if(argstr(0, &path) < 0 || (ip = namei(path)) == 0){
    end_op();
    return -1;
  }
  ilock(ip);
  if(ip->type != T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  iput(proc->cwd);
  end_op();
  proc->cwd = ip;
  return 0;
}

int
sys_exec(void)
{
  char *path, *argv[MAXARG];
  int i;
  uint uargv, uarg;

  if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
    return -1;
  }
  memset(argv, 0, sizeof(argv));
  for(i=0;; i++){
    if(i >= NELEM(argv))
      return -1;
    if(fetchint(uargv+4*i, (int*)&uarg) < 0)
      return -1;
    if(uarg == 0){
      argv[i] = 0;
      break;
    }
    if(fetchstr(uarg, &argv[i]) < 0)
      return -1;
  }
  return exec(path, argv);
}

int
sys_pipe(void)
{
  int *fd;
  struct file *rf, *wf;
  int fd0, fd1;

  if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
    return -1;
  if(pipealloc(&rf, &wf) < 0)
    return -1;
  fd0 = -1;
  if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
    if(fd0 >= 0)
      proc->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  fd[0] = fd0;
  fd[1] = fd1;
  return 0;
}

int
mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm);

int
sys_palloc(void)
{

	if (argint(0, (int *) &(proc->addr1)) < 0) {
		return -1;
	}

	if (argint(1, (int *) &(proc->addr2)) < 0) {
		return -1;
	}
	
	cprintf("%p\n", proc->addr1);
	cprintf("%p\n", proc->addr2);
	
	/* allocate a page */
	proc->page = (void *) kalloc();
	if (proc->page != 0) {
		cprintf("SUCCESS: page allocated!\n");
	}
	
	/* map page to addr1 */
	mappages(proc->pgdir, proc->addr1, PGSIZE, v2p(proc->page), PTE_W|PTE_U);

	/* map page to addr2 */
	mappages(proc->pgdir, proc->addr2, PGSIZE, v2p(proc->page), PTE_W|PTE_U);
		
	return 0;
}

/* Keep track of kernel allocated pages for shared memory region */
void *shared_pages[16];
/* Number of shared pages */
int shared_npages = 0;
/* Reference count for number of processes that have access */
int shared_refcount = 0;
/* Lock for shared variables */
struct spinlock shared_lock;
/* Shared initialized */
int shared_initialized = 0;

int
sys_mmap_alloc(void)
{
	void *vaddr;
	int npages;
	int i;
	void *page;

	if (shared_initialized == 0) {
		shared_initialized = 1;
		initlock(&shared_lock, "shared_mem_lock");
	}

	if (shared_refcount > 0) {
		cprintf("Shared memory already allocated\n");
		return -1;
	}

	if (argint(0, (int *) &(vaddr)) < 0) {
		return -1;
	}

	if (argint(1, (int *) &(npages)) < 0) {
		return -1;
	}
	
	cprintf("vaddr: %p\n", vaddr);
	cprintf("npages: %d\n", npages);

	acquire(&shared_lock);
	proc->shared = 1;
	proc->vaddr = vaddr;
	
	shared_npages = npages;
	shared_refcount = 1;

	for (i = 0; i < npages; i++) {
		page = (void *) kalloc();
		shared_pages[i] = page;
		mappages(proc->pgdir, vaddr + ((PGSIZE) * i), PGSIZE, v2p(page), PTE_W|PTE_U);
	}
		
	release(&shared_lock);
	return 0;
}

int
sys_mmap_attach(void)
{
	void *vaddr;
	int i;
	
	if (argint(0, (int *) &(vaddr)) < 0) {
		return -1;
	}

	acquire(&shared_lock);
	proc->vaddr = vaddr;
	proc->shared = 1;

	shared_refcount += 1;

	for (i = 0; i < shared_npages; i++) {
		mappages(proc->pgdir, vaddr + ((PGSIZE) * i), PGSIZE, v2p(shared_pages[i]), PTE_W|PTE_U);
	}
	release(&shared_lock);
	
	return 0;
}

int sys_mmap_detach(void)
{
	int i;
	
	if (proc->shared == 0) {
		return -1;
	}
	
	acquire(&shared_lock);

	for (i = 0; i < shared_npages; i++) {
		clearptep(proc->pgdir, proc->vaddr + (PGSIZE * i));
	}
	
	shared_refcount -= 1;
	if (shared_refcount == 0) {
		for (i = 0; i < shared_npages; i++) {
			kfree(shared_pages[i]);
		}
	}

	release(&shared_lock);
	
	return 0;
}

struct semaphore {
	struct spinlock lock;
	/* use address of value for sleep/wakeup channel */
	int value;
};

int sys_sema_init(void)
{
	void *vaddr;
	int v;
	struct semaphore *sp;

	/* Get the uva for the location of the semaphore struct. */
	if (argint(0, (int *) &(vaddr)) < 0) {
		return -1;
	}

	/* Get the initial value for the semaphore. */
	if (argint(1, (int *) &(v)) < 0) {
		return -1;
	}

	/* We need to map the uva to a kva (kernel virtual address) so that we do semaphore operations on
	   the same address. This is most important for sleep() and wakeup(). */
	sp = (struct semaphore *) shared_pages[(vaddr - proc->vaddr) / PGSIZE];
	initlock(&sp->lock, "semaphore");
	sp->value = v;
	
	return 0;
}

int sys_sema_down(void)
{
	void *vaddr;
	struct semaphore *sp;

	/* Get the uva for the location of the semaphore. */
	if (argint(0, (int *) &(vaddr)) < 0) {
		return -1;
	}
	
	sp = (struct semaphore *) shared_pages[(vaddr - proc->vaddr) / PGSIZE];

	/* Try to decrement the semaphore value. Block if the value is 0. */
	acquire(&sp->lock);
	while (sp->value == 0) {
		sleep(&sp->value, &sp->lock);
	}
	sp->value -= 1;
	release(&sp->lock);
	
	return 0;
}

int sys_sema_up(void)
{
	void *vaddr;
	
	struct semaphore *sp;

	if (argint(0, (int *) &(vaddr)) < 0) {
		return -1;
	}

	sp = (struct semaphore *) shared_pages[(vaddr - proc->vaddr) / PGSIZE];

	/* Increment the semaphore value and wake up any blocked processes on the semaphore. */
	acquire(&sp->lock);
	wakeup(&sp->value);
	sp->value += 1;
	release(&sp->lock);
	
	return 0;
}

struct mutexlock {
	struct spinlock lock;
	int value;
};

void lock_init(struct mutexlock *mlk, int value)
{
	initlock(&mlk->lock, "mutexlock");
	mlk->value = value;
}

int sys_lock_init(void)
{
	void *vaddr;
	int v;
	struct mutexlock *mlk;

	/* Get the uva for the location of the semaphore struct. */
	if (argint(0, (int *) &(vaddr)) < 0) {
		return -1;
	}

	/* Get the initial value for the semaphore. */
	if (argint(1, (int *) &(v)) < 0) {
		return -1;
	}

	/* We need to map the uva to a kva (kernel virtual address) so that we do semaphore operations on
	   the same address. This is most important for sleep() and wakeup(). */
	mlk = (struct mutexlock *) shared_pages[(vaddr - proc->vaddr) / PGSIZE];
    lock_init(mlk, v);
	return 0;
}

void lock_acquire(struct mutexlock *mlk)
{
	acquire(&mlk->lock);
	while (mlk->value == 1) {
		sleep(&mlk->value, &mlk->lock);
	}
	mlk->value = 1;
	release(&mlk->lock);
}

int sys_lock_acquire(void)
{
	void *vaddr;
	struct mutexlock *mlk;

	/* Get the uva for the location of the semaphore. */
	if (argint(0, (int *) &(vaddr)) < 0) {
		return -1;
	}
	
	mlk = (struct mutexlock *) shared_pages[(vaddr - proc->vaddr) / PGSIZE];

	lock_acquire(mlk);
	
	return 0;
}

void lock_release(struct mutexlock *mlk)
{
	
	acquire(&mlk->lock);
	wakeup(&mlk->value);
	mlk->value = 0;
	release(&mlk->lock);
}

int sys_lock_release(void)
{
	void *vaddr;
	
	struct mutexlock *mlk;

	if (argint(0, (int *) &(vaddr)) < 0) {
		return -1;
	}

	mlk = (struct mutexlock *) shared_pages[(vaddr - proc->vaddr) / PGSIZE];

	lock_release(mlk);

	return 0;
}

struct condition {
	struct list clist;
};

struct condition_node {
	struct list_elem elem;
	/* use address of lock as sleep/wakeup channel */
	struct spinlock lock;
};

void cond_init(struct condition *cv)
{
	list_init(&cv->clist);
}

int sys_cond_init(void)
{
	void *vaddr;
	struct condition *cv;

	/* Get the uva for the location of the semaphore struct. */
	if (argint(0, (int *) &(vaddr)) < 0) {
		return -1;
	}

	/* We need to map the uva to a kva (kernel virtual address) so that we do semaphore operations on
	   the same address. This is most important for sleep() and wakeup(). */
	cv = (struct condition *) shared_pages[(vaddr - proc->vaddr) / PGSIZE];
    cond_init(cv);
	return 0;
}

void cond_wait(struct condition *cv, struct mutexlock *mlk)
{
	struct condition_node c_node;	
	initlock(&c_node.lock, "condition_node");
	list_push_back(&cv->clist, &c_node.elem);
	lock_release(mlk);
	acquire(&c_node.lock);
	sleep(&c_node.lock, &c_node.lock);
	release(&c_node.lock);
	lock_acquire(mlk);
}

int sys_cond_wait(void)
{
	void *vaddr_cv;
	void *vaddr_mlk;
	struct condition *cv;
	struct mutexlock *mlk;

	/* Get the uva for the location of the semaphore struct. */
	if (argint(0, (int *) &(vaddr_cv)) < 0) {
		return -1;
	}

	if (argint(1, (int *) &(vaddr_mlk)) < 0) {
		return -1;
	}

	/* We need to map the uva to a kva (kernel virtual address) so that we do semaphore operations on
	   the same address. This is most important for sleep() and wakeup(). */
	cv = (struct condition *) shared_pages[(vaddr_cv - proc->vaddr) / PGSIZE];
	mlk = (struct mutexlock *) shared_pages[(vaddr_mlk - proc->vaddr) / PGSIZE];

    cond_wait(cv, mlk);
	return 0;
}

void cond_signal(struct condition *cv)
{
	struct condition_node *c_node;	
	struct list_elem *elem;

    if (list_empty(&cv->clist)) {
      return;
    }

	elem = list_pop_front(&cv->clist);
	c_node = list_entry(elem, struct condition_node, elem);
	acquire(&c_node->lock);
	wakeup(&c_node->lock);
	release(&c_node->lock);
}

int sys_cond_signal(void)
{
	void *vaddr_cv;
	struct condition *cv;

	/* Get the uva for the location of the semaphore struct. */
	if (argint(0, (int *) &(vaddr_cv)) < 0) {
		return -1;
	}

	/* We need to map the uva to a kva (kernel virtual address) so that we do semaphore operations on
	   the same address. This is most important for sleep() and wakeup(). */
	cv = (struct condition *) shared_pages[(vaddr_cv - proc->vaddr) / PGSIZE];

    cond_signal(cv);
	return 0;
}
