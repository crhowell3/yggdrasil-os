#include "io.h"

#define UNUSED_PORT 0x08

void i686_iowait() {
    i686_outb(UNUSED_PORT, 0);
}