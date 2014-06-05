#pragma once

#include "common.h"
#include "net/device.h"

void init_arp();

void gratuitous_arp(struct netdevice* device);

void arp_packet(struct netdevice* dev, const uint8_t* data);
void arp_store(mac dest, uint32_t ip);

int arp_lookup(struct netdevice * dev, uint32_t ip, mac dest);
