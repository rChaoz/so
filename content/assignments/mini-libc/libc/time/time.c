#include <internal/syscall.h>
#include <time.h>

unsigned int sleep(unsigned int seconds) {
    struct timespec time = {seconds, 0};
    struct timespec rem = {0, 0};
    if (syscall_errhandle(__NR_nanosleep, &time, &rem) < 0) return rem.tv_sec;
    else return 0;
}

int nanosleep(const struct timespec *req, const struct timespec *rem) {
    return syscall_errhandle(__NR_nanosleep, req, rem);
}