#ifndef MEMORY_ALLOCATOR_BRK_ALLOC_H
#define MEMORY_ALLOCATOR_BRK_ALLOC_H

#include "helpers.h"

void *brk_alloc(size_t size);
void *brk_realloc(void *ptr, size_t size);
int brk_free(void *ptr);

#endif //MEMORY_ALLOCATOR_BRK_ALLOC_H
