#ifndef __LED_H
#define	__LED_H

#include "stm32f4xx.h"

//Òý½Å¶¨Òå
/*******************************************************/
#define LED1_Pin GPIO_PIN_14
#define LED1_GPIO_Port GPIOF
#define LED2_Pin GPIO_PIN_11
#define LED2_GPIO_Port GPIOE
					




void LED_GPIO_Config(void);
void LED_RED_ON(void);
void LED_RED_OFF(void);
void LED_GREEN_ON(void);
void LED_GREEN_OFF(void);

#endif /* __LED_H */
