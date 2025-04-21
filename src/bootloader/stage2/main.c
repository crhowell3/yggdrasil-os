#include "stdint.h"
#include "stdio.h"

void _cdecl cstart_(uint16_t boot_drive) {
    puts("Hello world!");
    for(;;);
}