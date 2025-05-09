#pragma once

#include <stdint.h>

void i686_PIC_Configure(uint8_t offset_pic1, uint8_t offset_pic2);
void i686_PIC_SendEndOfInterrupt(int irq);
void i686_PIC_Disable();
void i686_PIC_Mask(int irq);
void i686_PIC_Unmask(int irq);
uint16_t i686_PIC_ReadIrqRequestRegister();
uint16_t i686_PIC_ReadInServiceRegister();