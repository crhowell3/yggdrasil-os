#include "memory.h"

void far* memcpy(void far *dst, const void far *src, uint16_t num) {
  uint8_t far *u8_dst = (uint8_t far *)dst;
  const uint8_t far *u8_src = (const uint8_t far *)src;

  for (uint16_t i = 0; i < num; i++) {
    u8_dst[i] = u8_src[i];
  }

  return dst;
}

void far *memset(void far *ptr, int value, uint16_t num) {
  uint8_t far *u8_ptr = (uint8_t far *)ptr;

  for (uint16_t i = 0; i < num; i++) {
    u8_ptr[i] = (uint8_t)value;
  }

  return ptr;
}

int memcmp(const void far *ptr1, const void far *ptr2, uint16_t num) {
  const uint8_t far *u8_ptr1 = (const uint8_t far *)ptr1;
  const uint8_t far *u8_ptr2 = (const uint8_t far *)ptr2;

  for (uint16_t i = 0; i < num; i++) {
    if (u8_ptr1[i] != u8_ptr2[i]) {
      return 1;
    }
  }
  return 0;
}
