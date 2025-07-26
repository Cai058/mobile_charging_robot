#include "bsp_485_battery.h"
#include "usart_callback.h"
#include <string.h>



UART_HandleTypeDef huart6;
DMA_HandleTypeDef hdma_usart6_tx;
DMA_HandleTypeDef hdma_usart6_rx;

uint8_t usart_buff_battery[BATTERY_RX_BUFFER_SIZE];
uint8_t usart_buff_battery_bak[BATTERY_RX_BUFFER_SIZE];
volatile uint16_t battery_rx_len = 0;  // 记录本次接收到的长度
volatile uint8_t battery_rx_complete = 0;  // 记录是否接收完成
volatile uint8_t battery_tx_complete = 1;  // 记录是否接收完成


void BSP_Battery_Config(void)
{
		 __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_USART6_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    /**USART6 GPIO Configuration    
    PG9     ------> USART6_TX
    PG14    ------> USART6_RX 
    */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    // USART6 初始化
    huart6.Instance = USART6;
    huart6.Init.BaudRate = 9600;
    huart6.Init.WordLength = UART_WORDLENGTH_8B;
    huart6.Init.StopBits = UART_STOPBITS_1;
    huart6.Init.Parity = UART_PARITY_NONE;
    huart6.Init.Mode = UART_MODE_TX_RX;
    huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart6.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart6);

    // DMA TX Init
    hdma_usart6_tx.Instance = DMA2_Stream6;
    hdma_usart6_tx.Init.Channel = DMA_CHANNEL_5;
    hdma_usart6_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart6_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart6_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart6_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart6_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart6_tx.Init.Mode = DMA_NORMAL;
    hdma_usart6_tx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_usart6_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    HAL_DMA_Init(&hdma_usart6_tx);

    // 关联DMA到USART6的TX
    __HAL_LINKDMA(&huart6, hdmatx, hdma_usart6_tx);
		
		// DMA RX Init
		hdma_usart6_rx.Instance = DMA2_Stream1;
    hdma_usart6_rx.Init.Channel = DMA_CHANNEL_5;
    hdma_usart6_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart6_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart6_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart6_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart6_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart6_rx.Init.Mode = DMA_NORMAL;
    hdma_usart6_rx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_usart6_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_usart6_rx);

    __HAL_LINKDMA(&huart6, hdmarx, hdma_usart6_rx);

    // 中断优先级配置
    HAL_NVIC_SetPriority(USART6_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART6_IRQn);
		
		HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 1, 0);  // USART6 TX 是 DMA2_Stream6
		HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);

		// 启动 DMA 接收
    HAL_UART_Receive_DMA(&huart6, usart_buff_battery, BATTERY_RX_BUFFER_SIZE);

    // 开启空闲中断
    __HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);
}

void USART6_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&huart6, UART_FLAG_IDLE)) // 检测空闲中断
    {
        __HAL_UART_CLEAR_FLAG(&huart6, UART_FLAG_IDLE);  // 清除空闲中断标志

        // 计算接收到的数据长度
        battery_rx_len = BATTERY_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart6_rx);
        battery_rx_complete = 1;  // 标记接收完成

        // 停止 DMA 传输并重新启动，以确保数据正确处理
        HAL_UART_DMAStop(&huart6);
			  memset(usart_buff_battery_bak, 0, BATTERY_RX_BUFFER_SIZE); // ?????
        memcpy(usart_buff_battery_bak, usart_buff_battery, battery_rx_len+3);
        memset(usart_buff_battery, 0, BATTERY_RX_BUFFER_SIZE);
        HAL_UART_Receive_DMA(&huart6, usart_buff_battery, BATTERY_RX_BUFFER_SIZE);
    }

    HAL_UART_IRQHandler(&huart6);
}

void DMA2_Stream6_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart6_tx);
}


/***************** 获取接收到的数据 **********************/
char *get_battery_rebuff(uint16_t *len)
{
    if (battery_rx_complete)
    {
        *len = battery_rx_len;
        battery_rx_complete = 0;  // 清除标志
        //return (char *)usart_buff_battery;
			  return (char *)usart_buff_battery_bak;
    }
    else
    {
        *len = 0;
        return NULL;
    }
}

uint8_t get_battery_rx_complete(void)
{
		return battery_rx_complete;
}
/***************** 清空接收缓冲区 **********************/
void clean_battery_rebuff(void) 
{
    //memset(usart_buff_battery, 0, BATTERY_RX_BUFFER_SIZE);
		battery_rx_len = 0;
		battery_rx_complete = 0;
}

void BATTERY_DMA_Rx_ReStart(void)    //++++++++++++++++++++++++
{
		HAL_UART_AbortReceive(&huart6);
	  huart6.RxState = HAL_UART_STATE_READY;
    HAL_UART_MspInit(&huart6);  // ??????MSP??
    HAL_UART_Receive_DMA(&huart6, usart_buff_battery, BATTERY_RX_BUFFER_SIZE);
	  hdma_usart6_rx.State = HAL_DMA_STATE_READY;
    __HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);
}

void battery_SendString(uint8_t *str, uint8_t  len)
{
		
		if(battery_tx_complete == 0)
		{
				return;
		}
    battery_tx_complete = 0;
    HAL_UART_Transmit_DMA(&huart6, str, len);
	 // m_Delay(1000);
}

void usart6_tx_cplt(void)
{
		battery_tx_complete = 1;
}
//static void m_Delay(__IO uint32_t nCount)	 //简单的延时函数
//{
//	for(; nCount != 0; nCount--);
//}
