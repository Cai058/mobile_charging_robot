#include "bsp_limitSwitch.h"

// RobotConfig_t RobotConfig_ls;

void Limit_Switch_Config(void)
{
  //初始化IO
  //Front ------> PI6
	//Rear  ------> PI7
	__GPIOI_CLK_ENABLE();
	
	GPIO_InitTypeDef GPIO_InitStructure;
			
	GPIO_InitStructure.Pin = GPIO_PIN_6;
	GPIO_InitStructure.Mode =GPIO_MODE_INPUT;
	GPIO_InitStructure.Pull =GPIO_PULLDOWN;	
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH; 	
	//GPIO_InitStructure.Alternate = ADVANCE_BKIN_AF;	
	HAL_GPIO_Init(GPIOI, &GPIO_InitStructure);
	
	GPIO_InitStructure.Pin = GPIO_PIN_7;	
	//GPIO_InitStructure.Alternate = ADVANCE_BKIN_AF;	
	HAL_GPIO_Init(GPIOI, &GPIO_InitStructure);
}

uint8_t Get_frontlimitState(void)  // NO: 按下1, 松开0
{
	/* 检测是否有按键按下 */
if (HAL_GPIO_ReadPin(GPIOI,GPIO_PIN_6) == 1 ) {
return 1;
}
return 0;
}


uint8_t Get_rearlimitState(void)  //
{
	/* 检测是否有按键按下 */
if (HAL_GPIO_ReadPin(GPIOI,GPIO_PIN_7) == Rear_LimitSwitch ) {
return 1;
}

return 0;
}

//好像没用
uint8_t limit_Switch_Scan(GPIO_TypeDef * GPIOx,uint16_t GPIO_Pin)
{
/* 检测是否有按键按下 */
if (HAL_GPIO_ReadPin(GPIOx,GPIO_Pin) == 1 ) {
/* 等待按键释放 */
while (HAL_GPIO_ReadPin(GPIOx,GPIO_Pin) == 1);
return 1;
} else
return 0;
}
