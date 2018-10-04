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

bool_t exitedNormally(int status)
{
	return WIFEXITED(status) && (WEXITSTATUS(status) == 0);
}

void displayError(int code)
{
	switch (code) {
		case 0:
			puts("Error reading line arguments");
			break;
		case 1:
			puts("Invalid file name");
			break;
		case 2:
			puts("Invalid commands");
			break;
	}
}

int main(int argc, char const *argv[])
{
	long maxChildren = -1; /* -1 means no limit of child processes */
	long numChildren = 0;
	long totalChildren = 0;
	
	if (argc == 2){
		if(sscanf(argv[1],"%ld", &maxChildren)!=1){
			printf("Invalid arguments\n");
			exit(1);
		}
	}

	char* argVector[3];
	char buffer[BUFFERSIZE];
	pid_t* pidVector = (pid_t*) malloc(PID_VECTOR_STEP * sizeof(pid_t));
	assert(pidVector);
	pid_t pid;
	int pStatus;

	while (TRUE) {
		if (readLineArguments(argVector, 3, buffer, BUFFERSIZE) == -1) {
			displayError(0);
			continue;
		}

		if (strcmp(argVector[0], "run") == 0) {
			if (argVector[1] == NULL || (access(argVector[1], R_OK) == -1)) { 
				displayError(1); /* invalid file name */
				continue;
			}
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
			for (long i = 0; i < totalChildren; ++ i)
			{
				waitpid(pidVector[i], &pStatus, 0);
				printf("%li\n", (long) pidVector[i]);
			}
			exit(0);
		}
		else { 
			displayError(2); /* invalid command */
			continue;
		}

	}

	free(pidVector);
	return 0;
}