#include "irq.h"
#include "io.h"
#include "pic.h"
#include <stddef.h>
#include <stdio.h>

#define PIC_REMAP_OFFSET 0x20

IRQHandler irq_handlers_[16];

void i686_IRQ_Handler(Registers *regs) {
  int irq = regs->interrupt - PIC_REMAP_OFFSET;

  uint8_t pic_isr = i686_PIC_ReadInServiceRegister();
  uint8_t pic_irr = i686_PIC_ReadIrqRequestRegister();

  if (irq_handlers_[irq] != NULL) {
    irq_handlers_[irq](regs);
  } else {
    printf("Unhandled IRQ %d  ISR=%x  IRR=%x...\n", irq, pic_isr, pic_irr);
  }

  i686_PIC_SendEndOfInterrupt(irq);
}

void i686_IRQ_Initialize() {
  i686_PIC_Configure(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8);

  for (int i = 0; i < 16; i++) {
    i686_ISR_RegisterHandler(PIC_REMAP_OFFSET + i, i686_IRQ_Handler);
  }

  i686_EnableInterrupts();
}

void i686_IRQ_RegisterHandler(int irq, IRQHandler handler) {
  irq_handlers_[irq] = handler;
}