#include "time.h"

TIM_HandleTypeDef htim2;
void Time_Init(void)
{
  __HAL_RCC_TIM2_CLK_ENABLE(); 
	
	htim2.Instance = TIM2;
  htim2.Init.Prescaler = 4200-1;  // 42MHz
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 10-1;  // 45M/(4500*10) = 1000Hz --> 1ms
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.RepetitionCounter = 0;
	
	 HAL_TIM_Base_Init(&htim2);  // ��ʼ����ʱ��

   HAL_TIM_Base_Start_IT(&htim2);  // ������ʱ����ʹ���ж�

    HAL_NVIC_SetPriority(TIM2_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

