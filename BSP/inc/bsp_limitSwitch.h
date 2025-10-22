#ifndef __limitSwitch_H
#define __limitSwitch_H
#include "stm32f4xx.h"
#include <stdbool.h>
#include "Robot_Config.h"

#define LIMIT_SWITCH1_PIN            		GPIO_PIN_14              //switch1
#define LIMIT_SWITCH1_GPIO_PORT      		GPIOB                      
#define LIMIT_SWITCH1_GPIO_CLK_ENABLE()	    __GPIOB_CLK_ENABLE()

#define LIMIT_SWITCH2_PIN            		GPIO_PIN_15             //switch2
#define LIMIT_SWITCH2_GPIO_PORT      		GPIOB                      
#define LIMIT_SWITCH2_GPIO_CLK_ENABLE()	__GPIOB_CLK_ENABLE()

void Limit_Switch_Config(void);
uint8_t limit_Switch_Scan(GPIO_TypeDef * GPIOx,uint16_t GPIO_Pin);
bool bsp_limitswitch_front_get(void);
uint8_t Get_rearlimitState(void);
uint8_t Get_frontlimitState(void);

#endif
