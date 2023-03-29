// SPDX-License-Identifier: BSD-3-Clause

#include <fcntl.h>
#include <internal/syscall.h>
#include <stdarg.h>
#include <errno.h>

int open(const char *filename, int flags, ...) {
    va_list list;
    va_start(list, flags);
	int mode = va_arg(list, int);
	return syscall_errhandle(__NR_open, filename, flags, mode);
}
