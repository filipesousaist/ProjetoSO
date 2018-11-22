/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * This code is an adaptation of the Lee algorithm's implementation originally included in the STAMP Benchmark
 * by Stanford University.
 *
 * The original copyright notice is included below.
 *
  * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) Stanford University, 2006.  All Rights Reserved.
 * Author: Chi Cao Minh
 *
 * =============================================================================
 *
 * Unless otherwise noted, the following license applies to STAMP files:
 *
 * Copyright (c) 2007, Stanford University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Stanford University nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 *
 * CircuitRouter-SeqSolver.c
 *
 * =============================================================================
 */


#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "lib/list.h"
#include "maze.h"
#include "router.h"
#include "thread.h"
#include "lib/timer.h"
#include "lib/types.h"

#define NUM_CHARS_EXT 4

enum param_types {
    PARAM_BENDCOST   = (unsigned char)'b',
    PARAM_XCOST      = (unsigned char)'x',
    PARAM_YCOST      = (unsigned char)'y',
    PARAM_ZCOST      = (unsigned char)'z',
    PARAM_NUMTHREADS = (unsigned char)'t'
};

enum param_defaults {
    PARAM_DEFAULT_BENDCOST   = 1,
    PARAM_DEFAULT_XCOST      = 1,
    PARAM_DEFAULT_YCOST      = 1,
    PARAM_DEFAULT_ZCOST      = 2,
    PARAM_DEFAULT_NUMTHREADS = -1
};

char* global_inputFile = NULL;
long global_params[256]; /* 256 = ascii limit */


/* =============================================================================
 * displayUsage
 * =============================================================================
 */
static void displayUsage (const char* appName){
    printf("Usage: %s [options]\n", appName);
    puts("\nOptions:                            (defaults)\n");
    printf("    b <INT>    [b]end cost          (%i)\n", PARAM_DEFAULT_BENDCOST);
    printf("    x <UINT>   [x] movement cost    (%i)\n", PARAM_DEFAULT_XCOST);
    printf("    y <UINT>   [y] movement cost    (%i)\n", PARAM_DEFAULT_YCOST);
    printf("    z <UINT>   [z] movement cost    (%i)\n", PARAM_DEFAULT_ZCOST);
    printf("    t <UINT>   number of [t]hreads  (required)\n");
    printf("    h          [h]elp message       (false)\n");
    exit(1);
}


/* =============================================================================
 * setDefaultParams
 * =============================================================================
 */
static void setDefaultParams (){
    global_params[PARAM_BENDCOST]   = PARAM_DEFAULT_BENDCOST;
    global_params[PARAM_XCOST]      = PARAM_DEFAULT_XCOST;
    global_params[PARAM_YCOST]      = PARAM_DEFAULT_YCOST;
    global_params[PARAM_ZCOST]      = PARAM_DEFAULT_ZCOST;
    global_params[PARAM_NUMTHREADS] = PARAM_DEFAULT_NUMTHREADS;
}


/* =============================================================================
 * parseArgs
 * =============================================================================
 */
static char* parseArgs (long argc, char* const argv[]) {
    
    long opt;

    opterr = 0;

    setDefaultParams();

    while ((opt = getopt(argc, argv, "hb:x:y:z:t:")) != -1) {
        switch (opt) {
            case 'b':
            case 'x':
            case 'y':
            case 'z':
            case 't':
                global_params[(unsigned char)opt] = atol(optarg);
                break;
            case '?':
            case 'h':
            default:
                opterr++;
                break;
        }
    }

    if (opterr || global_params['t'] <= 0) {
        displayUsage(argv[0]);
    }

    return argv[optind]; 
}


/* =============================================================================
 * createCoordinateLocksVector
 * =============================================================================
 */
static vector_t* createCoordinateLocksVector(grid_t* gridPtr) {
    long size = (gridPtr -> width) * (gridPtr -> height) * (gridPtr -> depth);
    vector_t* coordinateLocksVectorPtr = vector_alloc(size);

    for (long i = 0; i < size; ++ i) {
        pthread_mutex_t* newLock = malloc(sizeof(pthread_mutex_t));
        if (pthread_mutex_init(newLock, NULL) != 0) {
            perror("pthread_mutex_init");
            exit(1);
        }
        vector_pushBack(coordinateLocksVectorPtr, (void*) newLock);
    }

    return coordinateLocksVectorPtr;
}

