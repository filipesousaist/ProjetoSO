
#include <pthread.h>
#include "grid.h"

void thread_alloc(thread_t* threadPtr, grid_t* privateGridPtr)
{
	threadPtr -> tidPtr = NULL;
	threadPtr -> privateGridPtr = privateGridPtr;
}

void thread_exec(thread_t* threadPtr, void* (*func)(void*))
{
	pthread_t* tidPtr = (pthread_t*) malloc(sizeof(pthread_t));
	assert(tidPtr);
	if (pthread_create(tidPtr, 0, func, NULL) != 0)
	{
		perror("pthread_create");
		exit(1);
	}
	threadPtr -> tidPtr = tidPtr;
}