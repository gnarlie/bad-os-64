#include "common.h"

enum AtaErrors {
    OK = 0,
    ERR_BUSY = 1
};


int readSector(uint64_t lba, void * buf, size_t sz);
void init_ata();
