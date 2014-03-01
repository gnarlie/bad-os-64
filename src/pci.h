#pragma once

#include "common.h"

typedef void (*pci_init_fn)(uint8_t, uint32_t);

void init_pci();
void register_pci_device(uint16_t vendor, uint16_t device, pci_init_fn);
