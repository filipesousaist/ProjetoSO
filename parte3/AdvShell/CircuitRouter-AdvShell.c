#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#include "CircuitRouter-AdvShell.h"
#include "../lib/commandlinereader.h"
#include "../lib/buffers.h"
#include "../lib/queue.h"
#include "../lib/types.h"
#include "../lib/vector.h"

/* CONSTANTS */

#define PID_VECTOR_START 20
#define SOLVER_NAME "CircuitRouter-ParSolver"
#define NUM_THREADS 4
#define MAX_N 10
#define SHELL_PIPE_NAME "/tmp/CircuitRouter-AdvShell.pipe"

#define INSTRUCTION_EXIT NULL

typedef struct instruction {
	char circuitName[CIRCUIT_MAX_NAME];
	char clientPipeName[CLIENT_MAX_PIPE_NAME];
} instruction_t;

/* MACROS */

#define TRY(EXPRESSION) {if ((EXPRESSION) < 0) {perror(NULL); exit(1);}}

/* STATIC FUNCTIONS */

static bool_t exitedNormally(int status) {
	return WIFEXITED(status) && (WEXITSTATUS(status) == 0);
}

static void displayError(int code) {
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

/* GLOBAL VARIABLES */

long maxChildren = -1; /* -1 means no limit of child processes */
long numChildren = 0;

int numInstructions = 0;
queue_t* instructionsQueuePtr;
pthread_mutex_t instructionsMutex;
pthread_cond_t instructionsCond;

vector_t* pidVector;
vector_t* statusVector;

bool_t finished = FALSE;


/* =============================================================================
 * main
 * =============================================================================
 */
int main(int argc, char const *argv[]) {
	/* Initialize circuit-related variables */
	instructionsQueuePtr = queue_alloc(-1);
	assert(instructionsQueuePtr);
	TRY(pthread_mutex_init(&instructionsMutex, NULL));
	TRY(pthread_cond_init(&instructionsCond, NULL));

	/* Initialize vectors to store the pid and exit status of each thread */
	pidVector = vector_alloc(PID_VECTOR_START);
	assert(pidVector);
	statusVector = vector_alloc(PID_VECTOR_START);
	assert(statusVector);

	/* Check if program arguments are valid */
	if ((argc != 1  && argc != 2) ||
		(argc == 2 && sscanf(argv[1],"%ld", &maxChildren) != 1)) {
		fputs("Invalid arguments\n", stderr);
		exit(1);
	}

	/* Create a thread to manage inputs from stdin */
	pthread_t stdinThread;
	TRY(pthread_create(&stdinThread, NULL, &shell_manageStdin, NULL));

	/* Create a thread to manage inputs from a pipe */
	pthread_t pipeThread;	
	TRY(pthread_create(&pipeThread, NULL, &shell_managePipe, NULL));

	/* Current thread will execute instructions */
	shell_executeInstructions();

	/* Wait for all children to finish */
	while ((numChildren --) > 0)
		shell_waitNext(pidVector, statusVector);
	
	/* Print exit status for all children */
	pid_t* pidPtr;
	int* statusPtr; /* process exit status */	
	while ((pidPtr = (pid_t*) vector_popBack(pidVector)) != NULL) {
		statusPtr = (int*) vector_popBack(statusVector);
		printf("CHILD EXITED (PID=%li; ", (long) *pidPtr);
		if (exitedNormally(*statusPtr))
			puts("return OK)");
		else
			puts("return NOK)");
		free(pidPtr);
		free(statusPtr);
	}

	/* Clean up */
	TRY(unlink(SHELL_PIPE_NAME));
	TRY(pthread_mutex_destroy(&instructionsMutex));
	TRY(pthread_cond_destroy(&instructionsCond));
	queue_free(instructionsQueuePtr);
	vector_free(pidVector);
	vector_free(statusVector);

	puts("END.");

	return 0;
}


/* =============================================================================
 * shell_executeInstructions
 * =============================================================================
 */
void shell_executeInstructions() {
	while (TRUE) {
		if (numChildren == maxChildren) { /* wait for next children to finish */
			shell_waitNext(pidVector, statusVector);
			-- numChildren;
		}

		/* Get next circuit name */	
		TRY(pthread_mutex_lock(&instructionsMutex));
		while (numInstructions == 0)
			TRY(pthread_cond_wait(&instructionsCond, &instructionsMutex));
		-- numInstructions;
		instruction_t* instructionPtr = \
			(instruction_t*) queue_pop(instructionsQueuePtr);
		TRY(pthread_mutex_unlock(&instructionsMutex));

		if (instructionPtr == INSTRUCTION_EXIT)
			return;

		int pid;
		if ((pid = fork()) == -1) { /* error creating child */
			displayError(ERR_FORK);
			free(instructionPtr);
			continue;
		}
		else if (pid == 0) { /* child process */
			char numThreadsStr[MAX_N];
			sprintf(numThreadsStr, "%i", NUM_THREADS);

			char* circName = instructionPtr->circuitName;
			char* pipeName = instructionPtr->clientPipeName;
			if (pipeName[0] == '\0') {
				TRY(execl(SOLVER_NAME, SOLVER_NAME, circName, \
					"-t", numThreadsStr, NULL));
			}
			else
				TRY(execl(SOLVER_NAME, SOLVER_NAME, circName, \
					"-t", numThreadsStr, "-p", pipeName, NULL));
		}
		/* parent process */
		++ numChildren;
		free(instructionPtr);
	}
}


/* =============================================================================
 * shell_manageStdin
 * =============================================================================
 */
void* shell_manageStdin(void* args) {
	char buffer[COMMAND_MAX_SIZE];
	char* argVector[3];

	while (TRUE) {
		if (readLineArguments(argVector, 3, buffer, COMMAND_MAX_SIZE, NULL) \
			== -1)
			displayError(ERR_LINEARGS);		
		else if (argVector[0] == NULL)
			displayError(ERR_COMMANDS);
		else if (strcmp(argVector[0], "run") == 0 && argVector[1] != NULL)
			shell_pushInstruction(argVector[1], NULL);
		else if (strcmp(argVector[0], "exit") == 0)	{
			shell_pushInstruction(INSTRUCTION_EXIT, NULL);
			pthread_exit(NULL);
		}
		else /* invalid command */
			displayError(ERR_COMMANDS);
	}
}


/* =============================================================================
 * shell_managePipe
 * =============================================================================
 */
void* shell_managePipe(void* argPtr) {
	/* Delete pipe if it exists */
	int result = access(SHELL_PIPE_NAME, F_OK);
	if (errno != 0 && errno != ENOENT) {
		perror("access");
		exit(1);
	}
	else if (result == 0)
		TRY(unlink(SHELL_PIPE_NAME));

	/* Create and open pipe */
	TRY(mkfifo(SHELL_PIPE_NAME, 0600));

	char message[MESSAGE_MAX_SIZE];
	char buffer[MESSAGE_MAX_SIZE];
	char* argVector[4];

	while (TRUE) {
		int fd;
		TRY(fd = open(SHELL_PIPE_NAME, O_RDONLY));
		TRY(read(fd, message, MESSAGE_MAX_SIZE));
		TRY(close(fd));

		if (readLineArguments(argVector, 4, buffer, MESSAGE_MAX_SIZE, \
			message) != -1 \
			&& strcmp(argVector[1], "run") == 0 \
			&& argVector[2] != NULL)

			shell_pushInstruction(argVector[2], argVector[0]);
		else { /* Invalid command */
			int clientFd;
			TRY(clientFd = open(argVector[0], O_WRONLY));
			char messageToClient[] = "Command not supported.";
			TRY(write(clientFd, messageToClient, strlen(messageToClient)));
			TRY(close(clientFd));
		}
	}
}


/* =============================================================================
 * shell_pushInstuction
 * =============================================================================
 */
void shell_pushInstruction(char* circuitName, char* clientPipeName) {
	instruction_t* newInstrPtr = (instruction_t*) malloc(sizeof(instruction_t));

	if (circuitName) {
		strcpy(newInstrPtr->circuitName, circuitName);
		if (clientPipeName)
			strcpy(newInstrPtr->clientPipeName, clientPipeName);
		else
			(newInstrPtr->clientPipeName)[0] = '\0';
	}
	else
		newInstrPtr = INSTRUCTION_EXIT;
	
	TRY(pthread_mutex_lock(&instructionsMutex));
	queue_push(instructionsQueuePtr, (void*) newInstrPtr);
	numInstructions ++;
	pthread_cond_signal(&instructionsCond);
	TRY(pthread_mutex_unlock(&instructionsMutex));
}


/* =============================================================================
 * shell_waitNext
 * =============================================================================
 */
void shell_waitNext()
{
	pid_t* pidPtr = (pid_t *) malloc(sizeof(pid_t));
	assert(pidPtr);
	int* statusPtr = (int *) malloc(sizeof(int));
	assert(statusPtr);

	TRY(*pidPtr = wait(statusPtr));
	vector_pushBack(pidVector, pidPtr);
	vector_pushBack(statusVector, statusPtr);
}