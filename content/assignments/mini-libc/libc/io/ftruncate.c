// SPDX-License-Identifier: BSD-3-Clause

#include <unistd.h>
#include <internal/syscall.h>
#include <errno.h>

int ftruncate(int fd, off_t length) {
	return syscall_errhandle(__NR_ftruncate, fd, length);
}
