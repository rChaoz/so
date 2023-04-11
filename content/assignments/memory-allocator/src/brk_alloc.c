#include "brk_alloc.h"

#define BLOCK(payload) ((block_t*)(((void*)payload) - BLOCK_META_SIZE))

static void *initial = NULL;
static block_t *first = NULL, *last = NULL;

// Allocate another 128KB for the heap
static block_t *increase_heap() {
    // allocate 128KB
    void *r = sbrk(MMAP_THRESHOLD);
    DIE(r == ERROR, "sbrk failed");
    // if this is the first allocation, align memory block
    if (initial == NULL) {
        initial = r;
        long align = ((long) r) % 8;
        if (align) align = 8 - align;
        first = last = r + align;
        first->size = MMAP_THRESHOLD - BLOCK_META_SIZE - align;
        first->next = NULL;
        first->status = STATUS_FREE;
        return first;
    }
    // otherwise, previous block is aligned and has size multiple of 8, so new block is also aligned

    // if last block is free, just increase its size
    if (last->status == STATUS_FREE) {
        last->size += MMAP_THRESHOLD;
        return last;
    }
    // else create new block
    block_t *block = r;
    block->size = MMAP_THRESHOLD - BLOCK_META_SIZE;
    block->next = NULL;
    block->status = STATUS_FREE;
    // add it to end of list
    last->next = block;
    last = block;

    return block;
}

// Find first memory block that has at least 'size' bytes available
static block_t *find_free_block(size_t size) {
    block_t *block = first;
    while (block) {
        if (block->status == STATUS_FREE && block->size >= size) return block;
        block = block->next;
    }
    return NULL;
}

// split a free memory block if possible, return block of at least 'size' bytes
// block must already have at least 'size' bytes. block will be marked as 'allocated'
static void *split_block(block_t *block, size_t size) {
    block->status = STATUS_ALLOC;
    // align size to 8 bytes
    if (size % 8) size = (size / 8 + 1) * 8;
    // check if we can split
    if (block->size <= size + BLOCK_META_SIZE) return block; // no split
    // split block
    block_t *new = PAYLOAD(block) + size;
    new->size = block->size - BLOCK_META_SIZE - size;
    block->size = size;
    new->status = STATUS_FREE;
    // connect list nodes
    new->next = block->next;
    block->next = new;
    if (last == block) last = new;
    // return pointer to block
    return block;
}

// Obtain a memory block of at least 'size' bytes, allocating memory if needed
static block_t *alloc_block(size_t size) {
    block_t *block = find_free_block(size);
    if (block == NULL) {
        // allocate more memory
        block = increase_heap();
    }
    // split block to only use 'size' bytes
    return split_block(block, size);
}

// Coalesce a block with the surrounding block(s), if possible
static void coalesce(block_t *block) {
    // First, coalesce with all free blocks to its RIGHT
    block_t *next = block->next;
    while (next != NULL && next->status == STATUS_FREE) {
        block->next = next->next;
        block->size += next->size + BLOCK_META_SIZE;
        next = block->next;
    }
    // don't forget to update 'last' if needed
    if (next == NULL) last = block;
    // TODO Coalesce left?
}




// Allocate size bytes on the heap
void *brk_alloc(size_t size) {
    // Try to find an existing block to use
    block_t *block = find_free_block(size);
    // Otherwise, allocate more memory
    if (block == NULL) block = increase_heap();
    // Return slice of the block just big enough
    return PAYLOAD(split_block(block, size));
}

// Reallocate bytes on the heap
void *brk_realloc(void *ptr, size_t size) {
    block_t *block = BLOCK(ptr);
    if (block->status == STATUS_FREE) return NULL;
    else if (block->status != STATUS_ALLOC) return ERROR;
    // If we are reducing size, just split existing block
    if (size < block->size) return PAYLOAD(split_block(block, size));

    // Try to coalesce block with next block(s) first
    coalesce(block);
    if (block->size >= size) return PAYLOAD(split_block(block, size));

    // If that fails, obtaing new block
    block_t *new = alloc_block(size);
    // Copy all data to new block
    memcpy(PAYLOAD(new), PAYLOAD(block), size);
    // Mark old block as free (no need to coalesce, already tried)
    block->status = STATUS_FREE;

    return PAYLOAD(new);
}

// Free memory that was allocated with brk_alloc. Returns 1 for success, 0 for no action taken (pointer to already freed memory)
// and -1 for eror (invalid pointer)
int brk_free(void *ptr) {
    block_t *block = BLOCK(ptr);
    if (block->status == STATUS_FREE) return 0; // memory already freed
    else if (block->status == STATUS_ALLOC) {
        // Mark block as free and coalesce it with nearby blocks
        block->status = STATUS_FREE;
        coalesce(block);
        return 1; // success
    } else return -1; // invalid pointer
}