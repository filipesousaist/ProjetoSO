#include <pthread.h>


enum { /* estado dos trincos */
	LOCK_ALLOC,
	LOCK_CLOSED,
	LOCK_OPEN
};

enum { /* código de erro dos trincos */
	LOCK_OK,
	LOCK_ERR_INIT,
	LOCK_ERR_CLOSE,
	LOCK_ERR_OPEN
};

typedef struct lock {
	pthread_mutex_t* mutexPtr;
	pthread_mutex_t* statusMutexPtr;
	int status;
	int errorCode;
} lock_t;