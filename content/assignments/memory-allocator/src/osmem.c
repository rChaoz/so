// SPDX-License-Identifier: BSD-3-Clause

#include "osmem.h"
#include "helpers.h"
#include "mmap_alloc.h"
#include "brk_alloc.h"

void *os_malloc(size_t size)
{
	if (size == 0)
		return NULL;
	else if (size + BLOCK_META_SIZE >= MMAP_THRESHOLD)
		return mmap_alloc(size);
	else
		return brk_alloc(size);
}

void os_free(void *ptr)
{
	if (ptr == NULL)
		return;
	// first, test if block is large (mmapped)
	if (mmap_free(ptr))
		return;
	// if not, free with brk
	brk_free(ptr);
}

void *os_calloc(size_t nmemb, size_t size)
{
	size *= nmemb;
	if (size == 0)
		return NULL;
	if (size + BLOCK_META_SIZE >= PAGE_SIZE)
		return mmap_alloc(size); // mmap already sets memory to 0

	void *ptr = brk_alloc(size);

	memset(ptr, 0, size);
	return ptr;
}

void *os_realloc(void *ptr, size_t size)
{
	if (ptr == NULL) {
		return os_malloc(size);
	} else if (size == 0) {
		os_free(ptr);
		return NULL;
	}

	// test if ptr is mmapped
	void *r = mmap_realloc(ptr, size);

	if (r)
		return r;

	// next, try the brk realloc
	return brk_realloc(ptr, size);
}
