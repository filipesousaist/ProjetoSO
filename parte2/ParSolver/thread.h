#ifndef THREAD_H
#define THREAD_H 1

#include <pthread.h>
#include <stdio.h>
#include "coordinate.h"
#include "grid.h"


typedef struct thread{
	pthread_t* tidPtr;
	grid_t* privateGridPtr;
} thread_t;

void thread_alloc(thread_t* threadPtr, long width, long height, long depth);

void thread_getGrid();

#endif