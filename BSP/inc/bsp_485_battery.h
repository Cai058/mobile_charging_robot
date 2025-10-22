/* bsp_battery.h */
#ifndef __BSP_BATTERY_H__
#define __BSP_BATTERY_H__

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define BATTERY_RX_BUFFER_SIZE 1024

void BSP_Battery_Config(void);

char *get_battery_rebuff(uint16_t *len);
void clean_battery_rebuff(void);
void battery_SendString(uint8_t *str, uint8_t len);
uint8_t get_battery_rx_complete(void);
void BATTERY_DMA_Rx_ReStart(void);

void usart6_tx_cplt(void);

static void m_Delay(__IO uint32_t nCount);

#endif

