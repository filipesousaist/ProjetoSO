#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "thread.h"

thread_t* thread_alloc()
{
	thread_t* threadPtr = (thread_t*) malloc(sizeof(thread_t));
	threadPtr->errorCode = THREAD_OK;
	threadPtr->status = THREAD_INIT;
	return threadPtr;
}

void thread_free(thread_t* threadPtr)
{
	free(threadPtr);
}

void thread_exec(thread_t* threadPtr, void* (*func)(void*), void* argPtr)
{
	if (pthread_create(&(threadPtr->tid), NULL, func, argPtr) != 0)
		threadPtr->errorCode = THREAD_ERR_CREATE;
	else
		threadPtr->status = THREAD_START;
}

void thread_wait(thread_t* threadPtr)
{
	if (threadPtr->status != THREAD_START || \
		pthread_join(threadPtr->tid, NULL) != 0)
		threadPtr->errorCode = THREAD_ERR_JOIN;
	else
		threadPtr->status = THREAD_FINISH;
}	

void thread_displayError(thread_t* threadPtr)
{
	fprintf(stderr, "Thread #%lu:\n", (unsigned long) threadPtr->tid);
	switch (threadPtr->errorCode) {
		case THREAD_ERR_CREATE:
			perror("pthread_create"); break;
		case THREAD_ERR_JOIN:
			switch (threadPtr->status) {
				case THREAD_INIT:
					fputs("Error: Tried to join before thread started.\n",
						stderr); break;
				case THREAD_START:
					perror("pthread_join"); break;
				case THREAD_FINISH:
					fputs("Error: Tried to join after thread finished.\n",
						stderr); break;
			} break;		
		default:
			fputs("No error.\n", stderr);
	}
}