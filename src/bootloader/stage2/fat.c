#include "fat.h"

#include "disk.h"
#include "memdefs.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "utility.h"

static const int kSectorSize = 512;
static const int kMaxPathSize = 256;
static const int kMaxFileHandles = 10;
static const int kRootDirectoryHandle = -1;

#pragma pack(push, 1)

typedef struct {
  // BIOS parameters
  uint8_t boot_jump_instruction[3];
  uint8_t oem_identifier[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t fat_count;
  uint16_t dir_entry_count;
  uint16_t total_sectors;
  uint8_t media_descriptor_type;
  uint16_t sectors_per_fat;
  uint16_t sectors_per_track;
  uint16_t heads;
  uint32_t hidden_sectors;
  uint32_t large_sector_count;

  // Extended boot record (EBR)
  uint8_t drive_number;
  uint8_t _reserved;
  uint8_t signature;
  uint32_t volume_id;
  uint8_t volume_label[11];
  uint8_t system_id[8];
} FATBootSector;

#pragma pack(pop)

typedef struct {
  FATFile public;
  bool opened;
  uint32_t first_cluster;
  uint32_t current_cluster;
  uint32_t current_sector_in_cluster;
  uint8_t buffer[kSectorSize];
} FATFileData;

typedef struct {
  union {
    FATBootSector boot_sector;
    uint8_t boot_sector_bytes[kSectorSize];
  } boot_sector_data;

  FATFileData root_directory;

  FATFileData opened_files[kMaxFileHandles];
} FATData;

static FATData far *data_;
static uint8_t far *fat_ = NULL;
static uint32_t data_section_lba_;

bool FATReadBootSector(DISK *disk) {
  return DISKReadSectors(disk, 0, 1, data_->boot_sector_data.boot_sector_bytes);
}

bool FATReadFat(DISK *disk) {
  return DISKReadSectors(
      disk, data_->boot_sector_data.boot_sector.reserved_sectors,
      data_->boot_sector_data.boot_sector.sectors_per_fat, fat_);
}

bool FATInitialize(DISK *disk) {
  data_ = (FATData far *)MEMORY_FAT_ADDR;

  // Read boot sector
  if (!FATReadBootSector(disk)) {
    printf("FAT: read boot sector failed\r\n");
    return false;
  }

  // Read file allocation table (FAT)
  fat_ = (uint8_t far *)data_ + sizeof(FATData);
  uint32_t fat_size = data_->boot_sector_data.boot_sector.bytes_per_sector *
                      data_->boot_sector_data.boot_sector.sectors_per_fat;
  if (sizeof(FATData) + fat_size >= MEMORY_FAT_SIZE) {
    printf("FAT: not enough memory to read FAT! Required %lu, only have "
           "%u\r\n",
           sizeof(FATData) + fat_size, MEMORY_FAT_SIZE);
    return false;
  }

  if (!FatReadFat(disk)) {
    printf("FAT: read FAT failed\r\n");
    return false;
  }

  // Read root directory
  uint32_t root_dir_lba = data_->boot_sector_data.boot_sector.reserved_sectors +
                          data_->boot_sector_data.boot_sector.sectors_per_fat *
                              data_->boot_sector_data.boot_sector.fat_count;
  uint32_t root_dir_size = sizeof(FATDirectoryEntry) *
                           data_->boot_sector_data.boot_sector.dir_entry_count;

  // Open root directory file
  data_->root_directory.opened = true;
  data_->root_directory.public.handle = kRootDirectoryHandle;
  data_->root_directory.public.is_directory = true;
  data_->root_directory.public.position = 0;
  data_->root_directory.public.size =
      sizeof(FATDirectoryEntry) *
      data_->boot_sector_data.boot_sector.dir_entry_count;
  data_->root_directory.first_cluster = ;
  data_->root_directory.current_cluster = 0;
  data_->root_directory.current_sector_in_cluster = 0;

  if (!DISKReadSectors(disk, root_dir_lba, 1, data_->root_directory.buffer)) {
    printf("FAT: read root directory failed\r\n");
    return false;
  }

  // Calculate data section
  uint32_t root_dir_sectors =
      (root_dir_size + data_->boot_sector_data.boot_sector.bytes_per_sector -
       1) /
      data_->boot_sector_data.boot_sector.bytes_per_sector;
  data_section_lba_ = root_dir_lba + root_dir_sectors;

  // Reset opened files
  for (int i = 0; i < kMaxFileHandles; i++) {
    data_->opened_files[i].opened = false;
  }

  return true;
}

uint32_t FATClusterToLba(uint32_t cluster) {
  return data_section_lba_ +
         (cluster - 2) *
             data_->boot_sector_data.boot_sector.sectors_per_cluster;
}

FATFile far *FATOpenEntry(DISK *disk, FATDirectoryEntry *entry) {
  // Find empty handle
  int handle = -1;
  for (int i = 0; i < kMaxFileHandles && handle < 0; i++) {
    if (!data_->opened_files[i].opened) {
      handle = i;
    }
  }

  // Out of handles
  if (handle < 0) {
    printf("FAT: out of file handles\r\n");
    return false;
  }

  // Set up vars
  FATFileData far *fd = &data_->opened_files[handle];
  fd->public.handle = handle;
  fd->public.is_directory = (entry->attributes & kFatAttributeDirectory) != 0;
  fd->public.position = 0;
  fd->public.size = 0;
  fd->first_cluster =
      entry->first_cluster_low + ((uint32_t)entry->first_cluster_high << 16);
  fd->current_cluster = fd->first_cluster;
  fd->current_sector_in_cluster = 0;

  if (!DISKReadSectors(disk, FATClusterToLba(fd->current_cluster), 1,
                       fd->buffer)) {
    printf("FAT: read error\r\n");
    return false;
  }

  fd->opened = true;
  return &fd->public;
}

uint32_t FATNextCluster(uint32_t current_cluster) {
  uint32_t fat_index = current_cluster * 3 / 2;
  if (current_cluster % 2 == 0) {
    return (*(uint16_t *)(fat_ + fat_index)) & 0x0FFF;
  } else {
    return (*(uint16_t *)(fat_ + fat_index)) >> 4;
  }
}

uint32_t FATRead(DISK *disk, FATFile far *file, uint32_t byte_count,
                 void *data_out) {
  // Get file data
  FATFileData far *fd = (file->handle == kRootDirectoryHandle)
                            ? &data_->root_directory
                            : &data_->opened_files[file->handle];

  uint8_t *u8_data_out = (uint8_t *)data_out;

  // Don't read past the end of the file
  byte_count = min(byte_count, fd->public.size - fd->public.position);

  while (byte_count > 0) {
    uint32_t left_in_buffer = kSectorSize - (fd->public.position % kSectorSize);
    uint32_t take = min(byte_count, left_in_buffer);

    memcpy(u8_data_out, fd->buffer + fd->public.position % kSectorSize, take);
    u8_data_out += take;
    fd->public.position += take;
    byte_count -= take;

    // See if we need to read more data
    if (byte_count > 0) {
      // Special handling for the root directory
      if (fd->public.handle == kRootDirectoryHandle) {
        ++fd->current_cluster;
        // Read next sector
        if (!DISKReadSectors(disk, fd->current_cluster, 1, fd->buffer)) {
          printf("FAT: Read error!\r\n");
          break;
        }
      } else {
        // Read next sector
        if (++fd->current_sector_in_cluster >=
            data_->boot_sector_data.boot_sector.sectors_per_cluster) {
          fd->current_sector_in_cluster = 0;
          fd->current_cluster = FATNextCluster(fd->current_cluster);
        }

        if (fd->current_cluster >= 0xFF8) {
          printf("FAT: Read error! Invalid next cluster!\r\n");
          break;
        }

        // Read next sector
        if (!DISKReadSectors(disk,
                             FATClusterToLba(fd->current_cluster) +
                                 fd->current_sector_in_cluster,
                             1, fd->buffer)) {
          printf("FAT: Read error!\r\n");
          break;
        }
      }
    }
  }

  return u8_data_out - (uint8_t *)data_out;
}

bool FATReadEntry(DISK *disk, FATFile far *file, FATDirectoryEntry *dir_entry) {
  return FATRead(disk, file, sizeof(FATDirectoryEntry), dir_entry) ==
         sizeof(FATDirectoryEntry);
}

void FATClose(FATFile far *file) {
  if (file->handle() == kRootDirectoryHandle) {
    file->position = 0;
    data_->root_directory.current_cluster = data_->root_directory.first_cluster;
  } else {
    data_->opened_files[file->handle].opened = false;
  }
}

bool *FATFindFile(DISK *disk, FATFile *file, const char *name,
                  FATDirectoryEntry *entry_out) {
  char fat_name[11];
  FATDirectoryEntry entry;

  // Convert from name to FAT name
  memset(fat_name, ' ', sizeof(fat_name));
  const char* ext = strchr(name, '.');
  for (int i = 0; i < )

  while(FATReadEntry(disk, file, &entry)) {}

  return NULL;
}

FATFile far *FATOpen(DISK *disk, const char *path) {
  char name[kMaxPathSize];

  // Ignore the leading slash in the filepath
  if (path[0] == '/') {
    path++;
  }

  FATFile far *current = &data_->root_directory.public;

  while (*path) {
    // Extract next file name in path
    bool is_last = false;
    const char *delim = strchr(path, '/');
    if (delim != NULL) {
      memcpy(name, path, delim - path);
      name[delim - path + 1] = '\0';
      path = delim + 1;
    } else {
      unsigned len = strlen(path);
      memcpy(name, path, len);
      name[len + 1] = '\0';
      path += len;
      is_last = true;
    }

    // Find directory entry in current directory
    FATDirectoryEntry entry;
    if (FATFindFile(current, name, &entry)) {
      FATClose(current);

      // Check if directory
      if (!is_last && entry.attributes & kFatAttributeDirectory == 0) {
        printf("FAT: %s is not a directory\r\n", name);
        return NULL;
      }

      // Open new directory entry
      current = FATOpenEntry(disk, &entry);
    } else {
      FATClose(current);

      printf("FAT: %s not found\r\n", name);
      return NULL;
    }
  }

  return current;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("Syntax: %s <disk image> <file name>\n", argv[0]);
    return -1;
  }

  FILE *disk = fopen(argv[1], "rb");
  if (!disk) {
    fprintf(stderr, "Cannot open disk image %s!\n", argv[1]);
    return -1;
  }

  if (!ReadBootSector(disk)) {
    fprintf(stderr, "Could not read boot sector!\n");
    return -2;
  }

  if (!ReadFat(disk)) {
    fprintf(stderr, "Could not read FAT!\n");
    free(fat_);
    return -3;
  }

  if (!ReadRootDirectory(disk)) {
    fprintf(stderr, "Could not read root directory!\n");
    free(fat_);
    free(root_dir_);
    return -4;
  }

  DirectoryEntry *file_entry = FindFile(argv[2]);
  if (!file_entry) {
    fprintf(stderr, "Could not find file %s!\n", argv[2]);
    free(fat_);
    free(root_dir_);
    return -5;
  }

  uint8_t *buffer =
      (uint8_t *)malloc(file_entry->size * boot_sector_.bytes_per_sector);
  if (!ReadFile(file_entry, disk, buffer)) {
    fprintf(stderr, "Could not find file %s!\n", argv[2]);
    free(fat_);
    free(root_dir_);
    free(buffer);
    return -6;
  }

  for (size_t i = 0; i < file_entry->size; i++) {
    if (isprint(buffer[i])) {
      fputc(buffer[i], stdout);
    } else {
      printf("<%02x>", buffer[i]);
    }
  }

  printf("\n");

  free(buffer);
  free(fat_);
  free(root_dir_);
  return 0;
}