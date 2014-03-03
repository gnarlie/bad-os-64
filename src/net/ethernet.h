#pragma once

#include "common.h"

struct netdevice;

void ethernet_packet(struct netdevice * device, const uint8_t *packet);
