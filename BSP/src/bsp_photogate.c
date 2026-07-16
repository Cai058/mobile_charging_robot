#include "bsp_photogate.h"

GPIO_PinState mpg_state;

void Photogate_Update(void)
{
	mpg_state = HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_2);
}

uint8_t Get_PGState(void)
{
	return !mpg_state;
}
