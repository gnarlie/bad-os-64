#include "net/udp.h"
#include "net/ntox.h"
#include "net/ip.h"
#include "net/sbuff.h"
#include "errno.h"
#include "util/list.h"
#include "memory.h"

typedef struct udp_header_t {
    uint16_t srcPort;
    uint16_t destPort;
    uint16_t len;
    uint16_t chksum;
} udp_header;

typedef struct udp_listening_port {
    uint16_t port;
    udp_notify cb;
} udp_listening_port;

static list_t listening_ports;

static sbuff* udp_sbuff_alloc(uint32_t size) {
    sbuff * p = ip_sbuff_alloc(size
                    + sizeof(udp_header));

    sbuff_push(p, sizeof(udp_header));
    return p;
}

static int do_send(sbuff * sb, uint16_t len,
        uint16_t dstPort, uint32_t dstIp,
        uint16_t srcPort, struct netdevice* dev) {

    sbuff_pop(sb, sizeof(udp_header));
    udp_header  * hdr = (udp_header*)sb->head;
    hdr->srcPort = ntos(srcPort);
    hdr->destPort = ntos(dstPort);
    hdr->len = ntos(len);
    checksum(hdr, len, &hdr->chksum); // TODO udp checksum fix needed

    ip_send(sb, 17, dstIp, dev);

    return EOK;
}

void udp_datagram(struct netdevice* dev, const uint8_t * data, uint32_t srcIp) {
    const udp_header * udp = (const udp_header*)data;

    list_node * node;
    LIST_FOREACH(listening_ports, node) {
        udp_listening_port* p = node->payload;
        if (ntos(udp->destPort) == p->port) {
            udp_quad quad;
            quad.dst_port = ntos(udp->destPort);
            quad.dst_addr = ntol(dev->ip);
            quad.src_port = ntos(udp->srcPort);
            quad.src_addr = srcIp;

            p->cb(&quad, data + sizeof(udp), ntos(udp->len));
        }
    }
}

int udp_listen(int port, udp_notify on_read) {
    udp_listening_port * l = kmem_alloc(sizeof(udp_listening_port));
    l->cb = on_read;
    l->port = port;

    list_append(&listening_ports, l);

    return EOK;
}

#include "console.h"

int udp_send(udp_quad* quad, const uint8_t* data, uint32_t size) {
    sbuff * sb = udp_sbuff_alloc(size);
    memcpy(sb->head, data, size);

    struct netdevice * dev = ip_resolve_local(quad->src_addr);
    if (! dev) return EINVALID;

    sbuff_pop(sb, sizeof(udp_header));
    udp_header  * hdr = (udp_header*)sb->head;
    hdr->srcPort = ntos(quad->src_port);
    hdr->destPort = ntos(quad->dst_port);
    hdr->len = ntos(size);
    checksum(hdr, size, &hdr->chksum); // TODO udp checksum fix needed

    return ip_send(sb, 17, quad->dst_addr, dev);
}

