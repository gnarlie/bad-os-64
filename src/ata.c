#include "ata.h"
#include "fs/vfs.h"
#include "fs/fat32.h"
#include "console.h"

typedef enum AtaStatus {
    ERR = 1,
    DRQ = 8,
    SRV = 16,
    DF = 32,
    RDY = 64,
    BSY = 128,
} AtaStatus;

typedef enum PortOffset {
    Data = 0,
    FeaturesError = 1,
    SectorCount = 2,
    SectorLBAlo = 3,
    CylinderLowLBAmid = 4,
    CylinderHighLBAhi = 5,
    DriveHead = 6,
    CommandStatus = 7
} PortOffset;

typedef enum MasterOrSlave {
    Master = 0,
    Slave = 1
} MasterOrSlave;

typedef struct AtaDevice {
    storage_device vtable;
    uint8_t bus;
    uint16_t base;
    uint16_t ctrl;
    MasterOrSlave ms;
    const char * type;
    uint8_t lba48;
    uint32_t lba28Sectors;
    uint64_t lba48Sectors;
} AtaDevice;

static int read_sector(storage_device * self, uint64_t lba, void * buf, size_t sz);

static AtaDevice possible_devices[] = {
    {.vtable = {read_sector}, .bus = 0, .base = 0x1f0, .ctrl=0x3f6, .ms = Master},
    {.vtable = {read_sector}, .bus = 0, .base = 0x1f0, .ctrl=0x3f6, .ms = Slave},
    {.vtable = {read_sector}, .bus = 1, .base = 0x170, .ctrl=0x376, .ms = Master},
    {.vtable = {read_sector}, .bus = 1, .base = 0x170, .ctrl=0x376, .ms = Slave},
    {.vtable = {read_sector}, .bus = 2, .base = 0x1e8, .ctrl=0x3e6, .ms = Master},
    {.vtable = {read_sector}, .bus = 2, .base = 0x1e8, .ctrl=0x3e6, .ms = Slave},
    {.vtable = {read_sector}, .bus = 3, .base = 0x168, .ctrl=0x366, .ms = Master},
    {.vtable = {read_sector}, .bus = 3, .base = 0x168, .ctrl=0x366, .ms = Slave},
};

static int drive_select(AtaDevice * dev) {
    // select primary master or slave
    outb(dev->base + DriveHead, dev->ms ? 0xb0 : 0xa0);

    inb(dev->ctrl);
    inb(dev->ctrl);
    inb(dev->ctrl);
    inb(dev->ctrl);
    return inb(dev->ctrl);
}

// Use ATA PIO. Consider switching to DMA at some point
int read_sector(storage_device * self, uint64_t lba, void * buf, size_t sz) {
    AtaDevice * dev = (AtaDevice*) self;
    uint8_t status = drive_select(dev);
    if ((status & ERR) || (status & DRQ) || (status & BSY)) {
        // device error
        return ERR_BUSY;
    }

    uint16_t nSectors = sz / 512;
    if (sz % 512) nSectors ++;

    uint8_t * lba2 = (void*)&lba;

    outb(dev->base + DriveHead, 0x40 | (dev->ms << 4));
    outb(dev->base + SectorCount, nSectors >> 8);
    outb(dev->base + SectorLBAlo, lba2[3]);
    outb(dev->base + CylinderLowLBAmid, lba2[4]);
    outb(dev->base + CylinderHighLBAhi, lba2[5]);

    outb(dev->base + SectorCount, nSectors & 0xff);
    outb(dev->base + SectorLBAlo, lba2[0]);
    outb(dev->base + CylinderLowLBAmid, lba2[1]);
    outb(dev->base + CylinderHighLBAhi, lba2[2]);

    outb(dev->base + CommandStatus, 0x24); // 0x24 == read sectors ext

    status = inb(dev->base + CommandStatus);
    while (status & BSY) {
        status = inb(dev->base + CommandStatus);
    }

    uint16_t * dst = buf;
    size_t x = 0;
    while(x < sz / 2) {
        uint16_t data = inw(dev->base + Data);
        dst[x++] = data;
    }

    return 0;
}

static int identify(AtaDevice * dev) {
    uint8_t status = inb(dev->base + CommandStatus);
    if (status == 0xff) return 0; // floating bus -> no device

    // select the correct bus & master/slave
    drive_select(dev);

    outb(dev->base + SectorLBAlo, 0);
    outb(dev->base + CylinderLowLBAmid, 0);
    outb(dev->base + CylinderHighLBAhi, 0);

    // identify
    outb(dev->base + CommandStatus, 0xec);

    uint8_t sigLow = inb(dev->base + CylinderLowLBAmid);
    uint8_t sigHigh = inb(dev->base + CylinderHighLBAhi);

    if (sigLow == 0 && sigHigh == 0) dev->type = "ATA";
    if (sigLow == 0x14 && sigHigh == 0xeb) dev->type = "ATAPI";
    if (sigLow == 0x69 && sigHigh == 0x96) dev->type = "SATAPI";
    if (sigLow == 0x3c && sigHigh == 0xc3) dev->type = "SATA";

    status = inb(dev->base + CommandStatus);
    if (status) {

        // wait for BSY clear
        while(status & BSY) {
            status = inb(dev->base + CommandStatus);
        }

        while(1) {
            if (status & DRQ) {
                break;
            }

            if (status & ERR) {
                return 0;
            }

            status = inb(dev->base + CommandStatus);
        }

        uint16_t ident[256];
        for (int i = 0; i < 256; ++i) {
            ident[i] = inw(dev->base + Data);
        }

        dev->lba48 = (((uint32_t)ident[83]) & 0x400) != 0;
        dev->lba28Sectors =
            ident[60] |
            ((uint32_t)ident[61] << 16);
        dev->lba48Sectors =
            ident[100] |
            ((uint64_t)ident[101] << 16) |
            ((uint64_t)ident[102] << 32) |
            ((uint64_t)ident[103] << 48);

        return 1;
    }

    return 0;
}

void init_ata() {
    for(int i = 0; i < 8; ++i) {
        AtaDevice * dev = possible_devices + i;
        if (identify(dev) && dev->lba48) {
            console_print_string("Discovered %s device %d:%d. %d lba48 sectors\n",
                    dev->type, dev->bus, dev->ms, dev->lba48Sectors);
            init_fat32(&dev->vtable);
        }
    }
}
