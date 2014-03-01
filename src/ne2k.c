#include "ne2k.h"
#include "interrupt.h"
#include "pci.h"

#include "console.h"
#include "memory.h"

uint32_t iomem;

// Page0
#define PSTART (iomem + 1)
#define PSTOP (iomem + 2)
#define BOUNDRY (iomem + 3)
#define TSTART (iomem + 4)
#define ISR (iomem + 7)
#define REMSTARTADDRLO (iomem + 0x8)
#define REMSTARTADDRHI (iomem + 0x9)
#define REMBCOUNTLO (iomem + 0xa)
#define REMBCOUNTHI (iomem + 0xb)
#define RCR (iomem + 0xc)
#define TCR (iomem + 0xd)
#define DCR (iomem + 0xe)
#define IMR (iomem + 0xf)

// Page 1
#define CURPAGE (iomem + 7)

enum State {
    Initializing,
    Ready
} state;

enum Ne2k_ISRBits {

    PacketReceived = 1,
    PacketTransmitted = 2,
    ReceiveError = 4,
    TxError = 8,
    OverwriteWarn = 16,
    CounterOverflow = 32,
    RemoteDmaComplete = 64,
    ResetStatus = 128,

    ImrAllIsr = 0x3f // Reset status cannot IRQ
};

enum Ne2K_CmdBits{
    Stop = 1,
    Start = 2,
    Transmit = 4,
    RemoteRead = 8,
    RemoteWrite = 0x10,
    SendPacket = 0x18,
    NoDma = 0x20,
    Page0 = 0,
    Page1 = 0x40,
    Page2 = 0x80,
    Page3 = 0xc0
};

static uint8_t buffer[1600];

static int tx_start_page = 0x40; // start of NE2000 buffer
static int rx_start_page = 0x4c;
static int stop_page = 0x80;     // end of NE2000 buffer

static void ne2k_irq(registers_t* regs) {
    uint8_t wtf = inb(iomem+0x7);

    if (wtf & RemoteDmaComplete) {
        outb(iomem+0x7, RemoteDmaComplete);
    }

    if (wtf & PacketReceived) {
        outb(iomem, NoDma | Page1);
        uint8_t rxpage = inb(CURPAGE);
        outb(iomem, NoDma | Start);
        uint8_t frame = inb(BOUNDRY) + 1;
        if (frame == stop_page)
            frame = rx_start_page;

        console_put_hex8(rxpage);
        console_put_hex8(frame);
        console_print_string(" ");

        while (rxpage != frame) {

            // Read the four byte header
            outb(iomem, NoDma | Start);
            outb(REMBCOUNTLO, 4);
            outb(REMBCOUNTHI, 0);
            outb(REMSTARTADDRLO, 0);
            outb(REMSTARTADDRHI, frame);
            outb(iomem, RemoteRead | Start);

            union {
                struct {
                    uint8_t status;
                    uint8_t next;
                    uint16_t count;
                } header;
                uint32_t val;
            } hdr;

            hdr.val = inl(iomem + 0x10);
            uint16_t size = hdr.header.count - sizeof(hdr);

            console_print_string("status ");
            console_put_hex8(hdr.header.status);
            console_print_string(": ");

            outb(iomem, NoDma | Start);
            outb(REMSTARTADDRLO, 4);
            outb(REMBCOUNTLO, size & 0xff);
            outb(REMBCOUNTHI, hdr.header.count >> 8);

            // 16 bits at a time, up to a page (TODO handle larger reads)
            for (uint16_t s = 0; s < size; s+=2) {
               uint16_t d = inw(iomem + 0x10);
               buffer[s] = d >> 8;
               buffer[s+1] = d & 0xff; //TODO, handle odd reads
               console_put_hex8(buffer[s]);
               console_put_hex8(buffer[s+1]);
            }
            if (size & 1) {
               uint8_t d = inb(iomem + 0x10);
               buffer[size - 1] = d;
               console_put_hex8(d);
            }

            frame = hdr.header.next;
            outb(BOUNDRY, frame - 1);
        }

        console_print_string("\n");

        outb(iomem+0x7, PacketReceived);
    }

    outb(iomem, Start|NoDma|Page0);
    outb(ISR, 0xff); // clear ISR
}

static void initialize(uint8_t intr, uint32_t bar0) {
    state = Initializing;

    iomem = bar0 & ~3;
    console_print_string("Found ne2k on IRQ ");
    console_put_dec(intr);
    console_print_string("\n");

    outb(iomem, NoDma|Stop);
    outb(DCR, 0x49);  // DCR -> BYTEXFR|NOLOOP|WORD
    outb(REMBCOUNTLO, 0x0);   // RBCR0 -> 0 size
    outb(REMBCOUNTHI, 0x0);   // RBCR1 -> 0 size
    outb(RCR, 0x20);  // RCR - Monitor
    outb(TCR, 0x02);  // TCR - Internal Loopback

    outb(PSTART, rx_start_page);
    outb(PSTOP, stop_page);
    outb(BOUNDRY, stop_page - 1);
    outb(TSTART, tx_start_page);

    outb(ISR, 0xff); // clear ISR
    outb(IMR, 0); // IMR nothing, yet

    // Read the MAC from PROM
    uint8_t mac[6];
    outb(REMBCOUNTLO, 6);
    outb(REMBCOUNTHI, 0);
    outb(REMSTARTADDRLO, 0);
    outb(REMSTARTADDRHI, 0);
    outb(iomem, Start | RemoteRead);
    for (int i = 0; i < 6; i++) {
        mac[i] = inb(iomem + 0x10);
        if (i) console_print_string(":");
        console_put_hex8(mac[i]);
    }
    console_print_string("\n");

    // physical address accepted
    outb(iomem, NoDma|Page1|Stop); // page 1
    for (int i = 0; i < 6; i++) {
        outb(iomem + i + 1,  mac[i]);
    }
    outb(CURPAGE, rx_start_page); //Current page

    // accept all multicast
    for (int i = 0; i < 8; i++) {
        outb(iomem+8+i, 0xff);
    }

    register_interrupt_handler(intr + 32, ne2k_irq);

    // Game On!
    outb(iomem, NoDma|Start|Page0);
    outb(REMBCOUNTLO, 0);
    outb(ISR, 0xff);        // clear ISR
    outb(IMR, ImrAllIsr);   // IMR enable everything
    outb(RCR, 0x1c);  // RCR: everything
    outb(TCR, 0x00);  // TCR - normal
}

void init_ne2k() {
    register_pci_device(0x10ec, 0x8029, initialize);
}
