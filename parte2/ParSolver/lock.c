#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
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
	closeStatus(lockPtr);

	if (lockPtr->status == LOCK_ALLOC && \
		pthread_mutex_init(lockPtr->queueLockPtr, NULL) == 0) {
		lockPtr->status = LOCK_OPEN;
		openStatus(lockPtr);
	}
	else
		lockPtr->errorCode = LOCK_ERR_INIT;
}

/* =============================================================================
 * lock_close
 * =============================================================================
 *   Fecha o trinco se estiver aberto, ou espera que o trinco abra se estiver
 * fechado.
 */
void lock_close(lock_t* lockPtr){
	closeStatus(lockPtr);

	if (lockPtr->status != LOCK_ALLOC) {
		openStatus(lockPtr);

		if (pthread_mutex_lock(lockPtr->mutexPtr) == 0) {
			closeStatus(lockPtr);
			lockPtr->status = LOCK_CLOSED;
			openStatus(lockPtr);
		}
		else {
			closeStatus(lockPtr);
			lockPtr->errorCode = LOCK_ERR_CLOSE;
		}
	}
	else
		lockPtr->errorCode = LOCK_ERR_CLOSE;
}

/* =============================================================================
 * lock_open
 * =============================================================================
 *   Abre o trinco sse estiver fechado.
 */
void lock_open(lock_t* lockPtr) {
	closeStatus(lockPtr);

	if (lockPtr->status == LOCK_CLOSED && \
		pthread_mutex_unlock(lockPtr->mutexPtr) == 0) {
		lockPtr->status = LOCK_OPEN;
		openStatus(lockPtr);
	}
	else
		lockPtr->errorCode = LOCK_ERR_OPEN;
}

/* =============================================================================
 * lock_free
 * =============================================================================
 *   Liberta a memória associada ao trinco, mas este tem de estar aberto.
 */
void lock_free(lock_t* lockPtr) {
	closeStatus(lockPtr);

	if (lockPtr->status != LOCK_CLOSED) {
		pthread_mutex_destroy(lockPtr->mutexPtr);
		pthread_mutex_destroy(lockPtr->statusMutexPtr);
		free(lockPtr->mutexPtr);
		free(lockPtr->statusMutexPtr);
		free(lockPtr);
	}
	else
		lockPtr->errorCode = LOCK_ERR_DESTROY;
		
}

/* =============================================================================
 * lock_getStatus
 * =============================================================================
 *   (DEBUGGING) Obtém o estado do lock. O mutex da status tem de estar fechado.
 */
int lock_getStatus(lock_t* lockPtr)
{
	return lockPtr->status;
}


static void closeStatus(lock_t* lockPtr) {
	if (pthread_mutex_lock(lockPtr->statusMutexPtr) != 0)
	{
		fputs("<statusMutexPtr>\n", stderr);
		perror("pthread_mutex_lock");
		exit(1);
	}
}

static void tryCloseStatus(lock_t* lockPtr) {
	if (pthread_mutex_trylock(lockPtr->statusMutexPtr) != 0 && errno != EBUSY)
	{
		fputs("<statusMutexPtr>\n", stderr);
		perror("pthread_mutex_trylock");
		exit(1);
	}
}

static void openStatus(lock_t* lockPtr) {
	if (pthread_mutex_unlock(lockPtr->statusMutexPtr))
	{
		fputs("<statusMutexPtr>\n", stderr);
		perror("pthread_mutex_unlock");
		exit(1);
	}
}

void lock_checkError(lock_t* lockPtr){
	tryCloseStatus(lockPtr);
	if (lockPtr->errorCode != LOCK_OK) {
		lock_displayError(lockPtr);
		exit(1);
	}
	openStatus(lockPtr);
}

void lock_displayError(lock_t* lockPtr) {
	switch (lockPtr->errorCode) {
		case LOCK_ERR_INIT:
			switch (lockPtr->status) {
				case LOCK_OPEN:
					fputs("Error: Tried to init open lock.\n", stderr); break;
				case LOCK_CLOSED:
					fputs("Error: Tried to init closed lock.\n", stderr); break;
				default:
					perror("pthread_mutex_init");
			} break;
		case LOCK_ERR_CLOSE:
			switch (lockPtr->status) {
				case LOCK_ALLOC:
					fputs("Error: Tried to close uninitialized lock.\n", \
						stderr); break;
				default:
					perror("pthread_mutex_unlock");
			} break;
		case LOCK_ERR_OPEN:
			switch (lockPtr->status) {
				case LOCK_ALLOC:
					fputs("Error: Tried to open uninitialized lock.\n", \
						stderr); break;
				case LOCK_OPEN:
					fputs("Error: Tried to open open lock.\n", stderr); break;
				default:
					perror("pthread_mutex_open");
			} break;
		case LOCK_ERR_DESTROY:
			switch (lockPtr->status) {
				case LOCK_CLOSE:
					fputs("Error: Tried to destroy closed lock.\n", \
						stderr); break;
				default:
					perror("pthread_mutex_destroy");
			} break;
	}
}