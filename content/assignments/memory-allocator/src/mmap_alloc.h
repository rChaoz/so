#ifndef MEMORY_ALLOCATOR_MMAP_ALLOC_H
#define MEMORY_ALLOCATOR_MMAP_ALLOC_H

#include "helpers.h"

void *mmap_alloc(size_t size);
void *mmap_realloc(void *ptr, size_t size);
int mmap_free(void *ptr);

#endif //MEMORY_ALLOCATOR_MMAP_ALLOC_H
