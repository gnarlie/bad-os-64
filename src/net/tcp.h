#pragma once

#include "common.h"

struct netdevice;

typedef struct tcp_hdr_t {
    uint16_t srcPort;
    uint16_t destPort;
    uint32_t sequence;
    uint32_t ack;
    uint8_t reserved : 4;
    uint8_t offset : 4;
    uint8_t flags;
    uint16_t window;
    uint16_t chksum;
    uint32_t options[];
} tcp_hdr;

void tcp_segment(struct netdevice *dev, const uint8_t* data, uint32_t ip);
