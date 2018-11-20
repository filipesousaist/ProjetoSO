#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "../lib/types.h"

#define MAX_FILENAME 200
#define MAX_COMMAND_SIZE 200

enum {
	ERR_LINEARGS
};

void displayError(int code) {
	switch (code) {
		case ERR_LINEARGS:
			fputs("Error reading line arguments.\n", stderr);
			break;
	}
}

int main(int argc, char const *argv[]) {
	char fileName[MAX_FILENAME];
	if (argc != 2) {
		fputs("Invalid arguments\n", stderr);
		exit(1);
	}
	strcpy(fileName, argv[1]);

	char command[MAX_COMMAND_SIZE + 1];

	int fd;
	if ((fd = open("CircuitRouter-AdvShell.pipe", O_WRONLY)) == -1) {
		perror("open");
		exit(1);
	}

	while (TRUE) {
		if (fgets(command, MAX_FILENAME, stdin) == NULL)
			displayError(ERR_LINEARGS);
		printf("Li isto:\n%s", command);
		write(fd, command, strlen(command));
	}
	return 0;
}