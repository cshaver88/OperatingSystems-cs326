#include "types.h"
#include "stat.h"
#include "user.h"

int main (int argc, char *argv[]){
	printf(1, "Hello xv6\n");
	printf(1, "getpid() = %d\n", getpid());
	printf(1, "mygetpid() = %d\n", mygetpid());
	exit();
}
