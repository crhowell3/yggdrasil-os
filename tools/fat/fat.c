#include <stdio.h>
#include <stdint.h>

typedef enum { false, true } bool;

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

BootSector g_BootSector;
uint8_t* g_Fat = NULL;
DirectoryEntry* g_RootDirectory = NULL;

bool ReadBootSector(FILE* disk) {
    return fread(&g_BootSector, sizeof(g_BootSector), 1, disk) > 0;
}

bool ReadSectors(FILE* disk, uint32_t lba, uint32_t count, void* buffer_out) {
    bool is_ok = true;
    is_ok = is_ok && (fseek(
        disk, lba * g_BootSector.BytesPerSector, SEEK_SET) == 0);
    is_ok = is_ok && (fread(
        buffer_out, g_BootSector.BytesPerSector, count, disk) == count);
    return is_ok;
}

bool ReadFat(FILE* disk) {
    g_Fat = (uint8_t*) malloc(g_BootSector.SectorsPerFat * g_BootSector.BytesPerSector);
    return ReadSectors(disk, g_BootSector.ReservedSectors, g_BootSector.SectorsPerFat, g_Fat);
}

bool ReadRootDirectory(FILE* disk) {
    uint32_t lba = g_BootSector.ReservedSectors + g_BootSector.SectorsPerFat * g_BootSector.FatCount;
    uint32_t size = sizeof(DirectoryEntry) * g_BootSector.DirEntryCount;
    uint32_t sectors = (size / g_BootSector.BytesPerSector);
    if (size % g_BootSector.BytesPerSector > 0) {
        sectors++;
    }
    g_RootDirectory = (DirectoryEntry*) malloc(sectors * g_BootSector.BytesPerSector);
    return ReadSectors(disk, lba, sectors, g_RootDirectory);
}

DirectoryEntry* FindFile(const char* name) {
    for (uint32_t i = 0; i < g_BootSector.DirEntryCount; i++) {
        if (memcmp(name, g_RootDirectory[i].Name, 11) == 0) {
            return &g_RootDirectory[i];
        }
    }

    return NULL;
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
        free(g_Fat);
        return -3;
    }

    if (!ReadRootDirectory(disk)) {
        fprintf(stderr, "Could not read root directory!\n");
        free(g_Fat);
        free(g_RootDirectory);
        return -4;
    }

    free(g_Fat);
    free(g_RootDirectory);
    return 0;
}