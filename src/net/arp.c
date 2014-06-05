#include "net/arp.h"

#include "net/sbuff.h"
#include "net/ntox.h"
#include "net/ethernet.h"

static mac BROADCAST_MAC = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

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


static void reply(struct netdevice* dev, mac target, uint32_t ip, int broadcast) {
    sbuff * buf = raw_sbuff_alloc(sizeof(struct ethernet_frame) + sizeof(struct arp_packet));
    add_ref(buf);
    sbuff_push(buf, sizeof(struct ethernet_frame));

    struct arp_packet * arp = (struct arp_packet*)buf->head;
    arp->htype = ntos(1);
    arp->ptype = ntos(0x0800);
    arp->hlen = 6;
    arp->plen = 4;
    arp->operation = ntos(2);
    memcpy(arp->senderMac, dev->mac, 6);
    arp->senderIp = ntol(dev->ip);
    memcpy(arp->targetMac, target, 6);
    arp->targetIp = ntol(ip);

    ethernet_send(buf, 0x0806, broadcast ? BROADCAST_MAC : target, dev);
    release_ref(buf, sbuff_free);
}

static void arp_request(struct netdevice * device, uint32_t targetIp) {
    sbuff * buf = ethernet_sbuff_alloc(sizeof(struct arp_packet));
    add_ref(buf);

    struct arp_packet * arp = (struct arp_packet*)buf->head;
    arp->htype = ntos(1);
    arp->ptype = ntos(0x0800);
    arp->hlen = 6;
    arp->plen = 4;
    arp->operation = ntos(1);
    memcpy(arp->senderMac, device->mac, 6);
    arp->senderIp = ntol(device->ip);
    bzero(arp->targetMac, 6);
    arp->targetIp = ntol(targetIp);

    ethernet_send(buf, 0x0806, BROADCAST_MAC, device);
    release_ref(buf, sbuff_free);
}

void gratuitous_arp(struct netdevice * device) {
    static mac zeros = {0,0,0,0,0,0};
    reply(device, zeros, device->ip, 1);
}

struct arp_table {
    uint32_t ips[128];
    mac macs[128];
    uint16_t count;
};

static struct arp_table table;
int arp_lookup(struct netdevice* dev, uint32_t ip, mac dest) {
    for (int e = 0; e < table.count; e++) {
        if (table.ips[e] == ip) {
            memcpy(dest, table.macs[e], 6);
            return 1;
        }
    }

    arp_request(dev, ip);
    return 0;
}

void arp_store(mac dest, uint32_t ip) {
    table.ips[table.count] = ip;
    memcpy(table.macs[table.count], dest, 6);

    table.count++;

    //TODO overflow of table
}

void arp_packet(struct netdevice* dev, const uint8_t * data) {
    struct arp_packet * arp = (struct arp_packet*) data;

    if (ntol(arp->targetIp) != dev-> ip) return;

    int request = ntos(arp->operation) == 1;
    if (request) {
        // store(arp->senderMac, ntol(arp->senderIp));
        reply(dev, arp->senderMac, ntol(arp->senderIp), 0);
    }
    else {
        arp_store(arp->senderMac, ntol(arp->senderIp));
    }
}

void init_arp() {
    table.count = 0;
}

