#include "bsp_photogate.h"

GPIO_PinState mpg_state;

void Photogate_Config(void)
{
	//PF0
	GPIO_InitTypeDef GPIO_InitStruct;
	
	__HAL_RCC_GPIOI_CLK_ENABLE();
	
	GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
}

void Photogate_Update(void)
{
	mpg_state = HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_2);
}

uint8_t Get_PGState(void)
{
	return !mpg_state;
}
