#pragma once

#include "net/device.h"
#include "common.h"

void udp_datagram(struct netdevice* dev, const uint8_t * data, uint32_t srcIp);

typedef struct udp_quad {
    uint32_t src_addr;
    uint32_t dst_addr;

    uint16_t src_port;
    uint16_t dst_port;
} udp_quad;

typedef void (*udp_notify)(const udp_quad* quad, const uint8_t* data, uint32_t size);
int udp_listen(int port, udp_notify on_read);
int udp_send(udp_quad* quad, const uint8_t* data, uint32_t size);
