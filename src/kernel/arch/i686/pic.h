#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    const char* name;
    bool (*probe)();
    void (*initialize)(uint8_t offset_pic1, uint8_t offset_pic2, bool auto_eoi);
    void (*disable)();
    void (*send_end_of_interrupt)(int irq);
    void (*mask)(int irq);
    void (*unmask)(int irq);
} PICDriver;