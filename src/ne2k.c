#include "ne2k.h"
#include "interrupt.h"
#include "pci.h"

#include "console.h"
#include "memory.h"
#include "task.h"

#include "net/ethernet.h"
#include "net/arp.h"
#include "net/ntox.h"
#include "net/device.h"

// Page0
#define PSTART (self->iomem + 1)
#define PSTOP (self->iomem + 2)
#define BOUNDRY (self->iomem + 3)
#define TSTART (self->iomem + 4)
#define TXCNTLO (self->iomem + 5)
#define TXCNTHI (self->iomem + 6)
#define ISR (self->iomem + 7)
#define REMSTARTADDRLO (self->iomem + 0x8)
#define REMSTARTADDRHI (self->iomem + 0x9)
#define REMBCOUNTLO (self->iomem + 0xa)
#define REMBCOUNTHI (self->iomem + 0xb)
#define RCR (self->iomem + 0xc)
#define TCR (self->iomem + 0xd)
#define DCR (self->iomem + 0xe)
#define IMR (self->iomem + 0xf)

#define DATA (self->iomem + 0x10)

// Page 1
#define CURPAGE (self->iomem + 7)

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

static int tx_start_page = 0x40; // start of NE2000 buffer
static int rx_start_page = 0x4c;
static int stop_page = 0x80;     // end of NE2000 buffer

struct recv_data {
    struct netdevice * self;
    uint8_t buffer[];
};

static void dispatch(void* user) {
    struct recv_data *data = (struct recv_data*) user;
    ethernet_packet(data->self, data->buffer);
    kmem_free(data);
}

// TODO: * larger reads, DMA
static void ne2k_irq(registers_t* regs, void * ptr) {
    struct netdevice* self = (struct netdevice*) ptr;
    uint8_t wtf = inb(ISR);

    if (wtf & RemoteDmaComplete) {
        outb(ISR, RemoteDmaComplete);
    }

    if (wtf & PacketReceived) {
        outb(self->iomem, NoDma | Page1);
        uint8_t rxpage = inb(CURPAGE);
        outb(self->iomem, NoDma | Start);
        uint8_t frame = inb(BOUNDRY) + 1;
        if (frame == stop_page)
            frame = rx_start_page;

        while (rxpage != frame) {

            // Read the four byte header
            outb(self->iomem, NoDma | Start);
            outb(REMBCOUNTLO, 4);
            outb(REMBCOUNTHI, 0);
            outb(REMSTARTADDRLO, 0);
            outb(REMSTARTADDRHI, frame);
            outb(self->iomem, RemoteRead | Start);

            union {
                struct {
                    uint8_t status;
                    uint8_t next;
                    uint16_t count;
                } header;
                uint32_t val;
            } hdr;

            hdr.val = inl(self->iomem + 0x10);
            uint16_t size = hdr.header.count - sizeof(hdr);

            outb(self->iomem, NoDma | Start);
            outb(REMSTARTADDRLO, 4);
            outb(REMBCOUNTLO, size & 0xff);
            outb(REMBCOUNTHI, hdr.header.count >> 8);

            struct recv_data * mem = kmem_alloc(size + sizeof(struct recv_data));
            mem->self = self;
            uint8_t * buffer = mem->buffer;

            // 16 bits at a time, up to a page (TODO handle larger reads)
            for (uint16_t s = 0; s < size; s+=2) {
               uint16_t d = inw(DATA);
               buffer[s+1] = d >> 8;
               buffer[s] = d & 0xff;
            }
            if (size & 1) {
               uint8_t d = inb(DATA);
               buffer[size - 1] = d;
            }

            task_enqueue_easy(dispatch, mem);

            frame = hdr.header.next;
            outb(BOUNDRY, frame - 1);
        }

        outb(ISR, PacketReceived);
    }

    outb(self->iomem, Start|NoDma|Page0);
    outb(ISR, 0xff); // clear ISR
}

struct PendingSend {
    struct TaskT *task;
    struct sbuff_t * data;
    struct netdevice * dev;
};

static void send_sync(void * user) {
    struct PendingSend *send = (struct PendingSend*)user;
    struct netdevice *self = send->dev;
    uint16_t nextSize = send->data->currSize;
    uint8_t* next = send->data->data;

    // stage
    outb(self->iomem, NoDma|Page0);
    outb(IMR, 0); // disable interrupts
    outb(REMBCOUNTLO, nextSize & 0xff);
    outb(REMBCOUNTHI, nextSize >> 8);
    outb(REMSTARTADDRLO, 0);
    outb(REMSTARTADDRHI, tx_start_page);

    //console_print_string("\nSend: ");
    for(int i = 0; i < nextSize; i+=2) {
        uint16_t lo = next[i];
        uint16_t hi = next[i+1];
        uint16_t data = lo | hi << 8;
        //console_put_hex16(ntos(data));
        //console_print_string(" ");
        outw(DATA, data);
    }
    if (nextSize % 2) outb(DATA, next[nextSize - 1]);

    outb(ISR, 0x40);

    // trigger
    outb(self->iomem, NoDma|Page0);
    outb(TXCNTLO, nextSize & 0xff);
    outb(TXCNTHI, nextSize >> 8);
    outb(TSTART, tx_start_page);
    outb(IMR, ImrAllIsr);
    outb(self->iomem, NoDma|Transmit|Start);
}

static void ne2k_send(struct netdevice * dev, sbuff * sbuff) {
    struct PendingSend *send = kmem_alloc(sizeof(struct PendingSend));
    send->task = task_alloc(send_sync, send);
    send->data = sbuff;
    add_ref(send->data);
    send->dev = dev;

    task_enqueue(send->task);
}

static uint32_t myIp = 0xC0A80302;
static uint32_t gateway = 0xC0A80301;

static void initialize(uint8_t intr, uint32_t bar0) {
    struct netdevice * self = kmem_alloc(sizeof(struct netdevice));
    self = kmem_alloc(sizeof(struct netdevice));
    self->send = ne2k_send;
    self->ip = myIp;

    self->iomem = bar0 & ~3;
    console_print_string("Found ne2k on IRQ %d with MAC ", intr);

    outb(self->iomem, NoDma|Stop);
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
    outb(self->iomem, Start | RemoteRead);
    for (int i = 0; i < 6; i++) {
        mac[i] = inb(self->iomem + 0x10);
        if (i) console_print_string(":");
        console_put_hex8(mac[i]);
        self->mac[i] = mac[i];
    }
    console_print_string("\n");

    // physical address accepted
    outb(self->iomem, NoDma|Page1|Stop); // page 1
    for (int i = 0; i < 6; i++) {
        outb(self->iomem + i + 1,  mac[i]);
    }
    outb(CURPAGE, rx_start_page); //Current page

    // accept all multicast
    for (int i = 0; i < 8; i++) {
        outb(self->iomem+8+i, 0xff);
    }

    register_interrupt_handler(intr + 32, ne2k_irq, self);

    // Game On!
    outb(self->iomem, NoDma|Start|Page0);
    outb(REMBCOUNTLO, 0);
    outb(ISR, 0xff);        // clear ISR
    outb(IMR, ImrAllIsr);   // IMR enable everything
    outb(RCR, 0x1c);  // RCR: everything
    outb(TCR, 0x00);  // TCR - normal

    gratuitous_arp(self);
    arp_lookup(self, gateway, NULL);
}

void init_ne2k() {
    register_pci_device(0x10ec, 0x8029, initialize);
}
