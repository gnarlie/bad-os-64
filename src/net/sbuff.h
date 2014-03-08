#pragma once

#include "common.h"

typedef struct sbuff_t {
    uint16_t totalSize;
    uint16_t currSize;
    uint8_t* head;
    uint8_t refs;
    uint8_t data[];
} sbuff;

sbuff * raw_sbuff_alloc(uint16_t payload);
sbuff * sbuff_free(void *);

static inline void sbuff_push(sbuff * s, uint16_t size) {
    if (size > s->currSize) {
        panic("net: sbuff overflow");
    }
    s->currSize -= size;
    s->head += size;
}

static inline void sbuff_pop(sbuff * s, uint16_t size) {
    if (size + s->currSize > s->totalSize) {
        panic("net: sbuff underflow");
    }
    s->currSize += size;
    s->head -= size;
}
