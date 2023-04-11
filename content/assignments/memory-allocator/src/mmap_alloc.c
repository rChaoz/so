#include "mmap_alloc.h"
#include "brk_alloc.h"

static block_t *first;

// Finds the block right before the one with payload ptr. Returns NULL for first or ERROR if not found.
block_t *find_before(void *ptr) {
    if (first == NULL) return ERROR;
    if (PAYLOAD(first) == ptr) return NULL;
    block_t *block = first;
    while (block->next != NULL) {
        if (PAYLOAD(block->next) == ptr) return block;
        block = block->next;
    }
    return ERROR;
}

// Allocate memory with mmap
void *mmap_alloc(size_t size) {
    ALIGN(size);
    block_t *block = mmap(NULL, size + BLOCK_META_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    DIE(block == MAP_FAILED, "mmap failed");
    block->size = size;
    block->status = STATUS_MAPPED;
    // Add it to the list
    if (first == NULL) first = block;
    else {
        block->next = first;
        first = block;
    }
    return PAYLOAD(block);
}

// Reallocate memory with mremap (returns NULL if pointer is not alloc'd with mmap_alloc)
void *mmap_realloc(void *ptr, size_t size) {
    ALIGN(size);
    // Find block
    block_t *before = find_before(ptr);
    if (before == ERROR) return NULL;
    block_t *block = before == NULL ? first : before->next;
    block_t *after = block->next;

    // If new size is smaller than MMAP_THRESHOLD, use brk_alloc instead
    if (size < MMAP_THRESHOLD) {
        void *adr = brk_alloc(size);
        // Copy data to new location
        memcpy(adr, PAYLOAD(block), size);
        // Deallocate mmap memory and remove it from list
        DIE(munmap(block, block->size + BLOCK_META_SIZE)  == -1, "munmap failed");
        if (before != NULL) before->next = after;
        else first = after;
        return adr;
    }

    // Change its size with mremap
    block_t *new = mremap(block, block->size + BLOCK_META_SIZE, size, MREMAP_MAYMOVE);
    DIE(new == MAP_FAILED, "mremap failed");
    new->size = size;
    // Put new block in list in old one's place
    if (before != NULL) before->next = new;
    else first = new;
    new->next = after; // *shouldn't* be necessary as next value shouldn't be changed by mremap, but still
    // Return pointer to payload
    return PAYLOAD(new);
}

// Free a mmap block. Return 1 if block was found in list and freed, otherwise 0
int mmap_free(void *ptr) {
    block_t *before = find_before(ptr), *block;
    if (before == ERROR) return 0;
    else if (before == NULL) {
        block = first;
        first = first->next;
    } else {
        block = before->next;
        // remove it from list
        before->next = block->next;
    }
    // free block using munmap
    DIE(munmap(block, block->size + BLOCK_META_SIZE) == -1, "munmap failed");
    return 1; // success (was removed)
}