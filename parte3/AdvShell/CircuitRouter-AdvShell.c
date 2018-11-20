#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include "../lib/commandlinereader.h"
#include "../lib/types.h"
#include "../lib/vector.h"

#define BUFFERSIZE 200
#define COMMAND_MAX_SIZE 200
#define PID_VECTOR_START 20
#define SOLVER_NAME "CircuitRouter-ParSolver"
#define PIPE_SUFFIX ".pipe"

enum {
	ERR_LINEARGS,
	ERR_COMMANDS,
	ERR_FORK
};

typedef struct managePipeArgs {
	char* pipeName;
} managePipeArgs_t;

bool_t exitedNormally(int status) {
	return WIFEXITED(status) && (WEXITSTATUS(status) == 0);
}

void waitNext(vector_t* pidVector, vector_t* stateVector)
{
	pid_t* pidPtr = (pid_t *) malloc(sizeof(pid_t));
	assert(pidPtr);
	int* pStatusPtr = (int *) malloc(sizeof(int));
	assert(pStatusPtr);
	while ((*pidPtr = wait(pStatusPtr)) == -1);
	vector_pushBack(pidVector, pidPtr);
	vector_pushBack(stateVector, pStatusPtr);
}

void displayError(int code) {
	switch (code) {
		case ERR_LINEARGS:
			fputs("Error reading line arguments\n", stderr);
			break;
		case ERR_COMMANDS:
			fputs("Invalid commands\n", stderr);
			break;
		case ERR_FORK:
			fputs("Error creating child process. Please try again\n", stderr);
			break;
	}
}

void* managePipe(void* args) {
	managePipeArgs_t* managePipeArgsPtr = (managePipeArgs_t*) args;
	char* pipeName = managePipeArgsPtr->pipeName;

	if (mkfifo(pipeName, 0755) != 0) {
		perror("mkfifo");
		exit(1);
	}
	int fd;
	if ((fd = open(pipeName, O_RDONLY)) == -1) {
		perror("open");
		exit(1);
	}

	char command[COMMAND_MAX_SIZE + 1];

	while (TRUE) {
		read(fd, command, COMMAND_MAX_SIZE);
		/*command[strlen(command)] = '\0'*/
		printf("Recebi:\n%s", command);
	}


}

int main(int argc, char const *argv[]) {
	long maxChildren = -1; /* -1 means no limit of child processes */
	long numChildren = 0;

	char* argVector[3];
	char buffer[BUFFERSIZE];
	pthread_t pipeThreadPtr;
	pid_t pid;
	pid_t* pidPtr;
	int* pStatusPtr; /* process exit status */	
	vector_t* pidVector = vector_alloc(PID_VECTOR_START);
	assert(pidVector);
	vector_t* stateVector = vector_alloc(PID_VECTOR_START);
	assert(stateVector);

	if (argc != 2 || sscanf(argv[1],"%ld", &maxChildren) != 1) {
		fputs("Invalid arguments\n", stderr);
		exit(1);
	}

	char* pipeName = malloc((strlen(argv[0]) + sizeof(PIPE_SUFFIX) + 1) \
		* sizeof(char));
	sprintf(pipeName, "%s%s", argv[0], PIPE_SUFFIX);
	
	managePipeArgs_t pipeArgs = {
		pipeName
	};

	if (pthread_create(&pipeThreadPtr, NULL, &managePipe, &pipeArgs) != 0) {
		perror("pthread_create");
		exit(1);
	}

	while (TRUE) {
		if (readLineArguments(argVector, 3, buffer, BUFFERSIZE) == -1) {
			displayError(ERR_LINEARGS);
			continue;
		}

		if (strcmp(argVector[0], "run") == 0) {
			if (numChildren == maxChildren) {
				waitNext(pidVector, stateVector);
				-- numChildren;
			}
			if ((pid = fork()) == -1) { /* error creating child */
				displayError(ERR_FORK);
				continue;
			}
			else if (pid == 0) { /* child process */
				char* args[] = {SOLVER_NAME, argVector[1]};
				execv(SOLVER_NAME, args);
				perror("execv");
				exit(1);
			}
			else /* parent process */
				++ numChildren;
		}
		else if (strcmp(argVector[0], "exit") == 0)	
			break;
		else { 
			displayError(ERR_COMMANDS); /* invalid command */
			continue;
		}
	}

	for (;numChildren > 0; -- numChildren)
		waitNext(pidVector, stateVector);
	
	while ((pidPtr = vector_popBack(pidVector)) != NULL) {
		pStatusPtr = vector_popBack(stateVector);
		printf("CHILD EXITED (PID=%li; ", (long) *pidPtr);
		if (exitedNormally(*pStatusPtr))
			puts("return OK)");
		else
			puts("return NOK)");
		free(pidPtr);
		free(pStatusPtr);
	}

	if (unlink(pipeName) != 0) {
		perror("unlink");
		exit(1);
	}

	vector_free(pidVector);
	vector_free(stateVector);
	puts("END.");
	return 0;
}