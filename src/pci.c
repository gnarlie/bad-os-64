#include "pci.h"

#include "common.h"
#include "console.h"

static uint32_t readPciConfig(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (bus << 16) | (slot << 11) | (func << 8) | (1 << 31) | (offset & 0xfc);

    outl(0xcf8, address);

    return inl(0xcfc);
}

static void config_word(uint8_t slot, uint8_t o) {
    uint32_t word = readPciConfig(0, slot, 0, o);
    console_print_string(" "); console_put_hex(word);
}

static void print_device(uint32_t vendev, uint8_t slot, uint8_t f) {

    console_print_string("%d.%d: ", slot, f);
    console_put_hex16(vendev & 0xffff); console_print_string(":"); console_put_hex16(vendev >> 16);

    uint8_t hdr = readPciConfig(0, slot, f, 0xc) >> 16;
    console_print_string(" " ); console_put_hex8(hdr); console_print_string(" " );

    uint32_t bar0 = readPciConfig(0, slot, f, 0x10);
    console_put_hex(bar0); console_print_string(" " );

    uint32_t bar1 = readPciConfig(0, slot, f, 0x14);
    console_put_hex(bar1); console_print_string(" " );

    uint32_t statCmd = readPciConfig(0, slot, f, 0x4);
    console_print_string("%x ", statCmd);

    uint8_t intr = readPciConfig(0, slot, f, 0x3c) ;
    if (intr) console_print_string(" int %d", intr);

    console_print_string("\n");
}

void init_pci() {
    console_print_string("Probing the PCI bus:\n");
    for(uint16_t slot = 0; slot < 32; slot++) {
        uint32_t vendev = readPciConfig(0, slot, 0, 0);
        if (vendev != 0xffffffff) {
            print_device(vendev, slot, 0);
            uint8_t hdr = readPciConfig(0, slot, 0, 0xc) >> 16;

            if (hdr & 0x80) { // multi-function
                for (int f = 1; f < 8; f++) {
                    vendev = readPciConfig(0, slot, f, 0);
                    if (vendev != 0xffffffff) {
                        print_device(vendev, slot, f);
                    }
                }
            }
        }
    }
}

void register_pci_device(uint16_t vendor, uint16_t device, pci_init_fn fn) {
    for(uint16_t slot = 0; slot < 32; slot++) {
        uint32_t vendev = readPciConfig(0, slot, 0, 0);
        if (vendev == (device << 16 | vendor)) {
            uint8_t intr = readPciConfig(0, slot, 0, 0x3c) ;
            uint32_t bar0 = readPciConfig(0, slot, 0, 0x10);
            fn(intr, bar0);
        }
    }
}
