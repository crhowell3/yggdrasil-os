#include "fat.h"

#include "disk.h"
#include "memdefs.h"
#include "stdint.h"
#include "stdio.h"
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

bool FATReadBootSector(DISK *disk)
{
    return DISKReadSectors(disk, 0, 1,
                           data_->boot_sector_data.boot_sector_bytes);
}

bool FATReadFat(DISK *disk)
{
    return DISKReadSectors(
        disk, data_->boot_sector_data.boot_sector.reserved_sectors,
        data_->boot_sector_data.boot_sector.sectors_per_fat, fat_);
}

bool FATReadRootDirectory(DISK *disk)
{
    uint32_t lba = data_->boot_sector_data.boot_sector.reserved_sectors +
                   data_->boot_sector_data.boot_sector.sectors_per_fat *
                       data_->boot_sector_data.boot_sector.fat_count;
    uint32_t size = sizeof(FATDirectoryEntry) *
                    data_->boot_sector_data.boot_sector.dir_entry_count;
    uint32_t sectors =
        (size + data_->boot_sector_data.boot_sector.bytes_per_sector - 1) /
        data_->boot_sector_data.boot_sector.bytes_per_sector;

    root_directory_end_ = lba + sectors;
    return DISKReadSectors(disk, lba, sectors, root_dir_);
}

bool FATInitialize(DISK *disk)
{
    data_ = (FATData far *) MEMORY_FAT_ADDR;

    // Read boot sector
    if (!FATReadBootSector(disk)) {
        printf("FAT: read boot sector failed\r\n");
        return false;
    }

    // Read file allocation table (FAT)
    fat_ = (uint8_t far *) data_ + sizeof(FATData);
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
    uint32_t root_dir_lba =
        data_->boot_sector_data.boot_sector.reserved_sectors +
        data_->boot_sector_data.boot_sector.sectors_per_fat *
            data_->boot_sector_data.boot_sector.fat_count;
    uint32_t root_dir_size =
        sizeof(FATDirectoryEntry) *
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

    // Reset opened files
    for (int i = 0; i < kMaxFileHandles; i++) {
        data_->opened_files[i].opened = false;
    }

    return true;
}

FATFile *FATOpen(DISK *disk, const char *path)
{
    // Ignore the leading slash in the filepath
    if (path[0] == '/') {
        path++;
    }
}

FATDirectoryEntry *FindFile(const char *name)
{
    for (uint32_t i = 0;
         i < data_->boot_sector_data->boot_sector.dir_entry_count; i++) {
        if (memcmp(name, root_dir_[i].name, 11) == 0) {
            return &root_dir_[i];
        }
    }

    return NULL;
}

bool ReadFile(FATDirectoryEntry *file_entry, DISK *disk, uint8_t *output_buffer)
{
    bool ok = true;
    uint16_t current_cluster = file_entry->first_cluster_low;

    do {
        uint32_t lba = root_directory_end_ +
                       (current_cluster - 2) * boot_sector_.sectors_per_cluster;
        ok = ok && ReadSectors(disk, lba, boot_sector_.sectors_per_cluster,
                               output_buffer);
        output_buffer +=
            boot_sector_.sectors_per_cluster * boot_sector_.bytes_per_sector;

        uint32_t fat_index = current_cluster * 3 / 2;
        if (current_cluster % 2 == 0) {
            current_cluster = (*(uint16_t *) (fat_ + fat_index)) & 0x0FFF;
        } else {
            current_cluster = (*(uint16_t *) (fat_ + fat_index)) >> 4;
        }
    } while (ok && current_cluster < 0xFF8);

    return ok;
}

int main(int argc, char **argv)
{
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
        (uint8_t *) malloc(file_entry->size * boot_sector_.bytes_per_sector);
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