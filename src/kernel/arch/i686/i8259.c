#include "pic.h"
#include "io.h"
#include <stdbool.h>

// clang-tidy off
#define PIC1_COMMAND_PORT 0x20 // NOLINT
#define PIC1_DATA_PORT 0x21    // NOLINT
#define PIC2_COMMAND_PORT 0xA0 // NOLINT
#define PIC2_DATA_PORT 0xA1    // NOLINT
// clang-tidy on

// Initialization Control Word 1
// -----------------------------
//  0   IC4     if set, the PIC expects to receive ICW4 during initialization
//  1   SNGL    if set, only 1 PIC in the system; if unset, the PIC is cascaded
//              with slave PICs and ICW3 must be sent to the controller
//  2   ADI     ignored on x86, set to 0
//  3   LTIM    if set, operate in level triggered mode; if unset, operate in
//              edge triggered mode
//  4   INIT    set to 1 to initialize PIC
//  5-7         ignored on x86, set to 0

enum {
  PIC_ICW1_ICW4 = 0x01,
  PIC_ICW1_SINGLE = 0x02,
  PIC_ICW1_INTERVAL4 = 0x04,
  PIC_ICW1_LEVEL = 0x08,
  PIC_ICW1_INITIALIZE = 0x10,
} PIC_ICW1;

// Initialization Control Word 4
// -----------------------------
//  0   uPM     if set, PIC is in 8086 mode; if cleared, in MCS-80/85 mode
//  1   AEOI    if set, on last interrupt acknowledge pulse, controller
//              automatically performs end of interrupt operation
//  2   M/S     only use if BUF is set; if set, selects buffer master;
//              otherwise, selects buffer slave
//  3   BUF     if set, controller operates in buffered mode
//  4   SFNM    specially fully nested mode; used in systems with large number
//              of cascaded controllers
//  5-7         reserved, set to 0

enum {
  PIC_ICW4_8086 = 0x1,
  PIC_ICW4_AUTO_EOI = 0x2,
  PIC_ICW4_BUFFER_MASTER = 0x4,
  PIC_ICW4_BUFFER_SLAVE = 0x0,
  PIC_ICW4_BUFFERED = 0x8,
  PIC_ICW4_SFNM = 0x10,
} PIC_ICW4;

enum {
  PIC_CMD_END_OF_INTERRUPT = 0x20,
  PIC_CMD_READ_IRR = 0x0A,
  PIC_CMD_READ_ISR = 0x0B,
} PIC_CMD;

static uint16_t pic_mask_ = 0xFFFF;
static bool auto_eoi_ = false;

void i8259_SetMask(uint16_t new_mask) {
  pic_mask_ = new_mask;
  i686_outb(PIC1_DATA_PORT, pic_mask_ & 0xFF);
  i686_iowait();
  i686_outb(PIC2_DATA_PORT, pic_mask_ >> 8);
  i686_iowait();
}

uint16_t i8259_GetMask() {
  return i686_inb(PIC1_DATA_PORT) | (i686_inb(PIC2_DATA_PORT) << 8);
}

void i8259_Configure(uint8_t offset_pic1, uint8_t offset_pic2, bool auto_eoi) {
  // Mask everything
  i8259_SetMask(0xFFFF);

  // Initialization Control Word 1
  i686_outb(PIC1_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
  i686_iowait();
  i686_outb(PIC2_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
  i686_iowait();

  // Initialization Control Word 2
  i686_outb(PIC1_DATA_PORT, offset_pic1);
  i686_iowait();
  i686_outb(PIC2_DATA_PORT, offset_pic2);
  i686_iowait();

  // Initialization Control Word 3
  i686_outb(PIC1_DATA_PORT, 0x4);
  i686_iowait();
  i686_outb(PIC2_DATA_PORT, 0x2);
  i686_iowait();

  // Initialization Control Word 4
  uint8_t icw4 = PIC_ICW4_8086;
  if (auto_eoi) {
    icw4 |= PIC_ICW4_AUTO_EOI;
  }

  // Clear data registers
  i686_outb(PIC1_DATA_PORT, icw4);
  i686_iowait();
  i686_outb(PIC2_DATA_PORT, icw4);
  i686_iowait();

  // Mask all interrupts until they are enabled by the device driver
  i8259_SetMask(0xFFFF);
}

void i8259_SendEndOfInterrupt(int irq) {
  if (irq >= 8) {
    i686_outb(PIC2_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
  }
  i686_outb(PIC1_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
}

void i8259_Disable() {
  i686_outb(PIC1_DATA_PORT, 0xFF);
  i686_iowait();
  i686_outb(PIC2_DATA_PORT, 0xFF);
  i686_iowait();

  i8259_SetMask(0xFFFF);
}

void i8259_Mask(int irq) {
  i8259_SetMask(pic_mask_ | (1 << irq));
}

void i8259_Unmask(int irq) {
  i8259_SetMask(pic_mask_ & ~(1 << irq));
}

uint16_t i8259_ReadIrqRequestRegister() {
  i686_outb(PIC1_COMMAND_PORT, PIC_CMD_READ_IRR);
  i686_outb(PIC2_COMMAND_PORT, PIC_CMD_READ_IRR);
  return ((uint16_t)i686_inb(PIC2_COMMAND_PORT)) |
         (((uint16_t)i686_inb(PIC2_COMMAND_PORT)) << 8);
}

uint16_t i8259_ReadInServiceRegister() {
  i686_outb(PIC1_COMMAND_PORT, PIC_CMD_READ_ISR);
  i686_outb(PIC2_COMMAND_PORT, PIC_CMD_READ_ISR);
  return ((uint16_t)i686_inb(PIC2_COMMAND_PORT)) |
         (((uint16_t)i686_inb(PIC2_COMMAND_PORT)) << 8);
}

bool i8259_Probe() {
  i8259_Disable();
  i8259_SetMask(0x1337);
  return i8259_GetMask() == 0x1337;
}

static const PICDriver pic_driver_ = {
  .name = "8259 PIC",
  .probe = &i8259_Probe,
  .initialize = &i8259_Configure,
  .disable  = &i8259_Disable,
  .send_end_of_interrupt = &i8259_SendEndOfInterrupt,
  .mask = &i8259_Mask,
  .unmask = &i8259_Unmask,
};

const PICDriver* i8259_GetDriver() {
  return &pic_driver_;
}