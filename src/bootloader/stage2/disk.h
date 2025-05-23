#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint8_t id;
  uint16_t cylinders;
  uint16_t sectors;
  uint16_t heads;
} DISK;

bool DISKInitialize(DISK *disk, uint8_t drive_number);
bool DISKReadSectors(DISK *disk, uint32_t lba, uint8_t sectors, void *data_out);
