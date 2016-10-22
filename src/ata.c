#include "ata.h"
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
    uint8_t bus;
    uint16_t base;
    uint16_t ctrl;
    MasterOrSlave ms;
    const char * type;
    uint8_t lba48;
    uint32_t lba28Sectors;
    uint64_t lba48Sectors;
} AtaDevice;

AtaDevice devices[] = {
    {.bus = 0, .base = 0x1f0, .ctrl=0x3f6, .ms = Master},
    {.bus = 0, .base = 0x1f0, .ctrl=0x3f6, .ms = Slave},
    {.bus = 1, .base = 0x170, .ctrl=0x3f6, .ms = Master},
    {.bus = 1, .base = 0x170, .ctrl=0x3f6, .ms = Slave},
    {.bus = 2, .base = 0x1e8, .ctrl=0x3f6, .ms = Master},
    {.bus = 2, .base = 0x1e8, .ctrl=0x3f6, .ms = Slave},
    {.bus = 3, .base = 0x168, .ctrl=0x3f6, .ms = Master},
    {.bus = 3, .base = 0x168, .ctrl=0x3f6, .ms = Slave},
};

// Use ATA PIO. Consider switching to DMA at some point
int readSector(uint64_t lba, void * buf, size_t sz) {
    // hardcoded device for now
    AtaDevice * dev = &devices[1];

    uint8_t status = inb(dev->base + CommandStatus);
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

    uint16_t * dst = buf;
    size_t x = 0;
    while(x < sz / 2) {
        uint16_t data = inw(dev->base + Data);
        dst[x++] = data;
    }
}

static int identify(AtaDevice * dev) {
    uint8_t status = inb(dev->base + CommandStatus);
    if (status == 0xff) return 0;

    // select primary master or slave
    outb(dev->base + DriveHead, dev->ms ? 0xb0 : 0xa0);

    outb(dev->base + SectorLBAlo, 0);
    outb(dev->base + CylinderLowLBAmid, 0);
    outb(dev->base + CylinderHighLBAhi, 0);

    // identify
    outb(dev->base + CommandStatus, 0xec);

    status = inb(dev->base + CommandStatus);
    uint8_t sigLow = inb(dev->base + CylinderLowLBAmid);
    uint8_t sigHigh = inb(dev->base + CylinderHighLBAhi);
    if (status) {
        if (sigLow == 0 && sigHigh == 0) dev->type = "ATA";
        if (sigLow == 0x14 && sigHigh == 0xeb) dev->type = "ATAPI";
        if (sigLow == 0x69 && sigHigh == 0x96) dev->type = "SATAPI";
        if (sigLow == 0x3c && sigHigh == 0xc3) dev->type = "SATA";

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

        dev->lba48 = (uint32_t)ident[83] & 0x400;
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
        AtaDevice * dev = devices + i;
        if (identify(dev)) {
            console_print_string("Discovered %s device %d:%d. %d lba48 sectors\n",
                    dev->type, dev->bus, dev->ms, dev->lba48Sectors);
        }
    }
}
