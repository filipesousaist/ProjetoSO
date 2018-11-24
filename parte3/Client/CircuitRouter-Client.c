#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "../lib/types.h"
#include "../lib/buffers.h"

#define SHELL_PIPE_NAME "temp/CircuitRouter-AdvShell.pipe"
#define CLIENT_PIPE_TEMPLATE "temp/CircuitRouter-Client-XXXXXX.pipe"

#define TRY(EXPRESSION) if ((EXPRESSION) < 0) {perror(NULL); exit(1);}


int main(int argc, char const *argv[]) {
	/* Parse program arguments */
	if (argc != 2) {
		fputs("Invalid arguments\n", stderr);
		exit(1);
	}
	char fileName[SHELL_MAX_PIPE_NAME];
	strcpy(fileName, argv[1]);

	/* Message to send to shell */
	char clientPipeName[] = CLIENT_PIPE_TEMPLATE;

	/* Generate unique filename for the pipe; it will be the first part of the
	message to send to shell */
	int clientPipeFd;
	TRY(clientPipeFd = mkstemps(clientPipeName, strlen(".pipe")));
	TRY(unlink(clientPipeName)); /* We don't need the file itself */

	/* Create a named pipe using the generated filename */
	TRY(mkfifo(clientPipeName, 0600));

	int clientPipeNameLength = strlen(clientPipeName);

	char message[MESSAGE_MAX_SIZE];
	while (TRUE) {
		strcpy(message, clientPipeName);
		message[clientPipeNameLength] = ' ';
		if (fgets(message + clientPipeNameLength + 1, COMMAND_MAX_SIZE, stdin) \
			!= NULL) {
			/* Send message */
			int shellPipeFd;
			TRY(shellPipeFd = open(SHELL_PIPE_NAME, O_WRONLY));
			TRY(write(shellPipeFd, message, strlen(message)));
			TRY(close(shellPipeFd));

			/* Recieve message */
			TRY(clientPipeFd = open(clientPipeName, O_RDONLY));
			char response[MESSAGE_MAX_SIZE];
			TRY(read(clientPipeFd, response, MESSAGE_MAX_SIZE));
			TRY(close(clientPipeFd));

			printf("%s\n", response);
		}
		else
			fputs("Error reading line arguments.\n", stderr);
	}
	
	return 0;
}