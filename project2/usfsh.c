// support pipe redirection ex: echo world hello | wc
// for file redirection and pipe redirection support multiple args
// support output file redirection ex: echo hello world > hi.txt

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

const int max = 1024;

void command_line_input();
void split_tokens(char *buf);
void find_command_and_args(char *list_tokens[], int size);
void execute_command_curr_dir(char *list_tokens[]);
void output_file_redir(char *out, char *list_tokens[]);
void pipe_redir(char *mod_list[], char *list_tokens[]);

int main(int argc, char* argv[]) {
	
	while (1) {
		command_line_input();
	}

	return 0;
}

void command_line_input() {
	// print the $ and wait for input
	char buf[max];
	int count;
	int i;
	int boolean = 0;

	write(1, "$ ", 2);

	count = read(0, buf, max -1);
	buf[count] = '\0';

	split_tokens(buf);
}

void split_tokens(char *buf) {
	// split buf into tokens
	char *token;
	char *list_tokens[max];
	int i = 0;

	token = strtok(buf, " \n");
	while (token != NULL) {
		list_tokens[i] = token;
		i ++;
		token = strtok(NULL, " \n");
	}
	list_tokens[i] = NULL;
	find_command_and_args(list_tokens, i + 1);
}

void find_command_and_args(char *list_tokens[], int size) {
	// find the first command 
	// and it's args
	int i;
	char *mod_list[max];
	int j;
	int len;

	if (strcmp(list_tokens[0], "exit") == 0 || 
			strcmp(list_tokens[0], "Exit") == 0) {
			exit(0);
	}
	else {
		if (strcmp(list_tokens[0], "cd") == 0) {
			chdir(list_tokens[1]);
		}
		else {
			for (i = 0; i < size; i ++) {
				if (list_tokens[i] != NULL){
					if (strcmp(list_tokens[i], "|") == 0) {
						list_tokens[i] = NULL;
						for (j = 0; i < size; j ++) {
							mod_list[j] = list_tokens[i + 1];
							i ++;
						}
						mod_list[j + 1] = NULL;
						pipe_redir(mod_list, list_tokens);
						return;
					}
					else if (strcmp(list_tokens[i], ">") == 0) {
						list_tokens[i] = NULL;
						output_file_redir(list_tokens[i + 1], list_tokens);
						return;
					}
				}
			}
			execute_command_curr_dir(list_tokens);
		}
	}
}

void execute_command_curr_dir(char *list_tokens[]) {
	// execute commands that are in the current dir using ./ 
	//ex: ./usfls
	pid_t id;
	int fd;

	id = fork();

	if (id == 0) {
		if (execvp(list_tokens[0], list_tokens) < 0) {
			write(2, "The command entered is not a recognized command.\n", strlen("The command entered is not a recognized command.\n"));
			exit(-1);
		}
	}

	id = wait(NULL);
}

void output_file_redir(char *out, char *list_tokens[]) {
// support file output redirection that either creates or 
// 		overwrites the file ex: echo hi there > hi.txt
	int output;
	pid_t id;

	if ((output = open(out, O_CREAT | O_TRUNC | O_WRONLY, 0644)) < 0) {
		printf("Can't open %s\n", out);
		exit(-1);
	}

	id = fork();

	if (id == 0) {
		close(1);
		dup(output);
		close(output);
		if (execvp(list_tokens[0], list_tokens) < 0) {
			printf("The command entered is not a recognized command.\n");
			exit(-1);
		}

	}

	close(output);
	id = wait(NULL);
}

void pipe_redir(char *mod_list[], char *list_tokens[]) {
	// support pipe redirection ex: echo world hello | wc
	pid_t id;
	pid_t id2;
	int file_desc[2];

	pipe(file_desc);

	// First Child
	if ((id = fork()) == 0) {
		close(1);
		dup(file_desc[1]);
		close(file_desc[0]);
		close(file_desc[1]);
		if (execvp(list_tokens[0], list_tokens) < 0) {
			printf("The command %s is not a recognized command.\n", list_tokens[0]);
			exit(-1);
		} 
	}
	// Second Child
	if ((id2 = fork()) == 0) {
		close(0);
		dup(file_desc[0]);
		close(file_desc[1]);
		close(file_desc[0]);
		if (execvp(mod_list[0], mod_list) < 0) {
			printf("The command %s is not a recognized command.\n", list_tokens[0]);
			exit(-1);
		}
	}
	close(file_desc[0]);
	close(file_desc[1]);
	id2 = wait(NULL);
	id = wait(NULL);
}






















































