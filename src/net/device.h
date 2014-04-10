#pragma once

#include "common.h"

typedef char mac[6];

struct TaskT;

struct netdevice {
    void (*send)(struct netdevice * self, const void*, uint16_t);
    struct TaskT * sendTask;
    uint32_t ip;
    mac mac;
    uint16_t iomem;
}; 
