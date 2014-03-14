#include "net/ethernet.h"
#include "common.h"
#include "console.h"
#include "ntox.h"

#include "net/arp.h"
#include "net/sbuff.h"
#include "net/device.h"
#include "net/ip.h"

static void print_mac(const char * str, char* mac) {
    console_print_string(str);
    for(int i = 0; i < 6; i++) {
        if (i) console_print_string(":");
        console_put_hex8(mac[i]);
    }
}

inline static void assign(mac dest, const mac src) {
    memcpy(dest, src, 6);
}

void ethernet_send(sbuff* sbuff, uint16_t proto, mac dest, struct netdevice* device) {
    sbuff_pop(sbuff, sizeof(struct ethernet_frame));
    struct ethernet_frame* frame = (struct ethernet_frame*) sbuff->head;
    assign(frame->destination, dest);
    assign(frame->source, device->mac);
    frame->sizeOrType = ntos(proto);

    device->send(device, sbuff->head, sbuff->currSize);
}

sbuff * ethernet_sbuff_alloc(uint16_t size) {
    sbuff * p = raw_sbuff_alloc(size
                    + sizeof(struct ethernet_frame));
    sbuff_push(p, sizeof(struct ethernet_frame));
    return p;
}

void ethernet_packet(struct netdevice* dev, const uint8_t * data) {
    struct ethernet_frame *frame = (struct ethernet_frame*) data;
    uint16_t type = ntos(frame->sizeOrType);
    if (type == 0x0800) {
        ip_packet(dev, data + sizeof(struct ethernet_frame));
        return;
    }
    else if (type == 0x0806) {
        arp_packet(dev, data + sizeof(struct ethernet_frame));
        return;
    }
    else if (type == 0x86DD) {
        // no habla ipv6
        return;
    }
    else {
        console_print_string(" Unknown or ethernet frame type: ");
        console_put_hex16(type);
        console_print_string("\n");
    }
}
