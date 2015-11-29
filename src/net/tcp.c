#include "net/tcp.h"
#include "net/device.h"
#include "net/sbuff.h"
#include "net/ntox.h"
#include "net/ip.h"
#include "memory.h"
#include "errno.h"
#include "console.h"

typedef enum TcpState_t {
    Closed,
    Listen,
    SynReceived,
    SynSent,
    Established,
    FinWait1,
    FinWait2,
    Closing,
    TimeWait,
    CloseWait,
    LastAck
} TcpState;

typedef struct stream_t {
    struct netdevice * dev;

    uint16_t localPort;
    uint32_t localAddr;
    uint16_t remotePort;
    uint32_t remoteAddr;

    uint32_t localSeq;
    uint32_t pendingAck;
    uint32_t ackSeq;

    uint8_t needsAck;

    TcpState state;

    struct stream_t* next;
    uint8_t *read_buf;
    uint32_t read_offset;
    uint32_t read_max;

    tcp_read_fn read_fn;

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


typedef struct listen_state_t {
    uint16_t port;
    tcp_read_fn (*accept)();
    struct listen_state_t * next;
} listen_state;

static stream * all_streams = NULL;
static listen_state * all_listeners;

static void remove_stream(stream * s) {
    stream * before = all_streams;
    while(before && before->next != s) {
        before = before->next;
    }

    if (before)
        before->next = s->next;
    else
        all_streams = s->next;

    kmem_free(s);
}

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

static void reset_stream(struct netdevice *dev, tcp_hdr* hdr, uint32_t srcIp) {
    stream stream = {dev, ntos(hdr->destPort), ntol(dev->ip),
                     ntos(hdr->srcPort), srcIp,
                     0, 0, 0, 1 };
    sbuff * sb = ip_sbuff_alloc(sizeof(tcp_hdr));
    tcp_hdr * response = (tcp_hdr*) sb->head;
    header_from_stream(&stream, response, Rst);

    tcp_checksum(sb, sizeof(*hdr), dev->ip, srcIp);

    ip_send(sb, IPPROTO_TCP, srcIp, dev);
}

static void connected(struct netdevice *dev,
        uint16_t localPort, uint16_t remotePort,
        uint32_t remoteAddr, uint32_t ackSeq, tcp_read_fn fn) {
    uint32_t localSeq = 1;
    const uint32_t MaxReadBufffer = 2048;

    sbuff * sb = ip_sbuff_alloc(sizeof(tcp_hdr));
    tcp_hdr * hdr = (tcp_hdr*) sb->head;

    stream * s = kmem_alloc(sizeof(stream) + MaxReadBufffer);
    s->dev = dev;
    s->localPort = localPort;
    s->remotePort = remotePort;
    s->localAddr = dev->ip;
    s->remoteAddr = remoteAddr;
    s->localSeq = localSeq;
    s->ackSeq = ackSeq + 1;
    s->needsAck = 1;
    s->state = SynReceived;

    s->read_fn = fn;
    s->read_buf = (void*)(s + 1);
    s->read_offset = 0;
    s->read_max = MaxReadBufffer;

    s->next = all_streams;
    all_streams = s;

    header_from_stream(s, hdr, Syn | Ack);
    s->localSeq++;

    tcp_checksum(sb, sizeof(*hdr), dev->ip, remoteAddr);
    ip_send(sb, IPPROTO_TCP, remoteAddr, dev);
}

static void acked(stream * stream, tcp_hdr* hdr) {
    if (hdr->ack >= stream->pendingAck) {
        stream->pendingAck = hdr->ack;
        // TODO: delete retransmit buffers here
    }
    else {
        // retransmit?
    }

    if (stream->state == SynReceived) stream->state = Established;
    if (stream->state == LastAck) remove_stream(stream);
}

void tcp_send(stream *stream, const void* data, uint16_t sz) {
    //Segmentation would be good ... rcv window size, etc., etc.

    sbuff * sb = ip_sbuff_alloc(sizeof(tcp_hdr) + sz);
    tcp_hdr * response = (tcp_hdr*) sb->head;
    header_from_stream(stream, response, Psh);

    uint8_t* dst = sb->head + sizeof(*response);
    memcpy(dst, data, sz);

    tcp_checksum(sb, sizeof(*response) + sz, stream->dev->ip, stream->remoteAddr);
    ip_send(sb, IPPROTO_TCP, stream->remoteAddr, stream->dev);

    stream->localSeq += sz;
}

static void buffer_data(struct netdevice* dev, tcp_hdr *hdr,
        stream * stream, uint32_t len) {
    if (stream->read_offset + len > stream->read_max) {
        console_print_string("Out of buffer space in read ... dropping packet\n");
        return;
    }

    stream->ackSeq = ntol(hdr->sequence) + len;
    const uint8_t* data = (const uint8_t*)hdr + 4 * hdr->offset;
    memcpy(stream->read_buf + stream->read_offset, data, len);
    stream->read_offset += len;
}

static void pushit(struct netdevice* dev, tcp_hdr *hdr,
        stream * stream, uint32_t len) {
    stream->read_fn(stream, stream->read_buf, stream->read_offset);
    stream->read_offset = 0;

//    console_print_string(body);
//
//    const char* response =  "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 26\r\n\r\n<html>Bad-OS 64</html>\r\n\r\n";
//    tcp_send(stream, response, strlen(response));
}

static void fin(struct netdevice * dev, stream * s) {
    // send a fin/ack ... we're assuming the 'application'
    // has no more data, otherwise we'd need to Ack, wait for
    // upper layer to shutdown/close, then Fin
    stream * before;
    sbuff * sb = ip_sbuff_alloc(sizeof(tcp_hdr));
    tcp_hdr * response = (tcp_hdr*) sb->head;
    s->ackSeq++;
    header_from_stream(s, response, Fin | Ack);

    tcp_checksum(sb, sizeof(*response), dev->ip, s->remoteAddr);
    ip_send(sb, IPPROTO_TCP, s->remoteAddr, dev);
    s->state = LastAck;

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

static void syn(struct netdevice * dev, tcp_hdr *hdr, uint32_t srcIp) {
    uint16_t dst = ntos(hdr->destPort);

    listen_state * l = all_listeners;
    while(l) {
        if (l->port == dst) break;
        l = l->next;
    }

    if (l == NULL) {
        reset_stream(dev, hdr, srcIp);
    }
    else {
        tcp_read_fn fn = l->accept();
        connected(dev, dst, ntos(hdr->srcPort), srcIp, ntol(hdr->sequence), fn);
    }
}

int listen(uint16_t port, tcp_read_fn (*accept)()) {
    listen_state * l = all_listeners;
    while(l) {
        if (l->port == port) return EADDRINUSE;
        l = l->next;
    }

    l = kmem_alloc(sizeof(listen_state));
    l->next = all_listeners;
    all_listeners = l;
    l->port = port;
    l->accept = accept;

    return EOK;
}

void tcp_segment(struct netdevice *dev, const uint8_t* data, uint32_t sz, uint32_t srcIp) {
    tcp_hdr * hdr = (tcp_hdr*)data;

    if (hdr->flags & Syn) {
        syn(dev, hdr, srcIp);
    }

    stream * s = find(dev, srcIp, hdr);
    if (!s) {
        reset_stream(dev, hdr, srcIp);
        return;
    }

    if (hdr->flags & Ack) {
        acked(s, hdr);
    }

    buffer_data(dev, hdr, s, sz - hdr->offset * 4);

    if (hdr->flags & Psh) {
        pushit(dev, hdr, s, sz - hdr->offset * 4);
    }

    if (hdr->flags & Fin) {
        fin(dev, s);
    }
}
