#pragma once

#include "common.h"

static inline uint16_t ntos(uint16_t x)  {
    return x << 8 | x >> 8;
}

static inline uint32_t ntol(uint32_t x) {
    return ((int)ntos(x)) << 16 | ntos(x >> 16);
}
