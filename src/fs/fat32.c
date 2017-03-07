#include "fs/fat32.h"

#include "console.h"
#include "memory.h"
#include "errno.h"

typedef struct bios_parameter_block {
    char   skip[3];             // jump instruction or similar
    char   oem[8];              // meaningless
    uint16_t bytesPerSector;
    uint8_t  sectorsPerCluster;
    uint16_t reservedSectors;   // includes boot sector
    uint8_t  noFats;
    uint16_t noDirEntries;
    uint16_t totalSectors;
    uint8_t  mediaDescType;
    uint16_t sectorsPerFat16;
    uint16_t sectorsPerTrack; // dubious
    uint16_t heads;
    uint32_t noHiddenSectors;
    uint32_t largeAmtSectors;
} __attribute__((packed)) bios_parameter_block;

typedef struct fat32_boot_sector {
    uint32_t tableSize; // sectors per FAT
    uint16_t extFlags;
    uint16_t version;
    uint32_t rootCluster;
    uint16_t fatInfo;
    uint16_t backupBS;
    char     reserved[12];
    char     driveNo;
    char     reserved2;
    char     sig;
    uint32_t volumeId;
    char     label[11];
    char     fatTypeLabel[8];
    char     bootCode[420];
    uint16_t bootSig;
} __attribute__((packed)) fat32_boot_sector;

typedef struct directory_entry {
    char name[8];
    char ext[3];
    uint8_t attributes;
    uint8_t userattributes;

    char undelete;
    uint16_t create_t;
    uint16_t create_d;
    uint16_t access_d;

    uint16_t clusterHigh;

    uint16_t modify_t;
    uint16_t modify_d;
    uint16_t clusterLow;

    uint32_t fileSize;
} __attribute((packed)) directory_entry;

typedef struct {
    file_system fs;
    bios_parameter_block  bpb;
    fat32_boot_sector  bs;
    storage_device * store;
} __attribute__((packed)) fat_device;

inline uint32_t lbaOfCluster(fat_device* self, uint32_t cluster) {
    uint32_t fatSectors = self->bs.tableSize * self->bpb.noFats;
    uint32_t firstDataSector = self->bpb.reservedSectors + fatSectors;
    return firstDataSector + (cluster - 2) * self->bpb.sectorsPerCluster;
}

static void filenameFromDirectoryEntry(const directory_entry* de, char * name, size_t sz) {
    bzero(name, 13);
    strncpy(name, de->name, 8);
    char *dest = strnchr(name, ' ', 8);
    if (!dest) dest = name + 8;
    (*dest++) = '.';
    strncpy(dest, de->ext, 3);
    char * end = name + sizeof(name) - 1;
    while( (*end == ' ' || *end == 0)  && end >= name) {
        *end-- = 0;
    }
}

static int find(file_system * fs, const char * filename, directory_entry * buf) {
    fat_device * dev = (fat_device*) fs;
    storage_device * store = dev->store;

    uint32_t lba = lbaOfCluster(dev, dev->bs.rootCluster);

    directory_entry root[16];
    if (store->read_sector(store, lba, &root, sizeof(root))) {
        return EINVALID;
    }

    for (uint16_t i = 0; root[i].name[0] && i < 16; ++i) {
        if ((unsigned char)root[i].name[0] == 0xe5) continue; // unused
        if (root[i].attributes == 0xf) continue; // lfn

        char name[13];
        filenameFromDirectoryEntry(root + i, name, sizeof(name));
        if (0 == strncmp(name, filename, sizeof(name))) {
            if (buf) *buf = root[i];
            return EOK;
        }
    }

    return ENOTFOUND;
}

static int slurp(file_system *fs, const char * filename, char * buf, size_t size) {

    directory_entry e;
    int r = find(fs, filename, &e);
    if (r == EOK) {
        if (e.attributes != 0x20) {
            return EINVALID;
        }

        fat_device * dev = (fat_device*) fs;

        uint32_t cluster = (e.clusterHigh << 16) + e.clusterLow;
        uint32_t lba = lbaOfCluster(dev, cluster);

        if (size > e.fileSize) size = e.fileSize;

        if (EOK == (r = dev->store->read_sector(dev->store, lba, buf, size))) {
            return size;
        }
    }

    return r;
}

static int exists(file_system* fs, const char * filename) {
    return find(fs, filename, NULL);
}


void init_fat32(storage_device * dev) {
    fat_device * self = kmem_alloc(sizeof(fat_device));
    bzero(self, sizeof(fat_device));
    self->store = dev;

    if (EOK != dev->read_sector(dev, 0, &self->bpb, sizeof(bios_parameter_block) +
            sizeof(fat32_boot_sector))) {
        console_print_string("Cannot read \n");
        goto not_fat;
    }

    if (0 != memcmp(self->bs.fatTypeLabel, "FAT32", 5)) {
        goto not_fat;
    }

    char label[9];
    strncpy(label, self->bs.label, 8);
    label[8] = 0;

    console_print_string("Hello from FAT. Label: %s Sector size %d, Cluster size: %d\n",
            label, self->bpb.bytesPerSector, self->bpb.sectorsPerCluster);

    uint32_t lba = lbaOfCluster(self, self->bs.rootCluster);
    directory_entry root[16];
    if (dev->read_sector(dev, lba, &root, sizeof(root))) {
        console_print_string("Could not read root dir\n");
        goto not_fat;
    }

    for (uint16_t i = 0; root[i].name[0] && i < 16; ++i) {
        if ((unsigned char)root[i].name[0] == 0xe5) continue; // unused
        if (root[i].attributes == 0xf) continue; // lfn

        char name[13];
        filenameFromDirectoryEntry(root + i, name, sizeof(name));

        console_print_string("Root entry. flags %x ", root[i].attributes);
        console_print_string(" name %s\n", name);
    }

    self->fs.exists = exists;
    self->fs.slurp = slurp;
    register_fs(&self->fs);
    return;

not_fat:
    kmem_free(self);
    return;

}

