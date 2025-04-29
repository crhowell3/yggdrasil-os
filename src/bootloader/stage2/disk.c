#include "disk.h"
#include "stdio.h"
#include "x86.h"

bool DISKInitialize(DISK *disk, uint8_t drive_number) {
  uint8_t drive_type;
  uint16_t cylinders, sectors, heads;

  if (!x86_Disk_GetDriveParams(disk->id, &drive_type, &cylinders, &sectors,
                               &heads)) {
    return false;
  }

  disk->id = drive_number;
  disk->cylinders = cylinders;
  disk->sectors = sectors;
  disk->heads = heads;

  return true;
}

// NOLINTNEXTLINE
void DISK_LBA2CHS(DISK *disk, uint32_t lba, uint16_t *cylinders_out,
                  uint16_t *sectors_out, uint16_t *heads_out) {
  // Sector = (LBA % sectors per track + 1)
  *sectors_out = lba % disk->sectors + 1;

  // Cylinder = (LBA / sectors per track) / heads
  *cylinders_out = (lba / disk->sectors) / disk->heads;

  // Head = (LBA / sectors per track) % heads
  *heads_out = (lba / disk->sectors) % disk->heads;
}

bool DISKReadSectors(DISK *disk, uint32_t lba, uint8_t sectors,
                     void *data_out) {
  uint16_t cylinder;
  uint16_t sector;
  uint16_t head;

  DISK_LBA2CHS(disk, lba, &cylinder, &sector, &head);

  for (int i = 0; i < 3; i++) {
    if (x86_Disk_Read(disk->id, cylinder, sector, head, sectors, data_out)) {
      return true;
    }

    x86_Disk_Reset(disk->id);
  }

  return false;
}
