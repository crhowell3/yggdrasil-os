#pragma once
#include "disk.h"
#include <stdint.h>

#pragma pack(push, 1)

typedef struct {
  uint8_t name[11];
  uint8_t attributes;
  uint8_t _reserved;
  uint8_t created_time_tenths;
  uint16_t created_time;
  uint16_t created_date;
  uint16_t accessed_date;
  uint16_t first_cluster_high;
  uint16_t modified_time;
  uint16_t modified_date;
  uint16_t first_cluster_low;
  uint32_t size;
} FATDirectoryEntry;

#pragma pack(pop)

typedef struct {
  int handle;
  bool is_directory;
  uint32_t position;
  uint32_t size;
} FATFile;

enum FATAttributes {
  kFatAttributeReadOnly = 0x01,
  kFatAttributeHidden = 0x02,
  kFatAttributeSystem = 0x04,
  kFatAttributeVolumeId = 0x08,
  kFatAttributeDirectory = 0x10,
  kFatAttributeArchive = 0x20,
  kFatAttributeLFN = kFatAttributeReadOnly | kFatAttributeHidden |
                     kFatAttributeSystem | kFatAttributeVolumeId |
                     kFatAttributeDirectory | kFatAttributeArchive
};

bool FATInitialize(DISK *disk);
FATFile *FATOpen(DISK *disk, const char *path);
uint32_t FATRead(DISK *disk, FATFile *file, uint32_t byte_count,
                 void *data_out);
bool FATReadEntry(DISK *disk, FATFile *file, FATDirectoryEntry *dir_entry);
void FATClose(FATFile *file);
