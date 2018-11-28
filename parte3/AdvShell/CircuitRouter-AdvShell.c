#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#include "CircuitRouter-AdvShell.h"
#include "../lib/clock.h"
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

typedef struct processData {
	pid_t pid;
	int status;
	TIME_T startTime;
	TIME_T finishTime;
} processData_t;

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
long startedChildren = 0;
long finishedChildren = 0;
long auxFinishedChildren = 0;
pthread_mutex_t numChildrenMutex;
pthread_cond_t numChildrenCond;

int numInstructions = 0;
queue_t* instructionsQueuePtr;
pthread_mutex_t instructionsMutex;
pthread_cond_t instructionsCond;

vector_t* processDataVectorPtr;

CLOCK_T mainClock;
TIME_T globalFinishTime;

sigset_t childSigSet; /* Auxiliary set which will only contain SIGCHLD */


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
	processDataVectorPtr = vector_alloc(PID_VECTOR_START);
	assert(processDataVectorPtr);


	/* Check if program arguments are valid */
	if ((argc != 1  && argc != 2) ||
		(argc == 2 && sscanf(argv[1],"%ld", &maxChildren) != 1)) {
		fputs("Invalid arguments\n", stderr);
		exit(1);
	}

	/* Block signals from children; all "child" threads will inherit this 
	behaviour */
	TRY(sigemptyset(&childSigSet));
	TRY(sigaddset(&childSigSet, SIGCHLD));
	TRY(pthread_sigmask(SIG_BLOCK, &childSigSet, NULL));

	/* Create a thread to manage signals */
	pthread_t signalsThread;
	TRY(pthread_create(&signalsThread, NULL, &shell_manageSignals, NULL));

	/* Create a thread to manage inputs from stdin */
	pthread_t stdinThread;
	TRY(pthread_create(&stdinThread, NULL, &shell_manageStdin, NULL));

	/* Create a thread to manage inputs from a pipe */
	pthread_t pipeThread;	
	TRY(pthread_create(&pipeThread, NULL, &shell_managePipe, NULL));

	/* Current thread will execute instructions */
	shell_executeInstructions();

	/* Wait for all children to finish */
	shell_waitForAll();
	
	/* Print exit status for all children */
	while (vector_getSize(processDataVectorPtr) > 0) {
		processData_t* dataPtr = \
			(processData_t*) vector_popBack(processDataVectorPtr);

		char* statusStr = (exitedNormally(dataPtr->status) ? "OK" : "NOK");
		double timeDiff = \
			TIME_DIFF_SECONDS(dataPtr->startTime, dataPtr->finishTime);

		printf("CHILD EXITED (PID=%li; return %s; %lf s)\n", \
				(long) dataPtr->pid, statusStr, timeDiff);

		free(dataPtr);
	}

	/* Clean up */
	TRY(unlink(SHELL_PIPE_NAME));
	TRY(pthread_mutex_destroy(&instructionsMutex));
	TRY(pthread_cond_destroy(&instructionsCond));
	queue_free(instructionsQueuePtr);
	vector_free(processDataVectorPtr);

	puts("END.");

	return 0;
}


/* =============================================================================
 * shell_executeInstructions
 * =============================================================================
 */
void shell_executeInstructions() {
	while (TRUE) {
		if (maxChildren != -1) { /* There is a maximum number of children */
			TRY(pthread_mutex_lock(&numChildrenMutex));
			while (startedChildren - finishedChildren >= maxChildren)
				TRY(pthread_cond_wait(&numChildrenCond, &numChildrenMutex));
			TRY(pthread_mutex_unlock(&numChildrenMutex));
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
		else { /* parent process */
			/* Create process data */
			processData_t* dataPtr = \
				(processData_t*) malloc(sizeof(processData_t));
			dataPtr->pid = pid;
			CLOCK_READ(mainClock, &(dataPtr->startTime));
			vector_pushBack(processDataVectorPtr, dataPtr);

			++ startedChildren;

			free(instructionPtr);
		}
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
		bzero(message, MESSAGE_MAX_SIZE);
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
 * shell_manageSignals
 * =============================================================================
 */
void* shell_manageSignals(void* args) {
	/* Replace signal handler for SIGCHLD */
	struct sigaction action;
	action.sa_handler = &shell_signalHandler;
	TRY(sigaction(SIGCHLD, &action, NULL));

	/* Unblock signals from children */
	TRY(pthread_sigmask(SIG_UNBLOCK, &childSigSet, NULL));

	while (TRUE) {
		pause(); /* Allow other threads to get CPU time while no signals come */
		pthread_mutex_lock(&numChildrenMutex);
		finishedChildren = auxFinishedChildren;
		pthread_cond_signal(&numChildrenCond);
		pthread_mutex_unlock(&numChildrenMutex);
	}
}


/* =============================================================================
 * shell_signalHandler
 * =============================================================================
 */
void shell_signalHandler(int sig) {
	CLOCK_READ(mainClock, &globalFinishTime);

	/* Check which processes might have ended */
	pid_t pid;
	int status;	
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {	
		/* Search for process data in the vector */
		processData_t* processDataPtr;
		for (int i = 0; ; i ++) {
			processDataPtr = \
				(processData_t*) vector_at(processDataVectorPtr, i);
			if (processDataPtr->pid == pid)
				break;
		}
		processDataPtr->status = status;
		processDataPtr->finishTime = globalFinishTime;

		auxFinishedChildren ++;
	}
	if (pid < 0 && errno != ECHILD) {
		char errorMessage[] = "Error: waitpid";
		write(1, errorMessage, sizeof(errorMessage));
		_exit(1);
	}
	/* Else, result was 0 or ECHILD -> no more children left to wait for */
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
void shell_waitForAll()
{
	TRY(pthread_mutex_lock(&numChildrenMutex));
	while (startedChildren > finishedChildren)
		TRY(pthread_cond_wait(&numChildrenCond, &numChildrenMutex));
	TRY(pthread_mutex_unlock(&numChildrenMutex));
}