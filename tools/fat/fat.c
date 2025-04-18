#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum { false, true } bool;  // NOLINT

typedef struct {
    // BIOS parameters
    uint8_t BootJumpInstruction[3];
    uint8_t OemIdentifier[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FatCount;
    uint16_t DirEntryCount;
    uint16_t TotalSectors;
    uint8_t MediaDescriptorType;
    uint16_t SectorsPerFat;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t LargeSectorCount;

    // Extended boot record (EBR)
    uint8_t DriveNumber;
    uint8_t _Reserved;
    uint8_t Signature;
    uint32_t VolumeId;
    uint8_t VolumeLabel[11];
    uint8_t SystemId[8];
} __attribute__((packed)) BootSector;

typedef struct {
    uint8_t Name[11];
    uint8_t Attributes;
    uint8_t _Reserved;
    uint8_t CreatedTimeTenths;
    uint16_t CreatedTime;
    uint16_t CreatedDate;
    uint16_t AccessedDate;
    uint16_t FirstClusterHigh;
    uint16_t ModifiedTime;
    uint16_t ModifiedDate;
    uint16_t FirstClusterLow;
    uint32_t Size;
} __attribute__((packed)) DirectoryEntry;

BootSector boot_sector_;
uint8_t* fat_ = NULL;
DirectoryEntry* root_dir_ = NULL;
uint32_t roo_directory_end_;

bool ReadBootSector(FILE* disk) {
    return fread(&boot_sector_, sizeof(boot_sector_), 1, disk) > 0;
}

bool ReadSectors(FILE* disk, uint32_t lba, uint32_t count, void* buffer_out) {
    bool is_ok = true;
    is_ok = is_ok && (fseek(
        disk, lba * boot_sector_.BytesPerSector, SEEK_SET) == 0);
    is_ok = is_ok && (fread(
        buffer_out, boot_sector_.BytesPerSector, count, disk) == count);
    return is_ok;
}

bool ReadFat(FILE* disk) {
    fat_ = (uint8_t*) malloc(boot_sector_.SectorsPerFat * boot_sector_.BytesPerSector);
    return ReadSectors(disk, boot_sector_.ReservedSectors, boot_sector_.SectorsPerFat, fat_);
}

bool ReadRootDirectory(FILE* disk) {
    uint32_t lba = boot_sector_.ReservedSectors + boot_sector_.SectorsPerFat * boot_sector_.FatCount;
    uint32_t size = sizeof(DirectoryEntry) * boot_sector_.DirEntryCount;
    uint32_t sectors = (size / boot_sector_.BytesPerSector);
    if (size % boot_sector_.BytesPerSector > 0) {
        sectors++;
    }
    roo_directory_end_ = lba + sectors;
    root_dir_ = (DirectoryEntry*) malloc(sectors * boot_sector_.BytesPerSector);
    return ReadSectors(disk, lba, sectors, root_dir_);
}

DirectoryEntry* FindFile(const char* name) {
    for (uint32_t i = 0; i < boot_sector_.DirEntryCount; i++) {
        if (memcmp(name, root_dir_[i].Name, 11) == 0) {
            return &root_dir_[i];
        }
    }

    return NULL;
}

bool ReadFile(DirectoryEntry* file_entry, FILE* disk, uint8_t* output_buffer) {
    bool ok = true;
    uint16_t current_cluster = file_entry->FirstClusterLow;

    do {
        uint32_t lba = roo_directory_end_ + (current_cluster - 2) * boot_sector_.SectorsPerCluster;
        ok = ok && ReadSectors(disk, lba, boot_sector_.SectorsPerCluster, output_buffer);
        output_buffer += boot_sector_.SectorsPerCluster * boot_sector_.BytesPerSector;

        uint32_t fat_index = current_cluster * 3 / 2;
        if (current_cluster % 2 == 0) {
            current_cluster = (*(uint16_t*)(fat_ + fat_index)) & 0x0FFF;
        } else {
            current_cluster = (*(uint16_t*)(fat_ + fat_index)) >> 4;
        }
    } while (ok && current_cluster < 0xFF8);

    return ok;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Syntax: %s <disk image> <file name>\n", argv[0]);
        return -1;
    }

    FILE* disk = fopen(argv[1], "rb");
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

    DirectoryEntry* file_entry = FindFile(argv[2]);
    if (!file_entry) {
        fprintf(stderr, "Could not find file %s!\n", argv[2]);
        free(fat_);
        free(root_dir_);
        return -5;
    }

    uint8_t* buffer = (uint8_t*) malloc(file_entry->Size * boot_sector_.BytesPerSector);
    if (!ReadFile(file_entry, disk, buffer)) {
        fprintf(stderr, "Could not find file %s!\n", argv[2]);
        free(fat_);
        free(root_dir_);
        free(buffer);
        return -6;
    }

    for (size_t i = 0; i < file_entry->Size; i++) {
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