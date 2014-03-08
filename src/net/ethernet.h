#pragma once

#include "common.h"
#include "net/device.h"
#include "net/sbuff.h"

struct ethernet_frame {
   mac destination;
   mac source;
   // 802.1Q tag - four bytes
   uint16_t sizeOrType;
} __attribute__ ((packed));

void ethernet_packet(struct netdevice * device, const uint8_t *packet);
void ethernet_send(sbuff* sbuff, uint16_t proto, mac dest, struct netdevice* device);
