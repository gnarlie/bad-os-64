#include "net/udp.h"
#include "tobytes.h"
#include "net/sbuff.h"
#include "net/arp.h"
#include "memory.h"
#include "string.h"
#include "net/ntox.h"
#include "net/ip.h"
#include "net/device.h"

#include "../tinytest/tinytest.h"

static uint8_t * g_data = 0;
static uint16_t g_len;
static udp_quad g_quad;

static void udp_read(const udp_quad* quad, const uint8_t* data, uint32_t sz) {
    g_len = sz;
    g_data = malloc(g_len);
    memcpy(g_data, data, sz);
    g_quad = *quad;
}

static void capture(struct netdevice *dev, sbuff * sbuff) {
    g_len = sbuff->totalSize;
    g_data = malloc(g_len);
    memcpy(g_data, sbuff->data, g_len);
}


TEST(udp_send) {
    struct netdevice dev = {.ip = 9999, .send = capture};
    ip_add_device(&dev);

    mac mac;
    arp_store(mac, 0x11223344);

    uint8_t* reply =   tobytes("deadbeef");
    udp_quad quad = {
        .src_port = 8888, .dst_port = 7,
        .src_addr = dev.ip, .dst_addr = 0x11223344};

    ASSERT_INT_EQUALS(0, udp_send(&quad, reply, 4));

    ASSERT_INT_EQUALS(46, g_len);

    free(reply);
}

TEST(udp_listen) {
                            //  src dst len chk payload
    uint8_t* request = tobytes("e30a000700040000deadbeef");

    struct netdevice dev;
    dev.ip = 0;
    dev.send = capture;

    // seed arp
    udp_listen(7, udp_read);
    udp_datagram(&dev, request, 0xc0a80301);

    ASSERT_INT_EQUALS(4, g_len);
    ASSERT_INT_EQUALS(7, g_quad.dst_port);
    ASSERT_INT_EQUALS(dev.ip, g_quad.dst_addr);
    ASSERT_INT_EQUALS(58122, g_quad.src_port);
    ASSERT_INT_EQUALS(0xc0a80301, g_quad.src_addr);

    free(request);
}
