#pragma once
#include <stdint.h>

void __attribute__((cdecl)) i686_outb(uint16_t port, uint8_t value); // NOLINT
uint8_t __attribute__((cdecl)) i686_inb(uint16_t port);              // NOLINT
uint8_t __attribute__((cdecl)) i686_EnableInterrupts();
uint8_t __attribute__((cdecl)) i686_DisableInterrupts();

void __attribute__((cdecl)) i686_Panic();

void i686_iowait();