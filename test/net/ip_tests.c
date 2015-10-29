#include "net/ip.h"
#include "net/arp.h"
#include "net/device.h"
#include "../tinytest.h"
#include "tobytes.h"
#include "net/sbuff.h"

#include <string.h>

void checksums() {
    uint8_t * bytes = tobytes("450038000000000000000000c0a80302c0a80301");

    ASSERT_EQUALS(0xfbaa,
            checksum(bytes, 20, (uint16_t*)(bytes + 8)));
    ASSERT_EQUALS(0xaafb,
            *(uint16_t*)(bytes + 8));

    free(bytes);
}

uint8_t gCalled = 0;
void capture(struct netdevice* dev, sbuff * sbuff) {
    size_t capturedLen = sbuff->totalSize;
    ASSERT_INT_EQUALS(98, capturedLen);
    char *reply = tobytes("45000054000000004001f355c0a80302c0a803010000eda80eb203a52d771b53000000006f38030000000000101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f3031323334353637");

    uint8_t* p = sbuff->data;
    for (int i = 14; i < capturedLen; i++) {
        ASSERT_EQUALS(reply[i-14], ((char*)p)[i]);
    }

    capturedLen = capturedLen;
    gCalled = 1;
    free(reply);
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
