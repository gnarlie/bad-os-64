#include "common.h"

void kmem_init();
void kmem_add_block(uint64_t start, size_t size, size_t chunkSize);
void *kmem_alloc(size_t size);
void kmem_free(void*);

uint32_t kmem_current_objects();
