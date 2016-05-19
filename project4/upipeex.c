#include "types.h"
#include "stat.h"
#include "user.h"
#include "upipe.h"

#define NULL (void *) 0

int main(int argc, char **argv)
{
    struct upipe *p;
    char data[512];
    int id;
    int i;
    int j;

    p = upipe_create(4096, (void *)0x40000000); /* at 1GB */

    if (p == NULL) {
        printf(1, "Cannot create upipe, exiting\n");
        exit();
    }

    id = fork();

    if (id == 0) {
        /* in child */
        p = upipe_attach((void *)0x48000000); /* at 1.5 GB */
        if (p == NULL) {
            printf(1, "Cannot attach to a upipe, exiting\n");
        }
        /* fill a 512 buffer with 'x's */
        //write only one not 512
        for (i = 0; i < 512; i++) {
            data[i] = 'x';
        }
        for (i = 0; i < 10; i++) {
            if (upipe_write(p, data, 512) < 0) {
                printf(1, "upipe_write failure, exiting.\n");
                exit();
            }
        }
        upipe_detach(p);
        exit();
    }
//do only one not 10
    for (i = 0; i < 10; i++) {
        if (upipe_read(p, data, 512) < 0) {
            printf(1, "upipe_read failure, exiting.\n");
        }
        for (j = 0; j < 512; j++) {
            if (data[j] != 'x') {
                printf(1, "Did not receive data correctly from upipe, expected all 'x' values, exiting.\n");
                exit();
            }
        }
    }
    upipe_detach(p);
    printf(1, "upipe test finished\n");
    exit();
}