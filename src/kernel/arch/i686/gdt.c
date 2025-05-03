#include "gdt.h"
#include <stdint.h>

typedef struct {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_middle;
  uint8_t access;
  uint8_t flags_limit_hi;
  uint8_t base_high;
} __attribute__((packed)) GDTEntry;

typedef struct {
  uint16_t limit;
  GDTEntry *ptr;
} __attribute__((packed)) GDTDescriptor;

typedef enum {
  kCodeReadable = 0x02,
  kDataWriteable = 0x02,

  kAccessCodeConforming = 0x04,
  kDataDirectionNormal = 0x00,
  kDataDirectionDown = 0x04,

  kDataSegment = 0x10,
  kCodeSegment = 0x18,

  kDescriptorTss = 0x00,

  kRing0 = 0x00,
  kRing1 = 0x20,
  kRing2 = 0x40,
  kRing3 = 0x60,

  kPresent = 0x80,
} GDT_ACCESS;

typedef enum {
  k64Bit = 0x20,
  k32Bit = 0x40,
  k16Bit = 0x00,

  kGranularity1B = 0x00,
  kGranularity4K = 0x80,
} GDT_FLAGS;

// Helper macros
#define GDT_LIMIT_LOW(limit) ((limit) & 0xFFFF)
#define GDT_BASE_LOW(base) ((base) & 0xFFFF)
#define GDT_BASE_MIDDLE(base) (((base) >> 16) & 0xFF)
#define GDT_FLAGS_LIMIT_HI(limit, flags)                                       \
  ((((limit) >> 16) & 0xF) | ((flags) & 0xF0))
#define GDT_BASE_HIGH(base) (((base) >> 24) & 0xFF)

#define GDT_ENTRY(base, limit, access, flags)                                  \
  {GDT_LIMIT_LOW(limit),                                                       \
   GDT_BASE_LOW(base),                                                         \
   GDT_BASE_MIDDLE(base),                                                      \
   access,                                                                     \
   GDT_FLAGS_LIMIT_HI(limit, flags),                                           \
   GDT_BASE_HIGH(base)}

GDTEntry gdt_[] = {
    // NULL descriptor
    GDT_ENTRY(0, 0, 0, 0),

    // Kernel 32-bit code segment
    GDT_ENTRY(0, 0xFFFFF, kPresent | kRing0 | kCodeSegment | kCodeReadable,
              k32Bit | kGranularity4K),

    // Kernel 32-bit data segment
    GDT_ENTRY(0, 0xFFFFF, kPresent | kRing0 | kDataSegment | kDataWriteable,
              k32Bit | kGranularity4K),
};

GDTDescriptor gdt_descriptor_ = {
    sizeof(gdt_) - 1,
    gdt_,
};

// NOLINTNEXTLINE
void __attribute__((cdecl)) i686_GDT_Load(GDTDescriptor *descriptor,
                                          uint16_t code_segment,
                                          uint16_t data_segment);

// NOLINTNEXTLINE
void i686_GDT_Initialize() {
  i686_GDT_Load(&gdt_descriptor_, i686_GDT_CODE_SEGMENT, i686_GDT_DATA_SEGMENT);
}