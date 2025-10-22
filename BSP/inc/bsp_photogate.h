#ifndef PHOTOGATE_H
#define PHOTOGATE_H

#include "stm32f4xx.h"

void Photogate_Config(void);
void Photogate_Update(void);
uint8_t  Get_PGState(void);

#endif
