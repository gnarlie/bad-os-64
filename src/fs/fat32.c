#include "common.h"
#include "console.h"

#include "ata.h"

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

void init_fat32() {

    struct {
        bios_parameter_block  bpb;
        fat32_boot_sector  bs;
    } __attribute__((packed)) data;

    bzero(&data, sizeof(data));

    console_print_string("Addr %p\n", &data);

    readSector(0, &data, sizeof(data));

    char label[9];
    strncpy(label, data.bs.label, 8);
    label[8] = 0;

    console_print_string("Hello from FAT. Label: %s\n", label);
};

