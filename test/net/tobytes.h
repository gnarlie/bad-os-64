#pragma once

extern size_t strlen(const char *data);
extern void * malloc(size_t sz);

static inline uint8_t fromhex(char c) {
    if (c >= 'a' && c <= 'f') return 10 + c - 'a';
    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
    return c - '0';
}

static inline uint8_t * tobytes(const char * s) {
    int len = strlen(s) / 2;
    uint8_t * ret = malloc(len);
    for (int i = 0; i < len; i++) {
        ret[i] = (fromhex(s[i*2]) << 4) + fromhex(s[i*2+1]);
    }

    return ret;
}

