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
#include "../lib/types.h"
#include "../lib/vector.h"
#include "../lib/queue.h"


/* CONSTANTS */

#define BUFFERSIZE 200
#define PID_VECTOR_START 20
#define SOLVER_NAME "CircuitRouter-SeqSolver"
#define PIPE_NAME "CircuitRouter-AdvShell.pipe"


/* MACROS */

#define TRY(EXPRESSION) if ((EXPRESSION) < 0) {perror(NULL); exit(1);}
#define ASSERT(EXPRESSION) assert((EXPRESSION));

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
	ASSERT(instructionsQueuePtr = queue_alloc(-1));
	TRY(pthread_mutex_init(&instructionsMutex, NULL));
	TRY(pthread_cond_init(&instructionsCond, NULL));

	/* Initialize vectors to store the pid and exit status of each thread */
	ASSERT(pidVector = vector_alloc(PID_VECTOR_START));
	ASSERT(statusVector = vector_alloc(PID_VECTOR_START));

	/* Check if program arguments are valid */
	if (argc != 2 || sscanf(argv[1],"%ld", &maxChildren) != 1) {
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
	TRY(unlink(PIPE_NAME));
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
		char* instr = (char*) queue_pop(instructionsQueuePtr);
		-- numInstructions;
		TRY(pthread_mutex_unlock(&instructionsMutex));

		if (instr == NULL)
			return;

		int pid;
		if ((pid = fork()) == -1) { /* error creating child */
			displayError(ERR_FORK);
			free(instr);
			continue;
		}
		else if (pid == 0) /* child process */
			TRY(execl(SOLVER_NAME, SOLVER_NAME, instr, NULL));
		/* parent process */
		++ numChildren;
		free(instr);
	}
}


/* =============================================================================
 * shell_manageStdin
 * =============================================================================
 */
void* shell_manageStdin(void* args) {
	char buffer[BUFFERSIZE];
	char* argVector[3];

	while (TRUE) {
		if (readLineArguments(argVector, 3, buffer, BUFFERSIZE, NULL) == -1)
			displayError(ERR_LINEARGS);		
		else if (argVector[0] == NULL)
			displayError(ERR_COMMANDS);
		else if (strcmp(argVector[0], "run") == 0 && argVector[1] != NULL)
			shell_pushInstruction(argVector[1]);
		else if (strcmp(argVector[0], "exit") == 0)	{
			shell_pushInstruction(NULL);
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
	int result = access(PIPE_NAME, F_OK);
	if (errno != 0 && errno != ENOENT) {
		perror("access");
		exit(1);
	}
	else if (result == 0)
		TRY(unlink(PIPE_NAME));

	/* Create and open pipe */
	TRY(mkfifo(PIPE_NAME, 0755));
	int fd;
	TRY(fd = open(PIPE_NAME, O_RDONLY));

	char command[BUFFERSIZE];
	char buffer[BUFFERSIZE];
	char* argVector[3];

	while (TRUE) {
		read(fd, command, BUFFERSIZE);
		if (readLineArguments(argVector, 3, buffer, BUFFERSIZE, command) == -1) {
			displayError(ERR_LINEARGS);
			continue;
		}

		if (strcmp(argVector[0], "run") == 0 && argVector[1] != NULL)
			shell_pushInstruction(argVector[1]);
		else {
			displayError(ERR_COMMANDS); /* invalid command */
			continue;
		}
	}
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


/* =============================================================================
 * shell_pushInstuction
 * =============================================================================
 */
void shell_pushInstruction(char* instr) {
	char* newInstr;
	if (instr) {
		newInstr = (char*) malloc((strlen(instr) + 1) * sizeof(char));
		strcpy(newInstr, instr);
	}
	else
		newInstr = NULL;
	
	TRY(pthread_mutex_lock(&instructionsMutex));
	queue_push(instructionsQueuePtr, (void*) newInstr);
	numInstructions ++;
	pthread_cond_signal(&instructionsCond);
	TRY(pthread_mutex_unlock(&instructionsMutex));
}