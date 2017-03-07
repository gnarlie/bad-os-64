#include "fs/vfs.h"
#include "util/list.h"
#include "errno.h"

static list_t file_systems;

void register_fs(file_system * fs) {
    list_append(&file_systems, fs);
}

int read(const char * filename, char * buf, size_t sz) {
    list_node * node;
    for (node = file_systems.head; node; node = node->next) {
        file_system * fs = node->payload;
        if (fs->exists(fs, filename) == EOK) {
            return fs->slurp(fs, filename, buf, sz);
        }
    }

    return ENOTFOUND;
}
