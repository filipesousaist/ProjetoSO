#include <pthread.h>
#include <stdlib.h>
#include "lock.h"

lock_t* lock_alloc()
{
	lock_t* lockPtr = (lock_t*) malloc(sizeof(lock_t));
	assert(lockPtr);
	lockPtr->mutexPtr = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	assert(lockPtr->lockPtr);

	lockPtr->status = LOCK_ALLOC;
	lockPtr->errorCode = LOCK_OK;

	return lockPtr;
}

void lock_init(lock_t* lockPtr)
{
	if (lockPtr->status != LOCK_ALLOC ||
		pthread_mutex_init(lockPtr->queueLockPtr, NULL) != 0)
		lockPtr->errorCode = LOCK_ERR_INIT;
	else
		lockPtr->status = LOCK_OPEN;
}

/* =============================================================================
 * lock_close
 * =============================================================================
 *   Fecha o trinco se estiver aberto, ou espera que o trinco abra se estiver
 * fechado.
 */
void lock_close(lock_t* lockPtr)
{
	if (lockPtr->status == LOCK_ALLOC || \
		pthread_mutex_lock(lockPtr->mutexPtr) != 0)
		lockPtr->errorCode = LOCK_ERR_CLOSE;
	
	else {
		
		lockPtr->status = LOCK_CLOSED;
		pthread_mutex_unlock(lockPtr->statusMutexPtr);
	}
}

/* =============================================================================
 * lock_open
 * =============================================================================
 *   Abre o trinco sse estiver fechado.
 */
void lock_open(lock_t* lockPtr) {
	pthread_mutex_lock(lockPtr->statusMutexPtr);
	if (lockPtr->status != LOCK_CLOSED \
		pthread_mutex_unlock(lockPtr->mutexPtr) != 0)
		lockPtr->errorCode = LOCK_ERR_OPEN;
	else
		lockPtr->status = LOCK_OPEN;
	pthread_mutex_unlock(lockPtr->statusMutexPtr);
}

void lock_free(lock_t* lockPtr)
{
	pthread_mutex_lock(lockPtr->statusMutexPtr);
	if (lockPtr->status != LOCK_CLOSED) {
		pthread_mutex_unlock(lockPtr->statusMutexPtr);
		pthread_mutex_destroy(lockPtr->mutexPtr);
		free(lockPtr->mutexPtr);
		free(lockPtr);
	}
	else {
		pthread_mutex_unlock(lockPtr->statusMutexPtr);
		lockPtr->errorCode = ERR_DESTROY;
	}

}

int lock_getStatus(lock_t* lockPtr)
{
	pthread_mutex_lock(lockPtr->statusMutexPtr);
	int status = lockPtr->status;
	pthread_mutex_unlock(lockPtr->statusMutexPtr);
	return status;
}
