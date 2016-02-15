#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * [main  description]
 * @param  argc [description]
 * @param  argv [description]
 * @return      [description]
 */
int main (int argc, char *argv[]) {
	char command[100];
	if (argc > 1) {
		char *command = argv[1];
	} 
	for ( ; ; ) {
		if (argc > 1) {
			if (strcmp(command, "startup") == 0) {
				printf("Function not implemented yet: %s \n", command);
			} else if (strcmp(command, "shutdown") == 0) {
				printf("Function not implemented yet: %s \n", command);
			} else if (strcmp(command, "exit") == 0) {
				printf("exiting.. \n");
				exit(0);
			} else {
				printf("Command not found: %s \n", command);
			}
		} else {
			printf("csms: ");
			scanf("%s",command);
			if (strcmp(command, "startup") == 0) {
				printf("Function not implemented yet: %s \n", command);
			} else if (strcmp(command, "shutdown") == 0) {
				printf("Function not implemented yet: %s \n", command);
			} else if (strcmp(command, "exit") == 0) {
				printf("exiting.. \n");
				exit(0);
			} else {
				printf("Command not found: %s \n", command);
			}
		}
		sleep(1);
	}
	return 0;
}