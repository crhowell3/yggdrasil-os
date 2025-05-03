#pragma once
#include <stdint.h>

void __attribute__((cdecl)) i686_outb(uint16_t port, uint8_t value); // NOLINT
uint8_t __attribute__((cdecl)) i686_inb(uint16_t port);              // NOLINT

void __attribute__((cdecl)) i686_Panic();