/**
  ******************************************************************************
  * @file    bsp_led.c
  * @author  fire
  * @version V1.0
  * @date    2017-xx-xx
  * @brief   led应用函数接口
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火 STM32 F407 开发板
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :http://firestm32.taobao.com
  *
  ******************************************************************************
  */
  
#include "bsp_led.h"   

void LED_RED_ON(void)
{
  HAL_GPIO_WritePin(GPIOE, LED2_Pin, GPIO_PIN_RESET);
}

void LED_RED_OFF(void)
{
  HAL_GPIO_WritePin(GPIOE, LED2_Pin, GPIO_PIN_SET);
}

void LED_GREEN_ON(void)
{
  HAL_GPIO_WritePin(GPIOF, LED1_Pin, GPIO_PIN_RESET);
}

void LED_GREEN_OFF(void)
{
  HAL_GPIO_WritePin(GPIOF, LED1_Pin, GPIO_PIN_SET);
}
/*********************************************END OF FILE**********************/
