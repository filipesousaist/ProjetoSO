#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "../lib/types.h"
#include "../lib/buffers.h"

#define SHELL_PIPE_NAME "/tmp/CircuitRouter-AdvShell.pipe"
#define CLIENT_PIPE_DIR_TEMPLATE "/tmp/CircuitRouter-Client-XXXXXX"

#define TRY(EXPRESSION) if ((EXPRESSION) < 0) {perror(NULL); exit(1);}


int main(int argc, char const *argv[]) {
	/* Parse program arguments */
	if (argc != 2) {
		fputs("Invalid arguments\n", stderr);
		exit(1);
	}
	char fileName[SHELL_MAX_PIPE_NAME];
	strcpy(fileName, argv[1]);

	char clientPipeDirName[] = CLIENT_PIPE_DIR_TEMPLATE;

	/* Generate unique filename for the pipe; it will be the first part of the
	message to send to shell */
	int clientPipeFd;
	TRY(mkdtemp(clientPipeDirName));

	/* Create a named pipe using the generated filename */
	char clientPipeName[CLIENT_MAX_PIPE_NAME];
	strcpy(clientPipeName, clientPipeDirName);
	strcat(clientPipeName, "/client.pipe");
	TRY(mkfifo(clientPipeName, 0600));

	/* Send message */
	int shellPipeFd;
	TRY(shellPipeFd = open(SHELL_PIPE_NAME, O_WRONLY));

	int clientPipeNameLength = strlen(clientPipeName);

	char message[MESSAGE_MAX_SIZE];
	while (TRUE) {
		strcpy(message, clientPipeName);
		message[clientPipeNameLength] = ' ';
		if (fgets(message + clientPipeNameLength + 1, COMMAND_MAX_SIZE, stdin) \
			!= NULL) {
			
			TRY(write(shellPipeFd, message, strlen(message)));

			/* Recieve message */	
			char response[MESSAGE_MAX_SIZE];
			bzero(response, MESSAGE_MAX_SIZE);
			TRY(clientPipeFd = open(clientPipeName, O_RDONLY));
			TRY(read(clientPipeFd, response, MESSAGE_MAX_SIZE));
			TRY(close(clientPipeFd));

			printf("%s\n", response);
		}
		else
			fputs("Error reading line arguments.\n", stderr);
	}
	
	return 0;
}