#include "irq.h"
#include "i8259.h"
#include "io.h"
#include "pic.h"
#include <stddef.h>
#include <stdio.h>
#include <util/arrays.h>

#define PIC_REMAP_OFFSET 0x20

IRQHandler irq_handlers_[16];
static const PICDriver *driver_ = NULL;

void i686_IRQ_Handler(Registers *regs) {
  int irq = regs->interrupt - PIC_REMAP_OFFSET;

  if (irq_handlers_[irq] != NULL) {
    irq_handlers_[irq](regs);
  } else {
    printf("Unhandled IRQ %d...\n", irq);
  }

  // Send EOI
  driver_->send_end_of_interrupt(irq);
}

void i686_IRQ_Initialize() {
  const PICDriver* drivers[] = {
      i8259_GetDriver(),
  };

  for (int i = 0; i < SIZE(drivers); i++) {
    if (drivers[i]->probe()) {
      driver_ = drivers[i];
    }
  }

  if (driver_ == NULL) {
    printf("Warning: No PIC found!\n");
    return;
  }

  printf("Found %s PIC.\n", driver_->name);
  driver_->initialize(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8, false);

  for (int i = 0; i < 16; i++) {
    i686_ISR_RegisterHandler(PIC_REMAP_OFFSET + i, i686_IRQ_Handler);
  }

  i686_EnableInterrupts();
}

void i686_IRQ_RegisterHandler(int irq, IRQHandler handler) {
  irq_handlers_[irq] = handler;
}