// SPDX-License-Identifier: BSD-3-Clause

#include <internal/mm/mem_list.h>
#include <internal/types.h>
#include <internal/essentials.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>

void *malloc(size_t size) {
	void *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) return NULL;
    if (mem_list_add(p, size) < 0) {
        munmap(p, size);
        return NULL;
    }
    return p;
}

void *calloc(size_t nmemb, size_t size) {
    // Becuase malloc uses mmap with MAP_ANONYMOUS, which initializes memory with zeroes
	return malloc(nmemb * size);
}

void free(void *ptr) {
    struct mem_list *item = mem_list_find(ptr);
    if (item == NULL) return;
    if (munmap(item->start, item->len) < 0) return;
    mem_list_del(ptr);
}

void *realloc(void *ptr, size_t size) {
    struct mem_list *item = mem_list_find(ptr);
    if (item == NULL) return NULL;
    void *p = mremap(item->start, item->len, size, MREMAP_MAYMOVE);
    if (p == MAP_FAILED) return NULL;
    item->start = p;
    item->len = size;
	return p;
}

void *reallocarray(void *ptr, size_t nmemb, size_t size) {
	return realloc(ptr, nmemb * size);
}
