#ifndef _TIME_H
#define _TIME_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <internal/types.h>

typedef long int time_t;

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

int nanosleep(const struct timespec *req, const struct timespec *rem);

#ifdef __cplusplus
}
#endif

#endif //_TIME_H
