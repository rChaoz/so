// SPDX-License-Identifier: BSD-3-Clause

#include <unistd.h>
#include <internal/syscall.h>

int truncate(const char *path, off_t length) {
	return syscall_errhandle(__NR_truncate, path, length);
}
