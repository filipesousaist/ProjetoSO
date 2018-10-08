#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "../lib/commandlinereader.h"
#include "../lib/types.h"

#define BUFFERSIZE 200
#define PID_VECTOR_STEP 20
#define SEQ_SOLVER_PATH "../CircuitRouter-SeqSolver/CircuitRouter-SeqSolver"
#define SEQ_SOLVER_NAME "CircuitRouter-SeqSolver"

enum {
	ERR_LINEARGS,
	ERR_COMMANDS
};

bool_t exitedNormally(int status) {
	return WIFEXITED(status) && (WEXITSTATUS(status) == 0);
}

void displayError(int code) {
	switch (code) {
		case ERR_LINEARGS:
			fputs("Error reading line arguments", stderr);
			break;
		case ERR_COMMANDS:
			fputs("Invalid commands", stderr);
			break;
	}
}

int main(int argc, char const *argv[]) {
	long maxChildren = -1; /* -1 means no limit of child processes */
	long numChildren = 0;
	long totalChildren = 0;
	
	char* argVector[3];
	char buffer[BUFFERSIZE];
	pid_t* pidVector = (pid_t*) malloc(PID_VECTOR_STEP * sizeof(pid_t));
	assert(pidVector);
	pid_t pid;
	int pStatus;

	if (argc == 2) {
		if (sscanf(argv[1],"%ld", &maxChildren)!=1) {
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
				wait(&pStatus);
				-- numChildren;
			}
			pid = fork();
			char* args[] = {SEQ_SOLVER_NAME, argVector[1]};
			if (pid == 0)
				execv(SEQ_SOLVER_PATH, args);
			pidVector[totalChildren ++] = pid;
			++ numChildren;
		}

		else if (strcmp(argVector[0], "exit") == 0) {		
			break;
		}

		else { 
			displayError(ERR_COMMANDS); /* invalid command */
			continue;
		}
	}
	
	for (long i = 0; i < totalChildren; ++ i) {
		waitpid(pidVector[i], &pStatus, 0);
		
		printf("CHILD EXITED (PID=%li; ", (long) pidVector[i]);
		if (exitedNormally(pStatus))
			puts("return OK)");
		else
			puts("return NOK)");
	}
	free(pidVector);
	puts("END.");
	return 0;
}