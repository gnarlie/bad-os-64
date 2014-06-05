#include "net/tcp.h"
#include "net/device.h"
#include "net/sbuff.h"
#include "net/ntox.h"
#include "net/ip.h"

typedef struct stream_t {
    uint16_t localPort;
    uint16_t remotePort;
    uint32_t remoteAddr;

    uint32_t localSeq;
    uint32_t ackSeq;

    uint8_t needsAck;
} stream;

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

    checksum(hdr, sizeof(*hdr), &hdr->chksum);

    ip_send(sb, IPPROTO_TCP, stream->remoteAddr, dev);
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
    }

}
