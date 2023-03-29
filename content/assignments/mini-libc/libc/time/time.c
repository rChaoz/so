#include <internal/syscall.h>
#include <time.h>

unsigned int sleep(unsigned int seconds) {
    struct timespec time = {seconds, 0};
    return syscall_errhandle(__NR_nanosleep, &time, NULL);
}

int nanosleep(const struct timespec *req, const struct timespec *rem) {
    return syscall_errhandle(__NR_nanosleep, req, rem);
}