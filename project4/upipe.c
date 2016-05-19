#include "types.h"
#include "stat.h"
#include "user.h"
#include "upipe.h"

struct
upipe *upipe_create(int size, void *vaddr)
{

    int numPages = 4 + (size / 4096);

    mmap_alloc(vaddr, numPages);

    // create offsets for locations of each page in page table
    void *cvFull_addr = vaddr + (4096 * 1);
    void *cvEmpty_addr = vaddr + (4096 * 2);
    void *meta_addr = vaddr + (4096 * 3);

    lock_init(vaddr, 0);  // I am unlocked
    cond_init(cvFull_addr); //I am not full when full = 1
    cond_init(cvEmpty_addr); //I am empty when not empty = 0
    struct upipe *p = meta_addr;
    p -> read_index = 0;
    p -> write_index = 0;
    p -> count = 0;
    p -> size = size;

    return p;
}

struct
upipe *upipe_attach(void *vaddr)
{

    mmap_attach(vaddr);

    void *offset = vaddr + (4096 * 3);

    struct upipe *p = offset;

    return p;
}

int
upipe_detach(struct upipe *p)
{

    mmap_detach();
    p = 0;

    return 0;
}

int
upipe_write(struct upipe *p, char *buf, int n)
{

    int i;

    // Locations of all vars in shared pages
    void *base_addr = ((void *) p) - (3 * 4096);
    void *cvFull_addr = ((void *) p) - (4096 * 2);
    void *cvEmpty_addr = ((void *) p) - (4096 * 1);

    lock_acquire(base_addr);

    //check if the buff is full
    for (i = 0; i < n; i ++) {
        while (p -> count == p -> size) {
            cond_signal(cvEmpty_addr);
            cond_wait(cvFull_addr, base_addr);
        }
        p -> data[p -> write_index++ % p -> size] = buf[i];
        p -> count += 1;
    }

    cond_signal(cvEmpty_addr);
    lock_release(base_addr);

    return 0;
}

int
upipe_read(struct upipe * p, char *buf, int n)
{

    int i;

    // Locations of all vars in shared pages
    void *base_addr = ((void *) p) - (3 * 4096);
    void *cvFull_addr = ((void *) p) - (4096 * 2);
    void *cvEmpty_addr = ((void *) p) - (4096 * 1);

    lock_acquire(base_addr);

    while (p -> read_index == p -> write_index) {
        cond_signal(cvFull_addr);
        cond_wait(cvEmpty_addr, base_addr);
    }

    for (i = 0; i < n; i ++) {
        buf[i] = p -> data[p -> read_index++ % p -> size];
        p -> count -= 1;
    }

    cond_signal(cvFull_addr);
    lock_release(base_addr);

    return 0;
}