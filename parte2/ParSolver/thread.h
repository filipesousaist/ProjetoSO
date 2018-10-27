#ifndef THREAD_H
#define THREAD_H 1

#include <pthread.h>
#include <stdio.h>
#include "coordinate.h"
#include "grid.h"


enum { /* estado da tarefa */
	THREAD_INIT,
	THREAD_START,
	THREAD_FINISH
};

enum { /* código de erro da tarefa */
	THREAD_OK,
	THREAD_ERR_CREATE,
	THREAD_ERR_JOIN
};

typedef struct thread {
	pthread_t tid;
	int status; /* estado da tarefa */
	int errorCode; /* código de erro da tarefa */
} thread_t;

thread_t* thread_alloc();

void thread_free(thread_t* threadPtr);

void thread_exec(thread_t* threadPtr, void* (*func)(void*), void* argPtr);

void thread_wait(thread_t* threadPtr);

void thread_displayError(thread_t* threadPtr);

#endif