#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "../lib/types.h"
#include "../lib/buffers.h"

#define SHELL_PIPE_NAME "/tmp/CircuitRouter-AdvShell.pipe"

#define TRY(EXPRESSION) if ((EXPRESSION) < 0) {perror(NULL); exit(1);}

char clientPipeName[CLIENT_MAX_PIPE_NAME];
char clientPipeDirName[] = "/tmp/CircuitRouter-Client-XXXXXX";

void pipeSignalHandler(int sig) {
	char pipeError[] = "Cannot access shell pipe\n";
	write(2, pipeError, sizeof(pipeError));
	/* Delete pipe */
	if (unlink(clientPipeName) != 0) {
		char unlinkError[] = "Error: unlink\n";
		write(2, unlinkError, sizeof(unlinkError));
	}
	/* Delete pipe directory */
	if (rmdir(clientPipeDirName) != 0) {
		char rmdirError[] = "Error: rmdir\n";
		write(2, rmdirError, sizeof(rmdirError));
	}
	_exit(1);
}

int main(int argc, char const *argv[]) {
	/* Parse program arguments */
	if (argc != 2) {
		fputs("Invalid arguments\n", stderr);
		exit(1);
	}
	char fileName[SHELL_MAX_PIPE_NAME];
	strcpy(fileName, argv[1]);

	/* Generate unique filename for the pipe; it will be the first part of the
	message to send to shell */
	TRY(mkdtemp(clientPipeDirName));

	/* Create a named pipe using the generated filename */
	strcpy(clientPipeName, clientPipeDirName);
	strcat(clientPipeName, "/client.pipe");
	TRY(mkfifo(clientPipeName, 0600));

	/* Open shell pipe */
	int shellPipeFd;
	TRY(shellPipeFd = open(SHELL_PIPE_NAME, O_WRONLY));

	/* Listen to signals of type SIGPIPE */
	struct sigaction action;
	action.sa_handler = &pipeSignalHandler;
	TRY(sigaction(SIGPIPE, &action, NULL));

	int clientPipeNameLength = strlen(clientPipeName);

	char message[MESSAGE_MAX_SIZE];
	while (TRUE) {
		strcpy(message, clientPipeName);
		message[clientPipeNameLength] = ' ';
		if (fgets(message + clientPipeNameLength + 1, COMMAND_MAX_SIZE, stdin) \
			!= NULL) {
			/* Send message */
			TRY(write(shellPipeFd, message, strlen(message)));

			/* Recieve message */	
			char response[MESSAGE_MAX_SIZE];
			bzero(response, MESSAGE_MAX_SIZE);
			int clientPipeFd;
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