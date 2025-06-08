#include "memory.h"
#include "stdio.h"
#include <hal/hal.h>
#include <stdint.h>

#include <arch/i686/irq.h>

extern uint8_t __bss_start; // NOLINT
extern uint8_t __end;       // NOLINT

extern void _init();

void crash_me();

void timer(Registers* regs) {
  printf(".");
}

void __attribute__((section(".entry"))) start(uint16_t boot_drive) {
  memset(&__bss_start, 0, (&__end) - (&__bss_start));

  _init();

  HAL_Initialize();

  clrscr();

  printf("Hello from the kernel!\n");

  i686_IRQ_RegisterHandler(0, timer);

end:
  for (;;) {
  }
}
