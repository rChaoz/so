#include "brk_alloc.h"
#include "mmap_alloc.h"

#define BLOCK(payload) ((block_t*)(((void*)payload) - BLOCK_META_SIZE))

static void *initial = NULL;
static block_t *first = NULL, *last = NULL;

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

// Coalesce a block with the surrounding block(s), if possible
// Only need to check distance 1 in every direction because we call this function on every free;
// it's impossible for 2 consecutive free blocks to exist in the list
static block_t *coalesce(block_t *block) {
    // First, coalesce with a possible free block on its RIGHT
    block_t *right = block->next;
    if (right != NULL && right->status == STATUS_FREE) {
        block->next = right->next;
        block->size += right->size + BLOCK_META_SIZE;
        if (right == last) last = block;
    }
    if (block == first) return block;

    // Next, coalesce with a possible free block on its LEFT
    block_t *left = first;
    while (left->next != block) left = left->next;
    if (left->status != STATUS_FREE) return block;
    left->next = block->next;
    left->size += block->size + BLOCK_META_SIZE;
    left->status = block->status; // keep status

    if (last == block) last = left;
    return left;
}

// Allocate size bytes on the heap
void *brk_alloc(size_t size) {
    // Check if an existing block is big enough
    block_t *block = find_free_block(size);
    if (block != NULL) return PAYLOAD(split_block(block, size));
    // Increase heap size

    // I1f this is the first allocation, allocate 128kb and align memory block
    if (initial == NULL) {
        initial = sbrk(MMAP_THRESHOLD);
        DIE(initial == ERROR, "sbrk failed");
        long align = ((long) initial) % 8;
        if (align) align = 8 - align;
        first = last = initial + align;
        first->size = MMAP_THRESHOLD - BLOCK_META_SIZE - align;
        first->next = NULL;
        first->status = STATUS_FREE;
        // Return only how much is needed from this large memory block
        return PAYLOAD(split_block(first, size));
    }
    // Otherwise, previous block is aligned and has size multiple of 8, so new block is always aligned

    // If last block is free, just increase its size to be enough
    if (last->status == STATUS_FREE) {
        last->status = STATUS_ALLOC;
        // NOLINT(cppcoreguidelines-narrowing-conversions)
        DIE(sbrk(size - last->size) == ERROR, "sbrk failed");
        last->size = size;
        return PAYLOAD(last);
    }
    // Else allocate create new block
    block = sbrk(BLOCK_META_SIZE + size);
    DIE(block == ERROR, "sbrk failed");
    block->size = size;
    block->status = STATUS_ALLOC;
    // add it to end of list
    last->next = block;
    last = block;
    block->next = NULL;

    return PAYLOAD(block);
}

// Reallocate bytes on the heap
void *brk_realloc(void *ptr, size_t size) {
    block_t *block = BLOCK(ptr);
    if (block->status == STATUS_FREE) return NULL;
    DIE(block->status != STATUS_ALLOC, "invalid pointer");
    // If we are reducing size, just split existing block
    if (size < block->size) return PAYLOAD(split_block(block, size));

    // Special case: brk realloc called with a new size greater than MMAP_THRESHOLD
    if (size >= MMAP_THRESHOLD) {
        // Allocate block using mmap
        void *new = mmap_alloc(size);
        // Copy old block to new mmap block
        memcpy(new, PAYLOAD(block), block->size);
        // Free old block
        block->status = STATUS_FREE;
        coalesce(block);
        return new;
    }

    // Try to coalesce block with next block(s) first
    block_t *coalesced = coalesce(block);
    if (block->size >= size) {
        // If it was coalesced with a block on its left, data needs to be moved
        if (coalesced != block) memmove(PAYLOAD(coalesced), PAYLOAD(block), block->size);
        return PAYLOAD(split_block(coalesced, size));
    }

    // If that fails, obtaing new block
    void *new = brk_alloc(size);
    // Copy all data to new block
    memcpy(new, PAYLOAD(block), size);
    // Mark old block as free (no need to coalesce, already tried)
    block->status = STATUS_FREE;

    return new;
}

// Free memory that was allocated with brk_alloc. Returns 1 for success, 0 for no action taken (pointer to already freed memory)
int brk_free(void *ptr) {
    block_t *block = BLOCK(ptr);
    if (block->status == STATUS_FREE) return 0; // memory already freed
    else if (block->status == STATUS_ALLOC) {
        // Mark block as free and coalesce it with nearby blocks
        block->status = STATUS_FREE;
        coalesce(block);
        return 1; // success
    } else
        DIE(1, "invalid pointer");
}