#pragma once

#include "common.h"
#include "net/ntox.h"

struct netdevice;
struct sbuff_t;

#define IPPROTO_TCP 6

void ip_packet(struct netdevice* dev, const uint8_t* data);
int ip_send(struct sbuff_t* sbuff, uint8_t proto, uint32_t dest, struct netdevice *);
struct sbuff_t* ip_sbuff_alloc(uint16_t sz);

struct netdevice * ip_resolve_local(uint32_t addr);
void ip_add_device(struct netdevice * dev);

static inline uint16_t checksum(const void *buf, uint32_t len, uint16_t* dest) {
    const uint8_t* data= (const uint8_t*)buf;
    uint32_t sum = 0;
    *dest = 0;

    for (int i = 0; i < len; i+=2) {
        uint16_t w = ((data[i]<<8) & 0xff00) +
                        (data[i+1] & 0xff);
        sum += w;
    }
    while(sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    sum = ~sum;
    *dest = ntos(sum);

    return sum;
}
