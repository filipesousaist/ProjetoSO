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
#define SEQ_SOLVER_PATH "../CircuitRouter-SeqSolver/CircuitRouter-SeqSolver"
#define SEQ_SOLVER_NAME "CircuitRouter-SeqSolver"

enum {
	ERR_LINEARGS,
	ERR_COMMANDS,
	ERR_FORK
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
		case ERR_FORK:
			fputs("Aborted child", stderr);
			break;
	}
}

int main(int argc, char const *argv[]) {
	long maxChildren = -1; /* -1 means no limit of child processes */
	long numChildren = 0;
	pid_t* pid;
	int pStatus;
	
	char* argVector[3];
	char buffer[BUFFERSIZE];
	vector_t* pidVector = vector_alloc(PID_VECTOR_START);
	assert(pidVector);

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
			pid = (pid_t *) malloc(sizeof(pid_t));
			assert(pid);
			*pid = fork();
			char* args[] = {SEQ_SOLVER_NAME, argVector[1]};
			if (*pid == -1){
				displayError(ERR_FORK);
				continue;
			}
			
			if (*pid == 0)
				execv(SEQ_SOLVER_PATH, args);
			vector_pushBack(pidVector, pid);
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
	
	while ((pid = vector_popBack(pidVector)) != NULL) {
		while(waitpid(*pid, &pStatus, 0) == -1);
		
		printf("CHILD EXITED (PID=%li; ", (long) *pid);
		if (exitedNormally(pStatus))
			puts("return OK)");
		else
			puts("return NOK)");
		free(pid);
	}
	puts("END.");
	return 0;
}