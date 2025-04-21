#include "disk.h"
#include "x86.h"

bool DISK_Initialize(DISK* disk, uint8_t drive_number) {
    uint8_t drive_type;
    uint16_t cylinders, sectors, heads;

    
    if (!x86_Disk_GetDriveParams(disk->id, &drive_type, &cylinders, &sectors, &heads)) {
        return false;
    }

    disk->id = drive_number;
    disk->cylinders = cylinders;
    disk->sectors = sectors;
    disk->heads = heads;

    return true;
}

bool DISK_ReadSectors(DISK* disk, uint32_t lba, uint8_t sectors, uint8_t far* data_out) {

}