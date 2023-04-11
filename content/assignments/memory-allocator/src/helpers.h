/* SPDX-License-Identifier: BSD-3-Clause */

#pragma once

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define DIE(assertion, call_description)                        \
    do {                                        \
        if (assertion) {                            \
            fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);        \
            perror(call_description);                    \
            exit(errno);                            \
        }                                    \
    } while (0)

#define MMAP_THRESHOLD (128 * 1024)
#define PAGE_SIZE ((size_t) getpagesize())
#define BLOCK_META_SIZE (sizeof (struct block_meta))
#define PAYLOAD(block) ((void*)(((void*)block) + BLOCK_META_SIZE))
#define ERROR ((void*) -1)
#define ALIGN(size) if (size % 8) size = (size / 8 + 1) * 8

/* Structure to hold memory block_t metadata */
typedef struct block_meta {
    size_t size; // 8 bytes
    int status;  // 4 bytes
    struct block_meta *next; // 8 bytes
} block_t;

/* Block metadata status values */
#define STATUS_FREE   0
#define STATUS_ALLOC  1
