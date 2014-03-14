#include "net/ip.h"
#include "net/arp.h"
#include "net/device.h"
#include "../tinytest.h"

#include <string.h>

void checksums() {
    char bytes[] = {
        0x45, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x05, 0x01, 0x00, 0x00, 0xc0, 0xa8, 0x03, 0x02,
        0xc0, 0xa8, 0x03, 0x01
    };

    ASSERT_EQUALS(0xf6a9,
            checksum(bytes, sizeof(bytes), (uint16_t*)(bytes + 8)));
    ASSERT_EQUALS(0xa9f6,
            *(uint16_t*)(bytes + 8));
}

uint8_t gCalled = 0;
void capture(struct netdevice* dev, const void* p, uint16_t capturedLen) {
    ASSERT_EQUALS(98, capturedLen);
    char reply[] = {0x45, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, 0xf3, 0x55, 0xc0, 0xa8, 0x03, 0x02, 0xc0, 0xa8, 0x03, 0x01, 0x00, 0x00, 0xed, 0xa8, 0x0e, 0xb2, 0x03, 0xa5, 0x2d, 0x77, 0x1b, 0x53, 0x00, 0x00, 0x00, 0x00, 0x6f, 0x38, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, };

    for (int i = 14; i < capturedLen; i++) {
        ASSERT_EQUALS(reply[i-14], ((char*)p)[i]);
    }

    capturedLen = capturedLen;
    gCalled = 1;
}

uint8_t fromhex(char c) {
    if (c >= 'a' && c <= 'f') return 10 + c - 'a';
    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
    return c - '0';
}

uint8_t * tobytes(const char * s) {
    int len = strlen(s) / 2;
    uint8_t * ret = malloc(len);
    for (int i = 0; i < len; i++) {
        ret[i] = (fromhex(s[i*2]) << 4) + fromhex(s[i*2+1]);
    }

    return ret;
}

void icmp_ping_replied() {

    uint8_t *arp = tobytes("000108000604000212c937989189c0a80301b0c420000000c0a80302");
    uint8_t *request = tobytes("45000054d19440004001e1c0c0a80301c0a8030208006bd30eb203a52d771b53000000006f38030000000000101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f3031323334353637");
    struct netdevice dev;
    dev.ip = 0xC0A80302;
    dev.send = capture;

    arp_packet(&dev, arp);
    ip_packet(&dev, request);

    ASSERT_EQUALS(gCalled, 1);

    free(arp);
    free(request);
}
