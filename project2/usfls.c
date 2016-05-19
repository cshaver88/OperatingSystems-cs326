// implement own version of ls 
// use readdir sys calls to list the contents of the current dir
// list files and directories on separate lines
// do NOT list files that start with a .

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void list_directory_contents();

int main(int argc, char* argv[]) {

	list_directory_contents();

	return 0;
}

void list_directory_contents() {

	DIR *dirp;
	struct dirent *dp;
	// char *name = argv[1];
	int len;
	int i;

	dirp = opendir(".");
	if (dirp == NULL) {
		printf("Can't open the specified directory.\n");
		exit(-1);
	}

	// len = strlen(name);
	while ((dp = readdir(dirp)) != NULL) {
		// while I have found something in the directory 
		// if doesn't start with .
		// print each one followed by \n
		// else skip to next item
		if (dp -> d_name[0] != '.') {
			printf("%s\n", dp -> d_name);
		}
		
	}
	closedir(dirp);

}
