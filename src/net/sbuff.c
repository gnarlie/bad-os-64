#include "net/sbuff.h"

#include "memory.h"

sbuff * sbuff_free(void *p) {
    kmem_free(p);
}

sbuff* raw_sbuff_alloc(uint16_t size) {
    sbuff * ret = kmem_alloc(size + sizeof(sbuff));

    ret->totalSize = ret->currSize = size;
    ret->head = ret->data;
    ret->refs = 0;
}
