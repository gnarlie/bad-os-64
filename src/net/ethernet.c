#include "net/ethernet.h"
#include "common.h"
#include "console.h"
#include "ntox.h"
#include "net/device.h"

struct ethernet_frame {
   mac destination;
   mac source;
   // 802.1Q tag - four bytes
   uint16_t sizeOrType;
} __attribute__ ((packed));

struct arp_packet {
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t operation;
    mac senderMac;
    uint32_t senderIp;
    mac targetMac;
    uint32_t targetIp;
} __attribute__ ((packed));

static mac myMac = {0xb0, 0xc4, 0x20, 0, 0, 0};
static uint32_t myIp = 0xC0A80302;

static void print_mac(const char * str, char* mac) {
    console_print_string(str);
    for(int i = 0; i < 6; i++) {
        if (i) console_print_string(":");
        console_put_hex8(mac[i]);
    }
}

inline static void assign(mac dest, const mac src) {
    for (int i = 0; i < 6; i++)
        dest[i] = src[i];
}

extern void ethernet_send(const void*, uint16_t);

void reply(struct netdevice* dev, mac target, uint32_t ip) {
    struct reply {
        struct ethernet_frame frame;
        struct arp_packet arp;
    } __attribute__ ((packed));

    struct reply reply;
    assign(reply.frame.destination, target);
    assign(reply.frame.source, myMac);
    reply.frame.sizeOrType = ntos(0x0806);
    reply.arp.htype = ntos(1);
    reply.arp.ptype = ntos(0x0800);
    reply.arp.hlen = 6;
    reply.arp.plen = 4;
    reply.arp.operation = ntos(2);
    assign(reply.arp.senderMac, myMac);
    reply.arp.senderIp = ntol(myIp);
    assign(reply.arp.targetMac, target);
    reply.arp.targetIp = ntol(ip);

    dev->send(dev, &reply, sizeof(reply));
}

void arp_packet(struct netdevice* dev, const uint8_t * data) {
    struct arp_packet * arp = (struct arp_packet*) data;

    console_print_string(" arp ");

    int request = ntos(arp->operation) == 1;
    console_print_string(request ? "request\n" : "reply\n");
    print_mac("target: ", arp->targetMac);
    console_print_string(" ");
    console_put_hex(ntol(arp->targetIp));

    print_mac("\nsender: ", arp->senderMac);
    console_print_string(" ");
    console_put_hex(ntol(arp->senderIp));

    console_print_string("\n");

    if (request && ntol(arp->targetIp) == myIp) {
        reply(dev, arp->senderMac, ntol(arp->senderIp));
    }
}

void ip_packet(const uint8_t* data) {
    console_print_string(" ip\n");
}

void ethernet_packet(struct netdevice* dev, const uint8_t * data) {
    struct ethernet_frame *frame = (struct ethernet_frame*) data;
    print_mac("\ndst: ", frame->destination);
    print_mac(" src: ", frame->source);
    uint16_t type = ntos(frame->sizeOrType);
    if (type == 0x0800) {
        ip_packet(data + sizeof(struct ethernet_frame));
        return;
    }
    else if (type == 0x0806) {
        arp_packet(dev, data + sizeof(struct ethernet_frame));
        return;
    }
    else {
        console_print_string(" Unknown or ethernet frame type: ");
        console_put_hex(type);
        console_print_string(" ");
        console_put_hex(frame->sizeOrType);
        console_print_string(" ");
        console_put_hex16(frame->sizeOrType << 8);
        console_print_string(" ");
        console_put_hex16(frame->sizeOrType >> 8);
        console_print_string(" ");
        console_put_hex16(frame->sizeOrType << 8 | frame->sizeOrType >> 8);
        console_print_string("\n");
    }
}
