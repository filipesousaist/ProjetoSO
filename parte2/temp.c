/* =============================================================================
 * grid_addPath_Ptr
 * =============================================================================
 */
bool_t grid_addPath_Ptr (grid_t* gridPtr, vector_t* pointVectorPtr, \
    vector_t* coordinateLocksVectorPtr) {
    bool_t result = TRUE;
    long n = vector_getSize(pointVectorPtr);
    long i;
    long* gridPointPtr;

    for (i = 1; i < (n-1); i++) {
        gridPointPtr = (long*) vector_at(pointVectorPtr, i);
        pthread_mutex_t* lock = getLockRef(gridPointPtr, gridPtr, \
            coordinateLocksVectorPtr);
        if (pthread_mutex_lock(lock) != 0) {
            perror("pthread_mutex_lock");
            exit(1);
        }
        if (*gridPointPtr == GRID_POINT_FULL) {
            result = FALSE;
        }
        else
            *gridPointPtr = GRID_POINT_FULL;
        if (pthread_mutex_unlock(lock) != 0) {
            perror("pthread_mutex_lock");
            exit(1);
        }
        if (! result)
            break;
    }

    if (! result) {
        for (i--; i > 0; i--) { 
            gridPointPtr = (long*) vector_at(pointVectorPtr, i);
            pthread_mutex_t* lock = getLockRef(gridPointPtr, gridPtr, \
                coordinateLocksVectorPtr);
            if (pthread_mutex_lock(lock) != 0) {
                perror("pthread_mutex_lock");
                exit(1);
            }
            *gridPointPtr = GRID_POINT_EMPTY;
            if (pthread_mutex_unlock(lock) != 0) {
                perror("pthread_mutex_lock");
                exit(1);
            }
        }
    }

    return result;
}