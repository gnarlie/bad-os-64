#include "net/tcp.h"
#include "net/device.h"
#include "net/sbuff.h"
#include "net/ntox.h"
#include "net/ip.h"
#include "memory.h"
#include "console.h"

typedef struct stream_t {
    uint16_t localPort;
    uint32_t localAddr;
    uint16_t remotePort;
    uint32_t remoteAddr;

    uint32_t localSeq;
    uint32_t pendingAck;
    uint32_t ackSeq;

    uint8_t needsAck;

    struct stream_t* next;

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

static stream * all_streams = NULL;

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

static void header_from_stream(stream* stream, tcp_hdr* hdr, uint8_t flags) {
    hdr->srcPort = ntos(stream->localPort);
    hdr->destPort = ntos(stream->remotePort);
    hdr->sequence = ntol(stream->localSeq);
    hdr->ack = ntol(stream->ackSeq);
    hdr->offset = 5;
    hdr->reserved = 0;
    hdr->window = 12000;
    hdr->flags = flags | (stream->needsAck ? Ack : 0);
    hdr->chksum = 0;
}

void reset_stream(struct netdevice *dev, tcp_hdr* hdr, uint32_t srcIp) {
    stream stream = {ntos(hdr->destPort), ntol(dev->ip),
                     ntos(hdr->srcPort), srcIp,
                     0, 0, 0, 1 };
    sbuff * sb = ip_sbuff_alloc(sizeof(tcp_hdr));
    tcp_hdr * response = (tcp_hdr*) sb->head;
    header_from_stream(&stream, response, Rst);

    tcp_checksum(sb, sizeof(*hdr), dev->ip, srcIp);

    ip_send(sb, IPPROTO_TCP, srcIp, dev);
}

void connected(struct netdevice *dev,
        uint16_t localPort, uint16_t remotePort,
        uint32_t remoteAddr, uint32_t ackSeq) {
    uint32_t localSeq = 1;

    sbuff * sb = ip_sbuff_alloc(sizeof(tcp_hdr));
    tcp_hdr * hdr = (tcp_hdr*) sb->head;

    stream * s = kmem_alloc(sizeof(stream));
    s->localPort = localPort;
    s->remotePort = remotePort;
    s->localAddr = dev->ip;
    s->remoteAddr = remoteAddr;
    s->localSeq = localSeq;
    s->ackSeq = ackSeq + 1;
    s->needsAck = 1;

    s->next = all_streams;
    all_streams = s;

    header_from_stream(s, hdr, Syn | Ack);
    s->localSeq++;

    tcp_checksum(sb, sizeof(*hdr), dev->ip, remoteAddr);
    ip_send(sb, IPPROTO_TCP, remoteAddr, dev);
}

static void acked(stream * stream, tcp_hdr* hdr) {
    if (hdr->ack > stream->pendingAck) {
        stream->pendingAck = hdr->ack;
        // TODO: delete retransmit buffers here
    }
    else {
        // retransmit?
    }
}

static void tcp_send(struct netdevice *dev, stream *stream, uint8_t* data, uint16_t sz) {
    if (!sz) sz = strlen(data);

    //Segmentation would be good ... rcv window size, etc., etc.

    sbuff * sb = ip_sbuff_alloc(sizeof(tcp_hdr) + sz);
    tcp_hdr * response = (tcp_hdr*) sb->head;
    header_from_stream(stream, response, Psh);

    uint8_t* dst = sb->head + sizeof(*response);
    memcpy(dst, data, sz);

    tcp_checksum(sb, sizeof(*response) + sz, dev->ip, stream->remoteAddr);
    ip_send(sb, IPPROTO_TCP, stream->remoteAddr, dev);

    stream->localSeq += sz;
}

static void pushit(struct netdevice* dev, tcp_hdr *hdr,
        stream * stream, uint32_t len) {
    stream->ackSeq = ntol(hdr->sequence) + len;

    const char * body = (const char*)hdr + 4 * hdr->offset;
    console_print_string(body);

    tcp_send(dev, stream, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 26\r\n\r\n<html>Bad-OS 64</html>\r\n\r\n", 0);
}

static stream * find(struct netdevice *local, uint32_t src, tcp_hdr* hdr) {
    stream * stream = all_streams;
    while( stream ) {
        if (stream->localAddr == local->ip &&
                stream->remoteAddr == src &&
                stream->localPort == ntos(hdr->destPort) &&
                stream->remotePort == ntos(hdr->srcPort) ) {
            return stream;
        }

        stream = stream->next;
    }

    return NULL;
}


void tcp_segment(struct netdevice *dev, const uint8_t* data, uint32_t sz, uint32_t srcIp) {
    tcp_hdr * hdr = (tcp_hdr*)data;

    uint16_t dst = ntos(hdr->destPort);
    if (hdr->flags & Syn) {
        // are we listening on that port?
        if (dst != 80) {
            reset_stream(dev, hdr, srcIp);
        }
        else {
            connected(dev, dst, ntos(hdr->srcPort), srcIp, ntol(hdr->sequence) );
        }
    }

    if (hdr->flags & Ack) {
        stream * s = find(dev, srcIp, hdr);
        if (s) {
            acked(s, hdr);
        }
        else {
            reset_stream(dev, hdr, srcIp);
        }
    }

    if (hdr->flags & Psh) {
        stream * s = find(dev, srcIp, hdr);
        if ( !s) {
            reset_stream(dev, hdr, srcIp);
        }
        else {
            pushit(dev, hdr, s, sz - hdr->offset * 4);
        }
    }
}
