#include "disk.h"
#include "fat.h"
#include "memdefs.h"
#include "memory.h"
#include "stdio.h"
#include <stdint.h>

uint8_t *KernelLoadBuffer = (uint8_t *)MEMORY_LOAD_KERNEL;
uint8_t *Kernel = (uint8_t *)MEMORY_KERNEL_ADDR;

typedef void (*KernelStart)();

void __attribute__((cdecl)) start(uint16_t boot_drive) {
  clrscr();

  DISK disk;
  if (!DISKInitialize(&disk, boot_drive)) {
    printf("Disk init error\r\n");
    goto end;
  }

  if (!FATInitialize(&disk)) {
    printf("FAT init error\r\n");
    goto end;
  }

  // Browse files in root
  FATFile *fd = FATOpen(&disk, "/kernel.bin");
  uint32_t read;
  uint8_t *kernel_buffer = Kernel;
  while ((read = FATRead(&disk, fd, MEMORY_LOAD_SIZE, KernelLoadBuffer))) {
    memcpy(kernel_buffer, KernelLoadBuffer, read);
    kernel_buffer += read;
  }

  FATClose(fd);

  // Execute the kernel
  KernelStart kernel_start = (KernelStart)Kernel;
  kernel_start();

end:
  for (;;)
    ;
}
