#include "common.h"
#include "memory.h"

typedef struct BlockT {
    struct BlockT * next;
    uint64_t size;
    uint32_t nChunks;
    uint32_t chunkSize;
    uint32_t chunkMasks[];
} Block;

typedef struct AllocationHeaderT {
    uint32_t chunks;
    uint32_t sequence;
} AllocationHeader;

struct HeapT {
    uint32_t sequence;
    uint32_t currentObjects;
    Block * block;
} Heap;

struct HeapT heap;

uint32_t kmem_current_objects() {
    return heap.currentObjects;
}

void kmem_add_block(uint64_t start, uint64_t size, size_t chunkSize) {
    Block * newb = (Block*) start;
    newb->next = heap.block;
    heap.block = newb;

    newb->size = size - sizeof(Block);
    newb->chunkSize = chunkSize;

    newb->nChunks = newb->size / newb->chunkSize;
    if (newb->nChunks * newb->chunkSize < newb->size) newb->nChunks++;

    size_t maskSize = sizeof(newb->chunkMasks[0]);
    newb->nChunks = (newb->nChunks / maskSize) * maskSize;

    bzero(newb+1, newb->nChunks);
}

void kmem_init() {
    heap.block = 0;
    heap.sequence = 0;
    heap.currentObjects = 0;
}

static inline int test_bits(uint8_t p, int mask) {
    return (mask & p) == mask;
}

static inline void set_bit(uint8_t *p, int mask) {
    *p |= mask;
}

static char * kmem_block_start(Block* block) {
    return (char*)(block + 1) + block->nChunks / sizeof(block->chunkMasks[0]);
}

void* kmem_alloc(size_t size) {
    size += sizeof(AllocationHeader);
    for (Block * block = heap.block; block; block = block->next) {
        uint32_t requiredChunks = size / block->chunkSize;
        if (block->chunkSize * requiredChunks < size) requiredChunks ++;

        // FIXME .. this will never cross bitset boundry - so no allocation with
        // more than 32 chunks will *ever* work. We'll need test running end of
        // one chunk with the begging of the next
        uint32_t mask = (1 << requiredChunks) - 1;

        for(uint32_t chunk = 0; chunk < block->nChunks; chunk++) {
            int nBits = sizeof(block->chunkMasks[0]) * 8 - requiredChunks + 1;
            for (int bit = 0; bit < nBits; bit++, mask <<= 1) {
                if (test_bits(~block->chunkMasks[chunk], mask)) {
                    block->chunkMasks[chunk] |= mask;

                    size_t offset = block->chunkSize * (chunk * 8 + bit);
                    char *memory = kmem_block_start(block) + offset;
                    ((AllocationHeader*)memory)->chunks = requiredChunks;
                    ((AllocationHeader*)memory)->sequence = heap.sequence++;
                    heap.currentObjects ++;

                    return memory + sizeof(AllocationHeader);
                }
            }
        }
    }

    panic("out of memory");
    return NULL; //todo teach gcc that panic is one-way
}

// void printf(const char *, ...);

static void kmem_free_from_block(Block* block, AllocationHeader * header) {
    size_t offset = (char*)header - kmem_block_start(block);
    uint32_t chunkSet = (offset / block->chunkSize) / sizeof(block->chunkMasks[0]);
    char* firstForChunkSet = kmem_block_start(block) + sizeof(block->chunkMasks[0]) * block->nChunks;
    uint32_t shift = ((char*)header - firstForChunkSet) / block->chunkSize;

    uint32_t mask = ((1 << header->chunks) - 1) << shift;
    block->chunkMasks[chunkSet] &= ~mask;

    heap.currentObjects--;

//    printf("chunks %d, sequence %d\n", header->chunks, header->sequence);
//    printf("offset %x, chunk %x, mask %x, firstForChunk %p, header %p\n", offset, chunkSet, mask, firstForChunkSet, header);
}

void kmem_free(void *ptr) {
    AllocationHeader *hdr = ((AllocationHeader*)ptr) - 1;
    for (Block * block = heap.block; block; block = block->next) {
        void * end = kmem_block_start(block) + block->nChunks * block->chunkSize;
        if (ptr >= (void*) kmem_block_start(block) && ptr < end) {
            kmem_free_from_block(block, hdr);
            return;
        }
    }

    panic("could not find block for memory");
}
