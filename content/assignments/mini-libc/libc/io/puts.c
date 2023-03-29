#include <string.h>
#include <stdio.h>
#include <internal/syscall.h>

static const char *newline = "\n";

int puts(const char *str) {
    if (syscall(1, 1, str, strlen(str)) < 0) return EOF;
    if (syscall(1, 1, newline, 1) < 0) return EOF;
    return 0;
}