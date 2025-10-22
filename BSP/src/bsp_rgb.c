#include "bsp_rgb.h"

void RGB_Config(void)
{
		GPIO_InitTypeDef GPIO_InitStruct;
	
		__HAL_RCC_GPIOD_CLK_ENABLE();
		__HAL_RCC_GPIOH_CLK_ENABLE();
	
	  //HAL_GPIO_WritePin(GPIOE, LED2_Pin, GPIO_PIN_RESET);
	  //HAL_GPIO_WritePin(GPIOF, LED1_Pin, GPIO_PIN_RESET);
	  
	  GPIO_InitStruct.Pin =  GPIO_PIN_15 | GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_PULLDOWN;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	
		GPIO_InitStruct.Pin =  GPIO_PIN_12;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_PULLDOWN;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
}

void RGB_ON(RGB_COLOR color)
{
		switch(color){
			case YELLOW:
					HAL_GPIO_WritePin(GPIOD,GPIO_PIN_15,GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOH,GPIO_PIN_12,GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOD,GPIO_PIN_14,GPIO_PIN_RESET);
			
			break;
			
			case RED:
					HAL_GPIO_WritePin(GPIOD,GPIO_PIN_15,GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOH,GPIO_PIN_12,GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOD,GPIO_PIN_14,GPIO_PIN_RESET);
			
			break;
			
			case GREEN:
					HAL_GPIO_WritePin(GPIOD,GPIO_PIN_15,GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOH,GPIO_PIN_12,GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOD,GPIO_PIN_14,GPIO_PIN_SET);
			
			break;
			
			case CYAN:
					HAL_GPIO_WritePin(GPIOD,GPIO_PIN_15,GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOH,GPIO_PIN_12,GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOD,GPIO_PIN_14,GPIO_PIN_SET);
			break;
		}
}

void RGB_OFF(void)
{
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_12,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_13,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_14,GPIO_PIN_SET);
}
