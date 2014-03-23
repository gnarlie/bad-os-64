#include "net/udp.h"
#include "net/ntox.h"

typedef struct udp_header_t {
    uint16_t srcPort;
    uint16_t destPort;
    uint16_t len;
    uint16_t chksum;
} udp_header;

#include "console.h"
#include "memory.h"

void udp_datagram(struct netdevice* dev, const uint8_t * data, uint32_t srcIp) {
    const udp_header * udp = (const udp_header*)data;

    if (ntos(udp->destPort) == 8080) {
        char * d = kmem_alloc(udp->len +1);
        memcpy(d, data + sizeof(*udp), udp->len);
        d[udp->len] = 0;
        console_print_string(d);
    }
}
