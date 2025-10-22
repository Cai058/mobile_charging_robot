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

 /**
  * @brief  初始化控制LED的IO
  * @param  无
  * @retval 无
  */
void LED_GPIO_Config(void)
{		
	  GPIO_InitTypeDef GPIO_InitStruct;
	
		__HAL_RCC_GPIOE_CLK_ENABLE();
	  __HAL_RCC_GPIOF_CLK_ENABLE();
	  __HAL_RCC_GPIOG_CLK_ENABLE();
	
	  //HAL_GPIO_WritePin(GPIOE, LED2_Pin, GPIO_PIN_RESET);
	  //HAL_GPIO_WritePin(GPIOF, LED1_Pin, GPIO_PIN_RESET);
	  
	  GPIO_InitStruct.Pin =  LED2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	
	  GPIO_InitStruct.Pin = LED1_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
}

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
