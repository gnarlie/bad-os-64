#pragma once

#include "common.h"

typedef char mac[6];

struct TaskT;
struct sbuff_t;

struct netdevice {
    void (*send)(struct netdevice * self, struct sbuff_t * sbuff);
    uint32_t ip;
    mac mac;
    uint16_t iomem;
};
