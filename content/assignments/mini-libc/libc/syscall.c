// SPDX-License-Identifier: BSD-3-Clause

#include <internal/arch/x86_64/syscall_arch.h>
#include <stdarg.h>
#include <errno.h>

long syscall(long num, ...)
{
	va_list valist;
	long a, b, c, d, e, f;

	va_start(valist, num);
	a = va_arg(valist, long);
	b = va_arg(valist, long);
	c = va_arg(valist, long);
	d = va_arg(valist, long);
	e = va_arg(valist, long);
	f = va_arg(valist, long);
	va_end(valist);

	return __syscall(num, a, b, c, d, e, f);
}

int syscall_errhandle(long num, ...) {
    va_list valist;
    long a, b, c, d, e, f;

    va_start(valist, num);
    a = va_arg(valist, long);
    b = va_arg(valist, long);
    c = va_arg(valist, long);
    d = va_arg(valist, long);
    e = va_arg(valist, long);
    f = va_arg(valist, long);
    va_end(valist);

    int ret = (int) __syscall(num, a, b, c, d, e, f);
    if (ret < 0) {
        errno = -ret;
        return -1;
    } else return ret;
}