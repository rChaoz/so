// SPDX-License-Identifier: BSD-3-Clause

#include <sys/stat.h>
#include <fcntl.h>
#include <internal/syscall.h>

int stat(const char *restrict path, struct stat *restrict buf) {
	return syscall_errhandle(__NR_stat, path, buf);
}
