/* SPDX-License-Identifier: BSD-3-Clause */

#include "./syscall.h"

int main(void)
{
	char buff[100];
	write(1, "Hello, world!\n", 14);
	long size = read(0, buff, 99);
	write(1, buff, size);
	exit(0);
	return 0;
}
