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
    struct stream_t *next;
    struct netdevice * dev;

    uint16_t localPort;
    uint16_t remotePort;
    uint32_t localAddr;
    uint32_t remoteAddr;

    uint32_t localSeq;
    uint32_t pendingAck;
    uint32_t ackSeq;

    uint8_t needsAck;

    TcpState state;

    uint8_t *readBuf;
    uint32_t readOffset;
    uint32_t readMax;

    tcp_read_fn readFn;

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
static listen_state * all_listeners = NULL;

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
    hdr->window = ntos(stream->readMax - stream->readOffset);
    hdr->flags = flags | (stream->needsAck ? Ack : 0);
    hdr->chksum = 0;

    stream->needsAck = 0;
}

static void reset_stream(struct netdevice *dev, tcp_hdr* hdr, uint32_t srcIp) {
    stream stream = {0, dev, ntos(hdr->destPort), ntol(dev->ip),
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
    s->needsAck = 0;
    s->state = SynReceived;

    s->readFn = fn;
    s->readBuf = (void*)(s + 1);
    s->readOffset = 0;
    s->readMax = MaxReadBufffer;

    s->next = all_streams;
    all_streams = s;

    // reply with syn-ack
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
    if (stream->state == FinWait1) stream->state = FinWait2;
    if (stream->state == LastAck) remove_stream(stream);
}

void tcp_close(stream *stream) {
    sbuff * sb = ip_sbuff_alloc(sizeof(tcp_hdr));
    tcp_hdr * response = (tcp_hdr*) sb->head;
    header_from_stream(stream, response, Fin);
    stream->state = FinWait1;

    tcp_checksum(sb, sizeof(*response), stream->dev->ip, stream->remoteAddr);
    ip_send(sb, IPPROTO_TCP, stream->remoteAddr, stream->dev);
    stream->localSeq ++;
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
    if (!len) return;

    if (stream->readOffset + len > stream->readMax) {
        console_print_string("Out of buffer space in read ... dropping packet\n");
        return;
    }

    stream->ackSeq = ntol(hdr->sequence) + len;

    const uint8_t* data = (const uint8_t*)hdr + 4 * hdr->offset;
    memcpy(stream->readBuf + stream->readOffset, data, len);
    stream->readOffset += len;
    stream->needsAck = 1;
}

static void pushit(stream * stream) {
    stream->readFn(stream, stream->readBuf, stream->readOffset);
    stream->readOffset = 0;
}

static void time_wait(stream* stream) {
    // TODO ... schedule this for 2ms from now

    remove_stream(stream);
}

static void fin(struct netdevice * dev, stream * s) {
    if (s->state == FinWait2) {
        // ack their fin
        sbuff * sb = ip_sbuff_alloc(sizeof(tcp_hdr));
        tcp_hdr * response = (tcp_hdr*) sb->head;
        s->ackSeq++;
        header_from_stream(s, response, Ack);

        tcp_checksum(sb, sizeof(*response), dev->ip, s->remoteAddr);
        ip_send(sb, IPPROTO_TCP, s->remoteAddr, dev);
        s->state = TimeWait;

        time_wait(s);
    }
    else if (s->state == Established) {
        // send a fin/ack ... we're assuming the 'application'
        // has no more data, otherwise we'd need to Ack, wait for
        // upper layer to shutdown/close, then Fin.
        //
        // But skip all that and the CloseWait state. Go straight to LastAck
        //
        sbuff * sb = ip_sbuff_alloc(sizeof(tcp_hdr));
        tcp_hdr * response = (tcp_hdr*) sb->head;
        s->ackSeq++;
        header_from_stream(s, response, Fin | Ack);

        tcp_checksum(sb, sizeof(*response), dev->ip, s->remoteAddr);
        ip_send(sb, IPPROTO_TCP, s->remoteAddr, dev);
        s->state = LastAck;
    }
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
        if (l->port == dst) break; // ONEDAY listen on iface? ...
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
        pushit(s);
    }

    if (hdr->flags & Fin) {
        fin(dev, s);
    }
}
