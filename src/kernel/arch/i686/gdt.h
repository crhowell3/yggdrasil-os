#pragma once

#define i686_GDT_CODE_SEGMENT 0x08 // NOLINT
#define i686_GDT_DATA_SEGMENT 0x10 // NOLINT

void i686_GDT_Initialize(); // NOLINT