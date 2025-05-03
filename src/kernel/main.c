#include "memory.h"
#include "stdio.h"
#include <hal/hal.h>
#include <stdint.h>

extern uint8_t __bss_start; // NOLINT
extern uint8_t __end;       // NOLINT

void __attribute__((section(".entry"))) start(uint16_t boot_drive) {
  memset(&__bss_start, 0, (&__end) - (&__bss_start));

  HAL_Initialize();

  clrscr();

  printf("Hello from the kernel!\n");

end:
  for (;;) {
  }
}