#include "net/tcp.h"
#include "tobytes.h"
#include "net/sbuff.h"
#include "net/arp.h"

#include "../tinytest.h"

#include <stdio.h>

static uint8_t * g_data = 0;
uint16_t g_len;
static void capture(struct netdevice *dev, sbuff* buff) {
    size_t len = buff->totalSize;
    const uint8_t* ptr = buff->data;
    g_data = malloc(len);
    memcpy(g_data, ptr, len);
    g_len = len;
}

void connection_refused() {
    struct BytesLen syn;
    syn = tobyteslen("92061fa31cf94a9700000000a0026db075c40000020406180402080a02ebc4ad0000000001030307");
    struct netdevice dev;
    dev.ip = 0xC0A80302;
    dev.send = capture;

    mac remote = {1,2,3,4,5,6};
    arp_store(remote, 0xc0a80301);
    tcp_segment(&dev, syn.bytes, syn.len, 0xc0a80301);

    ASSERT_EQUALS(54, g_len);
    ASSERT_EQUALS(0x0008, *(uint16_t*)(g_data+12));
    ASSERT_EQUALS(0x0203a8c0, *(uint32_t*)(g_data+26));
    ASSERT_EQUALS(0x0103a8c0, *(uint32_t*)(g_data+30));
    ASSERT_EQUALS(0x14, g_data[47]); // ack,rst
}

void connection_synack() {
    struct BytesLen syn;
    syn = tobyteslen("c7780050ee75886200000000a00272109ec30000020405b40402080a04316d060000000001030307");
    struct netdevice dev;
    dev.ip = 0xC0A80302;

    dev.send = capture;
    mac remote = {1,2,3,4,5,6};
    arp_store(remote, 0xc0a80301);
    tcp_segment(&dev, syn.bytes, syn.len, 0xc0a80301);

    ASSERT_INT_EQUALS(54, g_len);
    ASSERT_INT_EQUALS(0x0008, *(uint16_t*)(g_data+12));
    ASSERT_INT_EQUALS(0x0203a8c0, *(uint32_t*)(g_data+26));
    ASSERT_INT_EQUALS(0x0103a8c0, *(uint32_t*)(g_data+30));
    ASSERT_INT_EQUALS(0x12, g_data[47]); // ack,syn
}


