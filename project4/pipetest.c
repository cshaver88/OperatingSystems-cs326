#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "file.h"
#include "fcntl.h"

//For gdb == /opt/i386dev/bin/i386-xv6-elf-gdb kernel
int main(int argc, char* argv[]) {

    int fd[2];
    int x;
    int i = 0;

    pipe(fd);

    printf(1, "Empty: %d \n", pipecount(fd[0])); //empty

    write(fd[1], "hi", 2);
    printf(1, "Between Full and Empty: %d \n", pipecount(fd[0])); // between full and empty

    for (; i <= 101; i ++) {
        write(fd[1], "hello", 5);
    }
    printf(1, "Full: %d \n", pipecount(fd[0])); // full

    x = open("usfsort.c", O_RDONLY);
    printf(1, "Invalid: %d \n", pipecount(x));

    exit();
    return 0;

//    gdb to get the data from the pipe
//remember this!!!
//    b piperead
//    p p->data
}