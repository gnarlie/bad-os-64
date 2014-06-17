#include "net/tcp.h"
#include "net/device.h"
#include "net/sbuff.h"
#include "net/ntox.h"
#include "net/ip.h"
#include "memory.h"

typedef struct stream_t {
    uint16_t localPort;
    uint16_t remotePort;
    uint32_t remoteAddr;

    uint32_t localSeq;
    uint32_t ackSeq;

    uint8_t needsAck;
} stream;

typedef struct {
    uint32_t src;
    uint32_t dst;
    uint8_t zero;
    uint8_t proto;
    uint16_t len;
} tcp_pseudo_hdr;

enum {
    Fin = 0x001,
    Syn = 0x002,
    Rst = 0x004,
    Psh = 0x008,
    Ack = 0x010,
    Urg = 0x020,
    Ece = 0x040,
    Cwr = 0x080,
    Ns = 0x100
};

static void tcp_checksum(sbuff *sb, uint16_t len, uint32_t src, uint32_t dest) {
    tcp_hdr * hdr = (tcp_hdr*) sb->head;

    tcp_pseudo_hdr *pseudo = (tcp_pseudo_hdr*)(sb->head - sizeof(tcp_pseudo_hdr));
    pseudo->src = ntol(src);
    pseudo->dst = ntol(dest);
    pseudo->zero = 0;
    pseudo->proto = IPPROTO_TCP;
    pseudo->len = ntos(len);
    checksum(pseudo, sizeof(*pseudo) + len, &hdr->chksum);
}

void reset_stream(struct netdevice *dev, stream* stream) {
    sbuff * sb = ip_sbuff_alloc(sizeof(tcp_hdr));
    tcp_hdr * hdr = (tcp_hdr*) sb->head;

    hdr->srcPort = ntos(stream->localPort);
    hdr->destPort = ntos(stream->remotePort);
    hdr->sequence = ntol(stream->localSeq);
    hdr->ack = ntol(stream->ackSeq);
    hdr->offset = 5;
    hdr->reserved = 0;
    hdr->flags = Rst | (stream->needsAck ? Ack : 0);
    hdr->window = 0;

    tcp_checksum(sb, sizeof(*hdr), dev->ip, stream->remoteAddr);

    ip_send(sb, IPPROTO_TCP, stream->remoteAddr, dev);
}

void connected(struct netdevice *dev,
        uint16_t localPort, uint16_t remotePort,
        uint32_t remoteAddr, uint32_t ackSeq) {
    uint32_t localSeq = 100;

    sbuff * sb = ip_sbuff_alloc(sizeof(tcp_hdr));
    tcp_hdr * hdr = (tcp_hdr*) sb->head;

    hdr->srcPort = ntos(localPort);
    hdr->destPort = ntos(remotePort);
    hdr->sequence = ntol(localSeq);
    hdr->ack = ntol(ackSeq + 1);
    hdr->offset = 5;
    hdr->reserved = 0;
    hdr->flags = Syn | Ack;
    hdr->window = ntos(65536 / 2);
    hdr->chksum = 0;

    tcp_checksum(sb, sizeof(*hdr), dev->ip, remoteAddr);

    ip_send(sb, IPPROTO_TCP, remoteAddr, dev);
}

void tcp_segment(struct netdevice *dev, const uint8_t* data, uint32_t srcIp) {
    tcp_hdr * hdr = (tcp_hdr*)data;

    uint16_t dst = ntos(hdr->destPort);
    if (hdr->flags & Syn) {
        // are we listening on that port?
        if (dst != 80) {
            stream s = {dst, ntos(hdr->srcPort), srcIp, 0, ntol(hdr->sequence), 1 };
            reset_stream(dev, &s);
        }
        else {
            connected(dev, dst, ntos(hdr->srcPort), srcIp, ntol(hdr->sequence) );
        }
    }
}
