#ifndef CLOCK_H
#define CLOCK_H 1


#include <sys/time.h>

#define TIME_T struct timespec
#define CLOCK_T clockid_t

#define CLOCK_READ(clock, timePtr) clock_gettime(clock, timePtr);

#define TIME_DIFF_SECONDS(start, stop) \
    (((double)(stop.tv_sec)  + (double)(stop.tv_nsec / 1000000000.0)) - \
     ((double)(start.tv_sec) + (double)(start.tv_nsec / 1000000000.0)))

#endif /* CLOCK_H */


/* =============================================================================
 *
 * End of clock.h
 *
 * =============================================================================
 */