#pragma once

#include "net/device.h"
#include "common.h"

void udp_datagram(struct netdevice* dev, const uint8_t * data, uint32_t srcIp);
