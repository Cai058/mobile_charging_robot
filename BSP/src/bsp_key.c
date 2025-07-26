#include "bsp_key.h" 

uint8_t m_action;
uint8_t cnt = 0;

void Key_GPIO_Config(void)

{
/*PB2*/
    GPIO_InitTypeDef GPIO_InitStructure;
    __HAL_RCC_GPIOB_CLK_ENABLE();
	
    GPIO_InitStructure.Pin = KEY1_PIN; 
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT; 
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(KEY1_GPIO_PORT, &GPIO_InitStructure);

}



uint8_t Key_Scan(GPIO_TypeDef* GPIOx,uint16_t GPIO_Pin)

{			
	if(HAL_GPIO_ReadPin(GPIOx,GPIO_Pin) == KEY_ON )  
	{	 
		while(HAL_GPIO_ReadPin(GPIOx,GPIO_Pin) == KEY_ON);   
		return 	KEY_ON;	 
	}
	else
		return KEY_OFF;
}


uint8_t Get_action(void)
{
if(	Key_Scan(KEY1_GPIO_PORT,KEY1_PIN) == KEY_ON)
		{
			cnt++;
		}
		
		if(cnt%2 == 1)
		{
			m_action = 1;
		}
		else if(cnt%2 == 0)
		{
			m_action = 0;
		}
 return m_action;
}
/*********************************************END OF FILE**********************/

