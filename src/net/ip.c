#include "net/ip.h"
#include "net/device.h"
#include "net/ethernet.h" // TODO: remove
#include "net/ntox.h"
#include "net/sbuff.h"

#include "console.h"

struct ipv4_header {
    uint8_t ihl : 4;
    uint8_t version : 4;
    uint8_t ecn : 2;
    uint8_t dscp : 6;
    uint16_t total_len;
    uint16_t id;
    uint16_t fragmentOffset : 13;
    uint16_t flags : 3;
    uint8_t ttl;
    uint8_t proto;
    uint16_t checksum;
    uint32_t src;
    uint32_t dest;
} __attribute__ ((packed));

static void console_put_ip(uint32_t ip) {
    console_put_dec(ip >> 24 & 0xff);
    console_put_dec(ip >> 16 & 0xff); console_print_string(".");
    console_put_dec(ip >> 8 & 0xff); console_print_string(".");
    console_put_dec(ip & 0xff); console_print_string(".");
}

sbuff* ip_sbuff_alloc(uint16_t size) {
    sbuff * p = raw_sbuff_alloc(size
                    + sizeof(struct ethernet_frame)
                    + sizeof(struct ipv4_header));

    sbuff_push(p, sizeof(struct ethernet_frame) + sizeof(struct ipv4_header));
    return p;
}

void icmp_segment(uint32_t sender, struct netdevice* dev, const uint8_t* data, uint16_t len) {
    typedef struct icmp_header_t{
        uint8_t type;
        uint8_t code;
        uint16_t checksum;
        uint16_t id;
        uint16_t sequence;
        uint8_t payload[];
    } icmp_header;

    icmp_header * icmp = (icmp_header*) data;

    if (len < sizeof(icmp_header)) {
        warn("ICMP packet too small");
        return;
    }

    if (icmp->type == 8) { // echo request
        sbuff* buf = ip_sbuff_alloc(len);
        add_ref(buf);
        icmp_header * pong = (icmp_header*) buf->head;
        bzero(pong, sizeof(icmp_header));
        pong->type = pong->code = 0;
        pong->id = icmp->id;
        pong->sequence = icmp->sequence;
        pong->checksum = 0;
        checksum(pong, len, &pong->checksum);
        memcpy(pong->payload, icmp->payload, len - sizeof(icmp_header));
        ip_send(buf, 1, sender, dev);
        release_ref(buf, sbuff_free);
    }
}

uint32_t arp_lookup(uint32_t ip, mac dest) {
    //TODO ... actual ARP lookup
    for(int i = 0;i < 6; i++)
        dest[i] = 0xff;
}

void ip_send(sbuff* sbuff, uint8_t proto, uint32_t dest, struct netdevice* device) {
    sbuff_pop(sbuff, sizeof(struct ipv4_header));
    uint16_t len = sbuff->currSize;
    struct ipv4_header *hdr = (struct ipv4_header*)sbuff->head;
    hdr->ihl = 5;
    hdr->version = 4;
    hdr->dscp = hdr->ecn = 0;
    hdr->total_len = ntos(len);
    hdr->id = 0;
    hdr->flags = 0;
    hdr->fragmentOffset = 0;
    hdr->ttl = 64;
    hdr->proto = proto;
    hdr->checksum = 0;
    hdr->src = ntol(device->ip);
    hdr->dest = ntol(dest);
    checksum(hdr, sizeof(*hdr), &hdr->checksum);

    static mac destMac;
    arp_lookup(dest, destMac);

    ethernet_send(sbuff, 0x0800u, destMac, device);
}

void ip_packet(struct netdevice* dev, const uint8_t* data) {
    struct ipv4_header* ip = (struct ipv4_header*) data;

    uint16_t hdrLen = ip->ihl * 4;
    switch(ip->proto) {
        case(1) : icmp_segment(ntol(ip->src), dev,
                          data + hdrLen,
                          ntos(ip->total_len) - hdrLen);
                  break;
        default:
            warn("Unsupported IP protocol");
    }
}