/* =============================================================================
 * deleteCoordinateLocksVector
 * =============================================================================
 */
static void deleteCoordinateLocksVector(vector_t* coordinateLocksVectorPtr) {
    long size = vector_getSize(coordinateLocksVectorPtr);

    for (long i = 0; i < size; ++ i) {
        pthread_mutex_t* curLock = (pthread_mutex_t*) \
            vector_popBack(coordinateLocksVectorPtr);
        if (pthread_mutex_destroy(curLock) != 0) {
            perror("pthread_mutex_destroy");
            exit(1);
        }
        free(curLock);
    }

    vector_free(coordinateLocksVectorPtr);
}


/* =============================================================================
 * main
 * =============================================================================
 */
int main(int argc, char** argv) {
    /* Initialization */
    char* inputFileName = parseArgs(argc, (char** const)argv);
    if (inputFileName == NULL) {
        fputs("No file name specified\n", stderr);
        exit(1);
    }
    maze_t* mazePtr = maze_alloc();
    assert(mazePtr);

    /* Create or open output file */
    char* outputFileName = (char *) \
        malloc((strlen(inputFileName) + NUM_CHARS_EXT + 1) * sizeof(char));
    assert(outputFileName);
    sprintf(outputFileName, "%s.res", inputFileName);

    if (access(outputFileName, F_OK) == 0) { /* output file exists */
        char* oldFileName = (char *) \
            malloc((strlen(outputFileName) + NUM_CHARS_EXT + 1) * sizeof(char));
        assert(oldFileName);
        sprintf(oldFileName, "%s.old", outputFileName);

        if (rename(outputFileName, oldFileName) == -1) {
            perror("rename");
            exit(1);
        }
        free(oldFileName);
    }

    FILE *inputFilePtr, *outputFilePtr;
    if ((inputFilePtr = fopen(inputFileName, "r")) == NULL) {
        fprintf(stderr, "Invalid file name: %s\n", inputFileName);
        exit(1);
    } 
    outputFilePtr = fopen(outputFileName, "w");
    assert(outputFilePtr);

    long numPathToRoute = maze_read(mazePtr, inputFilePtr, outputFilePtr);
    fclose(inputFilePtr);
    router_t* routerPtr = router_alloc(global_params[PARAM_XCOST],
                                       global_params[PARAM_YCOST],
                                       global_params[PARAM_ZCOST],
                                       global_params[PARAM_BENDCOST]);
    assert(routerPtr);
    list_t* pathVectorListPtr = list_alloc(NULL);
    assert(pathVectorListPtr);

    TIMER_T startTime;
    TIMER_READ(startTime);

    /* Creation of the locks */
    pthread_mutex_t* queueLockPtr = \
        (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    assert(queueLockPtr);
    pthread_mutex_t* pathVectorListLockPtr = \
        (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    assert(pathVectorListLockPtr);
    vector_t* coordinateLocksVectorPtr = \
        createCoordinateLocksVector(mazePtr->gridPtr);
    assert(coordinateLocksVectorPtr);

    /* Initialization of the locks */
    if (pthread_mutex_init(queueLockPtr, NULL) != 0 || \
        pthread_mutex_init(pathVectorListLockPtr, NULL) != 0) {
        perror("pthread_mutex_init");
        exit(1);
    }

    /* Creation of the structure to pass to each thread */
    router_solve_arg_t routerArg = { \
        routerPtr, \
        mazePtr, \
        pathVectorListPtr, \
        queueLockPtr, \
        pathVectorListLockPtr, \
        coordinateLocksVectorPtr \
    };
    
    /* Initization of threads queue */
    queue_t* threadsQueuePtr = queue_alloc(global_params[PARAM_NUMTHREADS]);
    assert(threadsQueuePtr);
    thread_t* threadPtr;

    /* Creation of threads */
    for (int i = 0; i < global_params[PARAM_NUMTHREADS]; ++ i) 
    {
        threadPtr = thread_alloc();
        assert(threadPtr);
        queue_push(threadsQueuePtr, (void *) threadPtr);
        thread_exec(threadPtr, router_solve, (void*) &routerArg);
        if (threadPtr->errorCode != THREAD_OK) {
            thread_displayError(threadPtr);
            exit(1);
        }
    }

    /* Wait for all threads */
    while (! queue_isEmpty(threadsQueuePtr)) {  
        threadPtr = (thread_t *) queue_pop(threadsQueuePtr);
        thread_wait(threadPtr);
        if (threadPtr->errorCode != THREAD_OK) {
            thread_displayError(threadPtr);
            exit(1);
        }
    }

    /* Release all coordinate locks */
    list_iter_t listIt;
    list_iter_reset(&listIt, pathVectorListPtr);
    while (list_iter_hasNext(&listIt, pathVectorListPtr)) {
        vector_t* curPathVectorPtr = (vector_t*) \
            list_iter_next(&listIt, pathVectorListPtr);
        long numPaths = vector_getSize(curPathVectorPtr);
        for (int p = 0; p < numPaths; p++) {
            vector_t* curPointVectorPtr = vector_at(curPathVectorPtr, p);
            long size = vector_getSize(curPointVectorPtr);
            for (int pnt = 1; pnt < size - 1; pnt++) {
                long* gridPointPtr = (long*) vector_at(curPointVectorPtr, pnt);
                pthread_mutex_t* lock = (pthread_mutex_t*) vector_at( \
                    coordinateLocksVectorPtr, gridPointPtr - mazePtr->gridPtr->points);
                if (pthread_mutex_unlock(lock) != 0) {
                    perror("pthread_mutex_unlock");
                    exit(1);
                }
            }
        }
    }
    /* Free memory and destroy locks */
    if (pthread_mutex_destroy(queueLockPtr) != 0 || \
        pthread_mutex_destroy(pathVectorListLockPtr) != 0){
        perror("pthread_mutex_destroy");
        exit(1);
    } 
    deleteCoordinateLocksVector(coordinateLocksVectorPtr);
    free(queueLockPtr);
    free(pathVectorListLockPtr);
    queue_free(threadsQueuePtr);

    TIMER_T stopTime;
    TIMER_READ(stopTime);

    /* Caluclate number of paths routed */
    long numPathRouted = 0;
    list_iter_t it;
    list_iter_reset(&it, pathVectorListPtr);
    while (list_iter_hasNext(&it, pathVectorListPtr)) {
        vector_t* pathVectorPtr = (vector_t*)list_iter_next(&it, pathVectorListPtr);
        numPathRouted += vector_getSize(pathVectorPtr);
    }

    fprintf(outputFilePtr, "Paths routed    = %li\n", numPathRouted);
    fprintf(outputFilePtr, "Elapsed time    = %f seconds\n", \
        TIMER_DIFF_SECONDS(startTime, stopTime));

    /* Check solution and clean up */
    assert(numPathRouted <= numPathToRoute);

    bool_t status = maze_checkPaths(mazePtr, pathVectorListPtr, outputFilePtr);
    assert(status == TRUE);
    fputs("Verification passed.\n", outputFilePtr);

    fclose(outputFilePtr);
    free(outputFileName);

    maze_free(mazePtr);
    router_free(routerPtr);

    list_iter_reset(&it, pathVectorListPtr);
    while (list_iter_hasNext(&it, pathVectorListPtr)) {
        vector_t* pathVectorPtr = (vector_t*)list_iter_next(&it, pathVectorListPtr);
        vector_t* v;
        while ((v = vector_popBack(pathVectorPtr))) {
            /* v stores pointers to longs stored elsewhere; 
            no need to free them here */
            vector_free(v);
        }
        vector_free(pathVectorPtr);
    }
    list_free(pathVectorListPtr);

    exit(0);
}

/* =============================================================================
 *
 * End of CircuitRouter-SeqSolver.c
 *
 * =============================================================================
 */
