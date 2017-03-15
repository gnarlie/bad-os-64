#include "net/tcp.h"
#include "tobytes.h"
#include "net/sbuff.h"
#include "net/arp.h"
#include "net/ntox.h"

#include "../tinytest/tinytest.h"

#include <stdio.h>

typedef union {
    uint8_t bytes[1600];
    tcp_hdr hdr;
} tcp_packet;

static mac remote = {1,2,3,4,5,6};

static uint8_t * g_data = NULL;
static uint16_t g_len = -1;
static tcp_hdr * g_recv = NULL;
static stream * last = NULL;

static void tcp_read(stream* stream, const uint8_t* d, uint32_t sz) {
    last = stream;
}

static tcp_read_fn accept() { return tcp_read; }


static void capture(struct netdevice *dev, sbuff* buff) {
    size_t len = buff->totalSize;
    const uint8_t* ptr = buff->data;
    g_data = malloc(len);
    memcpy(g_data, ptr, len);
    g_len = len;
    g_recv = (tcp_hdr*)(g_data + 20 + 14);
}

static void cleanup() {
    if (g_data) free(g_data);
    g_data = NULL;
    g_recv = NULL;
    g_len = -1;
}

TEST(connection_refused) {
    struct BytesLen syn;
    syn = tobyteslen("92061fa31cf94a9700000000a0026db075c40000020406180402080a02ebc4ad0000000001030307");
    struct netdevice dev = {.ip =  0xC0A80302, .send=capture};

    arp_store(remote, 0xc0a80301);
    tcp_segment(&dev, syn.bytes, syn.len, 0xc0a80301);

    ASSERT_EQUALS(54, g_len);
    ASSERT_EQUALS(0x0008, *(uint16_t*)(g_data+12));
    ASSERT_EQUALS(0x0203a8c0, *(uint32_t*)(g_data+26));
    ASSERT_EQUALS(0x0103a8c0, *(uint32_t*)(g_data+30));
    ASSERT_EQUALS(0x14, g_data[47]); // ack,rst

    cleanup();
}

TEST(connection_synack) {
    struct BytesLen syn;
    syn = tobyteslen("c7780050ee75886200000000a00272109ec30000020405b40402080a04316d060000000001030307");
    struct netdevice dev = {.ip =  0xC0A80302, .send=capture};

    listen(80, accept);

    dev.send = capture;
    arp_store(remote, 0xc0a80301);
    tcp_segment(&dev, syn.bytes, syn.len, 0xc0a80301);

    ASSERT_INT_EQUALS(54, g_len);
    ASSERT_INT_EQUALS(0x0008, *(uint16_t*)(g_data+12));
    ASSERT_INT_EQUALS(0x0203a8c0, *(uint32_t*)(g_data+26));
    ASSERT_INT_EQUALS(0x0103a8c0, *(uint32_t*)(g_data+30));
    ASSERT_INT_EQUALS(0x12, g_data[47]); // ack,syn
    cleanup();
}


TEST(passive_close) {

    tcp_packet syn = {
        .hdr = {
            .srcPort = 1000, .destPort = ntos(80),
            .sequence=1, .ack=0,
            .offset = 4, .flags = 2 } };

    tcp_packet ack = syn; ack.hdr.flags = 0x10;
    tcp_packet fin = syn; fin.hdr.flags = 1;

    struct netdevice dev = {.ip =  0xC0A80302, .send=capture};

    listen(80, accept);

    arp_store(remote, 0xc0a80301);
    tcp_segment(&dev, syn.bytes, sizeof(tcp_hdr), 0xc0a80301);
    ASSERT_INT_EQUALS(0x12, g_recv->flags); // ack,syn
    ASSERT_INT_EQUALS(ntos(80), g_recv->srcPort);

    cleanup();
    tcp_segment(&dev, (const uint8_t*) &ack, sizeof(tcp_hdr), 0xc0a80301);
    ASSERT_EQUALS(g_recv, NULL);

    cleanup();
    tcp_segment(&dev, (const uint8_t*)&fin, sizeof(fin), 0xc0a80301);
    ASSERT_INT_EQUALS(0x11, g_recv->flags); // ack,fin

    cleanup();
}

TEST(active_close) {

    tcp_packet syn = {
        .hdr = {
            .srcPort = 1000, .destPort = ntos(80),
            .sequence=1, .ack=0,
            .offset = 4, .flags = 2 } };

    tcp_packet ack = syn;
    ack.hdr.flags = 0x18;
    ack.hdr.sequence++;

    struct netdevice dev = {.ip =  0xC0A80302, .send=capture};

    listen(80, accept);

    arp_store(remote, 0xc0a80301);
    tcp_segment(&dev, syn.bytes, sizeof(tcp_hdr), 0xc0a80301);
    ASSERT_INT_EQUALS(0x12, g_recv->flags); // ack,syn
    ASSERT_INT_EQUALS(ntos(80), g_recv->srcPort);

    cleanup();
    tcp_segment(&dev, (const uint8_t*) &ack, sizeof(tcp_hdr) + 1, 0xc0a80301);
    ASSERT_EQUALS(g_recv, NULL);

    tcp_close(last);
    ASSERT_INT_EQUALS(0x11, g_recv->flags); // ack the psh + fin

    cleanup();
}

