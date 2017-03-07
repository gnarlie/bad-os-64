#pragma once

#include "common.h"

typedef struct storage_device_t {
    int (*read_sector)(struct storage_device_t *, uint64_t lba, void * buf, size_t sz);
} storage_device;

typedef struct file_system_t {
    int (*exists)(struct file_system_t *, const char * filename);
    int (*slurp)(struct file_system_t *, const char * filename, char * buf, size_t sz);
} file_system;


void register_fs(file_system * dev);

int read(const char * filename, char * buf, size_t sz);

