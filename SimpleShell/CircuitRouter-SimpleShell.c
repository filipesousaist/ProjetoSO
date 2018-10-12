#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "../lib/commandlinereader.h"
#include "../lib/types.h"
#include "../lib/vector.h"

#define BUFFERSIZE 200
#define PID_VECTOR_START 20
#define SEQ_SOLVER_NAME "CircuitRouter-SeqSolver"

enum {
	ERR_LINEARGS,
	ERR_COMMANDS,
	ERR_FORK
}; 

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

int main(int argc, char const *argv[]) {
	long maxChildren = -1; /* -1 means no limit of child processes */
	long numChildren = 0;
	
	char* argVector[3];
	char buffer[BUFFERSIZE];
	pid_t pid;
	pid_t* pidPtr;
	int* pStatusPtr; /* process exit status */	
	vector_t* pidVector = vector_alloc(PID_VECTOR_START);
	assert(pidVector);
	vector_t* stateVector = vector_alloc(PID_VECTOR_START);
	assert(stateVector);

	if (argc == 2) {
		if (sscanf(argv[1],"%ld", &maxChildren) != 1) {
			fputs("Invalid arguments\n", stderr);
			exit(1);
		}
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
				char* args[] = {SEQ_SOLVER_NAME, argVector[1]};
				execv(SEQ_SOLVER_NAME, args);
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
	vector_free(pidVector);
	vector_free(stateVector);
	puts("END.");
	return 0;
}