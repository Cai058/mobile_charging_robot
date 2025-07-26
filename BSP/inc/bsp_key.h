#ifndef __KEY_H
#define	__KEY_H

#include "stm32f4xx.h"
#include "main.h"
//Òý½Å¶¨Òå

#define KEY1_PIN                  GPIO_PIN_2                 
#define KEY1_GPIO_PORT            GPIOB                      
#define KEY1_GPIO_CLK_ENABLE()    __GPIOB_CLK_ENABLE()


#define KEY_ON	1
#define KEY_OFF	0

void Key_GPIO_Config(void);
uint8_t Key_Scan(GPIO_TypeDef* GPIOx,uint16_t GPIO_Pin);
uint8_t Get_action(void);

#endif /* __LED_H */

