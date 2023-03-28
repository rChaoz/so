// SPDX-License-Identifier: BSD-3-Clause

#include <string.h>

char *strcpy(char *destination, const char *source) {
    char *dest = destination;
    while (*source) *dest++ = *source++;
    *dest = 0;
    return destination;
}

char *strncpy(char *destination, const char *source, size_t len) {
    char *dest = destination;
    while (len-- && *source) *dest++ = *source++;
    ++len;
    while (len--) *dest++ = 0;
    return destination;
}

char *strcat(char *destination, const char *source) {
    char *dest = destination;
    while (*dest) ++dest;

    while (*source) *dest++ = *source++;
    *dest = 0;

    return destination;
}

char *strncat(char *destination, const char *source, size_t len) {
    char *dest = destination;
    while (*dest) ++dest;

    while (len-- && *source) *dest++ = *source++;
    *dest = 0;

    return destination;
}

int strcmp(const char *str1, const char *str2) {
    while (*str1 == *str2) {
        if (!*str1) return 0;
        ++str1, ++str2;
    }
    return *str1 - *str2;
}

int strncmp(const char *str1, const char *str2, size_t len) {
    ++len;
    while (--len && *str1 == *str2) {
        if (!*str1 || len == 1) return 0;
        ++str1, ++str2;
    }
    return *str1 - *str2;
}

size_t strlen(const char *str) {
    size_t n = 0;
    while (*str++) ++n;
    return n;
}

const char *strchr(const char *str, int c) {
    while (*str != c) {
        if (!*str) return NULL;
        ++str;
    }
    return str;
}

const char *strrchr(const char *str, int c) {
    const char *first = str;
    while (*str) ++str;
    --str;

    while (*str != c) {
        if (str == first) return NULL;
        --str;
    }

    return str;
}

const char *strstr(const char *haystack, const char *needle) {
    size_t len = strlen(needle);
    const char *last = haystack + strlen(haystack) - len;
    if (last < haystack) return NULL;

    while (strncmp(haystack, needle, len) != 0) if (haystack++ == last) return NULL;
    return haystack;
}

const char *strrstr(const char *haystack, const char *needle) {
    size_t len = strlen(needle);
    const char *p = haystack - len;
    if (p < haystack) return NULL;

    while (strncmp(p, needle, len) != 0) if (p-- == haystack) return NULL;
    return p;
}

void *memcpy(void *destination, const void *source, size_t num) {
    char *d = destination;
    const char *s = source;
    while (num--) *d++ = *s++;
    return destination;
}

void *memmove(void *destination, const void *source, size_t num) {
    const char *s;
    char *d;
    if (destination > source) {
        s = source + num - 1;
        d = destination + num - 1;
        while (num--) *d-- = *s--;
    } else if (destination < source) {
        d = destination;
        s = source;
        while (num--) *d++ = *s++;
    } // if (destination == source), nothing needs to be done
    return destination;
}

int memcmp(const void *ptr1, const void *ptr2, size_t num) {
    const char *p1 = ptr1;
    const char *p2 = ptr2;
    while (num && *p1 == *p2) {
        if (!--num) return 0;
        ++p1, ++p2;
    }
    return *p1 - *p2;
}

void *memset(void *source, int value, size_t num) {
    unsigned char *s = source;
    unsigned char v = value;
    while (num--) *s++ = v;
    return source;
}
