#ifndef BSP_ULTRAWAVE_H
#define BSP_ULTRAWAVE_H

#include "stm32f4xx.h"



void Ultrawave_Config(void);
void bsp_ultrawave_off(void);
void bsp_ultrawave_on(void);
void Ultrawave_Update(void);
uint16_t Ultrawave_IfStop(void);


#endif

