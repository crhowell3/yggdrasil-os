#include "idt.h"
#include <stdint.h>
#include <util/binary.h>

typedef struct {
  uint16_t base_low;
  uint16_t segment_selector;
  uint8_t reserved;
  uint8_t flags;
  uint16_t base_high;
} __attribute__((packed)) IDTEntry;

typedef struct {
  uint16_t limit;
  IDTEntry *ptr;
} __attribute__((packed)) IDTDescriptor;

IDTEntry idt_[256];

IDTDescriptor idt_descriptor_ = {sizeof(idt_) - 1, idt_};

void __attribute__((cdecl)) i686_IDT_Load(IDTDescriptor *idt_descriptor);

void i686_IDT_SetGate(int interrupt, void *base, uint16_t segment_descriptor,
                      uint8_t flags) {
  idt_[interrupt].base_low = ((uint32_t)base) & 0xFFFF;
  idt_[interrupt].segment_selector = segment_descriptor;
  idt_[interrupt].reserved = 0;
  idt_[interrupt].flags = flags;
  idt_[interrupt].base_high = ((uint32_t)base >> 16) & 0xFFFF;
}

void i686_IDT_EnableGate(int interrupt) {
  FLAG_SET(idt_[interrupt].flags, IDT_FLAG_PRESENT);
}

void i686_IDT_DisableGate(int interrupt) {
  FLAG_UNSET(idt_[interrupt].flags, IDT_FLAG_PRESENT);
}

void i686_IDT_Initialize() { i686_IDT_Load(&idt_descriptor_); }