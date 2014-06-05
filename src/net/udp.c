#include "net/udp.h"
#include "net/ntox.h"
#include "net/ip.h"
#include "net/sbuff.h"

typedef struct udp_header_t {
    uint16_t srcPort;
    uint16_t destPort;
    uint16_t len;
    uint16_t chksum;
} udp_header;

#include "console.h"
#include "memory.h"

static sbuff* udp_sbuff_alloc(uint32_t size) {
    sbuff * p = ip_sbuff_alloc(size
                    + sizeof(udp_header));

    sbuff_push(p, sizeof(udp_header));
    return p;
}

static void udp_send(sbuff * sb, uint16_t len,
        uint16_t dstPort, uint32_t dstIp,
        uint16_t srcPort, struct netdevice* dev) {

    sbuff_pop(sb, sizeof(udp_header));
    udp_header  * hdr = (udp_header*)sb->head;
    hdr->srcPort = ntos(srcPort);
    hdr->destPort = ntos(dstPort);
    hdr->len = ntos(len);
    checksum(hdr, len, &hdr->chksum); // TODO udp checksum fix needed

    ip_send(sb, 17, dstIp, dev);
}

void udp_datagram(struct netdevice* dev, const uint8_t * data, uint32_t srcIp) {
    const udp_header * udp = (const udp_header*)data;

    // echo server
    if (ntos(udp->destPort) == 7) {
        uint16_t len = ntos(udp->len);
        sbuff * sb = udp_sbuff_alloc(len);
        memcpy(sb->head, data + sizeof(*udp), len);
        udp_send(sb, len, ntos(udp->srcPort), srcIp, ntos(udp->destPort), dev);
    }
}
