#pragma once
#include <stdbool.h>
#include <stdint.h>

void __attribute__((cdecl)) x86_outb(uint16_t port, uint8_t value);
uint8_t __attribute__((cdecl)) x86_inb(uint16_t port);

bool __attribute__((cdecl)) x86_Disk_GetDriveParams(uint8_t drive,
                                                    uint8_t *drive_type_out,
                                                    uint16_t *cylinders_out,
                                                    uint16_t *sectors_out,
                                                    uint16_t *heads_out);

bool __attribute__((cdecl)) x86_Disk_Reset(uint8_t drive);

bool __attribute__((cdecl)) x86_Disk_Read(uint8_t drive, uint16_t cylinder,
                                          uint16_t sector, uint16_t head,
                                          uint8_t count, void *lower_data_out);
