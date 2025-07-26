#include "bsp_ultrawave.h"

TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim5;

uint16_t m_distance1; // Placeholder for distance value
uint16_t m_duration1; // Placeholder for duration value
uint16_t is_high1;        // Flag to track current high state
uint16_t counter1;    // Counter to record high level duration

uint16_t m_distance2; // Placeholder for distance value
uint16_t m_duration2; // Placeholder for duration value
uint16_t is_high2;        // Flag to track current high state
uint16_t counter2;    // Counter to record high level duration


void Ultrawave_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    TIM_OC_InitTypeDef sConfigOC;

   // 使能 GPIO 时钟
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    // 使能 TIM4 和 TIM5 时钟
    __HAL_RCC_TIM4_CLK_ENABLE();
    __HAL_RCC_TIM5_CLK_ENABLE();
	
	 // 配置 PD13 为 Trig 输出（TIM4_CH2）
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

     // 配置 PD12 为 Echo 输入（TIM4_CH1）
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // 配置 PH10 为 Trig 输出（TIM5_CH1）
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM5;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

    // 配置 PH11 为 Echo 输入（TIM5_CH2）
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

    // 配置定时器 TIM4，APB2 90MHz
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 84 - 1;          // APB2 时钟为 84 MHz，分频器为 84-1 使得定时器的时钟为 1 MHz  84
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 1000 - 1;           // 自动重载寄存器的值，定时器的周期为 1 ms (1 MHz 时钟)       1000
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.RepetitionCounter = 0;
    HAL_TIM_Base_Init(&htim4);

    // 配置定时器输出比较模式（PWM1模式）用于触发 Trig （TIM4_CH2）
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 500;                  // CCR1 寄存器的值，用于调整 Trig 信号的占空比
    sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_Init(&htim4);
    HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_2);
		
		// 配置定时器 TIM5，APB2 90MHz
    htim5.Instance = TIM5;
    htim5.Init.Prescaler = 84 - 1;          // APB2 时钟为 84 MHz，分频器为 84-1 使得定时器的时钟为 1 MHz
    htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5.Init.Period = 1000 - 1;           // 自动重载寄存器的值，定时器的周期为 1 ms (1 MHz 时钟)
    htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim5.Init.RepetitionCounter = 0;
    HAL_TIM_Base_Init(&htim5);

    // 配置定时器输出比较模式（PWM1模式）用于触发 Trig （TIM5_CH1）
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 500;                  // CCR1 寄存器的值，用于调整 Trig 信号的占空比
    sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_Init(&htim5);
    HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_1);

    // 启动定时器
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);  // 启动 Trig 引脚的 PWM 输出
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1);  // 启动 Trig 引脚的 PWM 输出
}

void bsp_ultrawave_on(void)
{
    // 启动定时器 PWM 输出（Trig）
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);  // 启动 Trig 引脚的 PWM 输出
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1);  // 启动 Trig 引脚的 PWM 输出
}

void bsp_ultrawave_off(void)
{
    // 停止定时器 PWM 输出（Trig）
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2);  // 启动 Trig 引脚的 PWM 输出
    HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_1);  // 启动 Trig 引脚的 PWM 输出
}

void Ultrawave_Update(void)
{
	   
	  // 读取前面的传感器的距离
    // 读取 GPIOH 11 的状态
    GPIO_PinState pin_state1 = HAL_GPIO_ReadPin(GPIOH, GPIO_PIN_11);

    if (pin_state1 == GPIO_PIN_SET) // 高电平
    {
        if (!is_high1)
        {
            // 检测到上升沿，开始计数
            is_high1 = 1;
            counter1 = 0; // 重置计数器
        }
        counter1++; // 在高电平期间，增加计数器
    }
    else if (pin_state1 == GPIO_PIN_RESET && is_high1) // 低电平且之前是高电平
    {
        // 检测到下降沿，停止计数
        is_high1 = 0;
        m_duration1 = counter1; // 记录高电平持续的时长
    }
		// 根据声速计算距离，单位为毫米
    m_distance1 = m_duration1 * 340 / 2;
		
		
		
		
		// 读取后面的传感器的距离
    // 读取 GPIOD 12 的状态
    GPIO_PinState pin_state2 = HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_12);

    if (pin_state2 == GPIO_PIN_SET) // 高电平
    {
        if (!is_high2)
        {
            // 检测到上升沿，开始计数
            is_high2 = 1;
            counter2 = 0; // 重置计数器
        }
        counter2++; // 在高电平期间，增加计数器
    }
    else if (pin_state2 == GPIO_PIN_RESET && is_high2) // 低电平且之前是高电平
    {
        // 检测到下降沿，停止计数
        is_high2 = 0;
        m_duration2 = counter2; // 记录高电平持续的时长
    }


    // 根据声速计算距离，单位为毫米
    m_distance2 = m_duration2 * 340 / 2;
		
		
}

// 获取计算的距离
uint16_t Ultrawave_IfStop(void)
{
    if (m_distance1 < 340 || m_distance2 < 340) // 判断任一传感器的距离是否小于340
    {
        return 1;
    }
    return 0; // 如果两个都大于340，则返回0
}

