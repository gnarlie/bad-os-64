#include "net/udp.h"
#include "tobytes.h"
#include "net/sbuff.h"
#include "net/arp.h"
#include "memory.h"
#include "string.h"

#include "../tinytest/tinytest.h"

uint8_t * g_data = 0;
uint16_t g_len;
static void capture(struct netdevice *dev, sbuff * sbuff) {
    g_len = sbuff->totalSize;
    g_data = malloc(g_len);
    memcpy(g_data, sbuff->data, g_len);
}


TEST(udp_echo) {
    uint8_t *arp = tobytes("0001080006040002aad745548156c0a80301b0c420000000c0a80302");
    uint8_t* request = tobytes("e30a000700040000deadbeef");
                            //  src dst len chk payload
    uint8_t* reply =   tobytes("0007e30a00041ceedeadbeef");

    struct netdevice dev;
    dev.ip = 0xC0A80302;
    dev.send = capture;

    // seed arp
    arp_packet(&dev, arp);
    udp_datagram(&dev, request, 0xc0a80301);

    ASSERT_INT_EQUALS(46, g_len);
    // 34 is sizeof(ip hdr) + sizeof(eth hdr)
    for (int i = 0; i < g_len - 34; ++i) {
        ASSERT_INT_EQUALS(reply[i], g_data[i+34]);
    }

    free(arp);
    free(request);
}
