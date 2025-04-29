#include "disk.h"
#include "fat.h"
#include "stdint.h"
#include "stdio.h"

void _cdecl cstart_(uint16_t boot_drive) {
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
  FATFile far *fd = FATOpen(&disk, "/");
  FATDirectoryEntry entry;
  int i = 0;
  while (FATReadEntry(&disk, fd, &entry) && i++ < 5) {
    printf("  ");
    for (int i = 0; i < 11; i++) {
      putc(entry.name[i]);
    }
    printf("\r\n");
  }
  FATClose(fd);

  // Read test.txt file
  char buffer[100];
  uint32_t read;
  fd = FATOpen(&disk, "mydir/test.txt");
  while ((read = FATRead(&disk, fd, sizeof(buffer), buffer))) {
    for (uint32_t i = 0; i < read; i++) {
      if (buffer[i] == '\n') {
        putc('\r');
      }
      putc(buffer[i]);
    }
  }
  FATClose(fd);

end:
  for (;;)
    ;
}
