#ifndef __DEBUG_USART_H
#define	__DEBUG_USART_H

#include "stm32f4xx.h"
#include <stdio.h>

//串口波特率
#define DEBUG_USART_BAUDRATE                    115200

//引脚定义
/*******************************************************/
//#define DEBUG_USART                             USART6
//#define DEBUG_USART_CLK_ENABLE()                __HAL_RCC_USART6_CLK_ENABLE();

////#define RCC_PERIPHCLK_UARTx                     RCC_PERIPHCLK_USART1
////#define RCC_UARTxCLKSOURCE_SYSCLK               RCC_USART1CLKSOURCE_SYSCLK

//#define DEBUG_USART_RX_GPIO_PORT                GPIOG
//#define DEBUG_USART_RX_GPIO_CLK_ENABLE()        __GPIOG_CLK_ENABLE()
//#define DEBUG_USART_RX_PIN                      GPIO_PIN_9
//#define DEBUG_USART_RX_AF                       GPIO_AF8_USART6

//#define DEBUG_USART_TX_GPIO_PORT                GPIOG
//#define DEBUG_USART_TX_GPIO_CLK_ENABLE()        __GPIOG_CLK_ENABLE()
//#define DEBUG_USART_TX_PIN                      GPIO_PIN_14
//#define DEBUG_USART_TX_AF                       GPIO_AF8_USART6

//#define DEBUG_USART_IRQHandler                  USART6_IRQHandler
//#define DEBUG_USART_IRQ                 		    USART6_IRQn




//#define DEBUG_USART                             UART7
//#define DEBUG_USART_CLK_ENABLE()                __HAL_RCC_UART7_CLK_ENABLE();

////#define RCC_PERIPHCLK_UARTx                     RCC_PERIPHCLK_USART1
////#define RCC_UARTxCLKSOURCE_SYSCLK               RCC_USART1CLKSOURCE_SYSCLK

//#define DEBUG_USART_RX_GPIO_PORT                GPIOE
//#define DEBUG_USART_RX_GPIO_CLK_ENABLE()        __GPIOE_CLK_ENABLE()
//#define DEBUG_USART_RX_PIN                      GPIO_PIN_7
//#define DEBUG_USART_RX_AF                       GPIO_AF8_UART7

//#define DEBUG_USART_TX_GPIO_PORT                GPIOE
//#define DEBUG_USART_TX_GPIO_CLK_ENABLE()        __GPIOE_CLK_ENABLE()
//#define DEBUG_USART_TX_PIN                      GPIO_PIN_8
//#define DEBUG_USART_TX_AF                       GPIO_AF8_UART7

//#define DEBUG_USART_IRQHandler                  UART7_IRQHandler
//#define DEBUG_USART_IRQ                 		    UART7_IRQn




#define DEBUG_USART                             UART8
#define DEBUG_USART_CLK_ENABLE()                __HAL_RCC_UART8_CLK_ENABLE();

//#define RCC_PERIPHCLK_UARTx                     RCC_PERIPHCLK_USART1
//#define RCC_UARTxCLKSOURCE_SYSCLK               RCC_USART1CLKSOURCE_SYSCLK

#define DEBUG_USART_RX_GPIO_PORT                GPIOE
#define DEBUG_USART_RX_GPIO_CLK_ENABLE()        __GPIOE_CLK_ENABLE()
#define DEBUG_USART_RX_PIN                      GPIO_PIN_0
#define DEBUG_USART_RX_AF                       GPIO_AF8_UART8

#define DEBUG_USART_TX_GPIO_PORT                GPIOE
#define DEBUG_USART_TX_GPIO_CLK_ENABLE()        __GPIOE_CLK_ENABLE()
#define DEBUG_USART_TX_PIN                      GPIO_PIN_1
#define DEBUG_USART_TX_AF                       GPIO_AF8_UART8

#define DEBUG_USART_IRQHandler                  UART8_IRQHandler
#define DEBUG_USART_IRQ                 		    UART8_IRQn
/************************************************************/

void Usart_SendString(uint8_t *str);
void DEBUG_USART_Config(void);
//int fputc(int ch, FILE *f);
extern UART_HandleTypeDef UartHandle;
#endif /* __USART1_H */
