#include "bsp_485_rfid.h"
#include "usart_callback.h"
#include <string.h>

UART_HandleTypeDef huart8;
DMA_HandleTypeDef hdma_uart8_tx;
DMA_HandleTypeDef hdma_uart8_rx;

#define RFID_RX_BUFFER_SIZE      256

uint8_t usart_buff_rfid[RFID_RX_BUFFER_SIZE];
uint8_t usart_buff_rfid_bak[RFID_RX_BUFFER_SIZE];  //++++++++++++++++++++++++
volatile uint16_t rfid_rx_len = 0;
volatile uint8_t rfid_rx_complete = 0;  
volatile uint8_t rfid_tx_complete = 1;


void BSP_RFID_Config(void)
{
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_UART8_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    /** UART8 GPIO Configuration
        PE1 -> UART8_TX
        PE0 -> UART8_RX
    */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART8;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    huart8.Instance = UART8;
    huart8.Init.BaudRate = 38400;
    huart8.Init.WordLength = UART_WORDLENGTH_8B;
    huart8.Init.StopBits = UART_STOPBITS_1;
    huart8.Init.Parity = UART_PARITY_NONE;
    huart8.Init.Mode = UART_MODE_TX_RX;
    huart8.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart8.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart8);

    // DMA TX - UART8 -> DMA1_Stream0 / Channel5
    hdma_uart8_tx.Instance = DMA1_Stream0;
    hdma_uart8_tx.Init.Channel = DMA_CHANNEL_5;
    hdma_uart8_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_uart8_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_uart8_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_uart8_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_uart8_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_uart8_tx.Init.Mode = DMA_NORMAL;
    hdma_uart8_tx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_uart8_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_uart8_tx);
    __HAL_LINKDMA(&huart8, hdmatx, hdma_uart8_tx);

    // DMA RX - UART8 -> DMA1_Stream6 / Channel5
    hdma_uart8_rx.Instance = DMA1_Stream6;
    hdma_uart8_rx.Init.Channel = DMA_CHANNEL_5;
    hdma_uart8_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_uart8_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_uart8_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_uart8_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_uart8_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_uart8_rx.Init.Mode = DMA_NORMAL;
    hdma_uart8_rx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_uart8_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_uart8_rx);
    __HAL_LINKDMA(&huart8, hdmarx, hdma_uart8_rx);

    // 中断配置
    HAL_NVIC_SetPriority(UART8_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(UART8_IRQn);

    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

    // 启动 DMA 接收
    HAL_UART_Receive_DMA(&huart8, usart_buff_rfid, RFID_RX_BUFFER_SIZE);

    // 开启空闲中断
    __HAL_UART_ENABLE_IT(&huart8, UART_IT_IDLE);
}

void UART8_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&huart8, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_FLAG(&huart8, UART_FLAG_IDLE);
        rfid_rx_len = RFID_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_uart8_rx);
        rfid_rx_complete = 1;
        HAL_UART_DMAStop(&huart8);
			  memset(usart_buff_rfid_bak, 0, RFID_RX_BUFFER_SIZE); // ?????
        memcpy(usart_buff_rfid_bak, usart_buff_rfid, rfid_rx_len+3);
        memset(usart_buff_rfid, 0, RFID_RX_BUFFER_SIZE);
        HAL_UART_Receive_DMA(&huart8, usart_buff_rfid, RFID_RX_BUFFER_SIZE);
    }

    HAL_UART_IRQHandler(&huart8);
}

void DMA1_Stream0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_uart8_tx); 
}

char *get_rfid_rebuff(uint16_t *len)
{
    if (rfid_rx_complete)
    {
        *len = rfid_rx_len;
        rfid_rx_complete = 0;
        //return (char *)usart_buff_rfid; 
			   return (char *)usart_buff_rfid_bak;   //++++++++++++++++++++++++++++++++++++
    }
    else
    {
        *len = 0;
        return NULL;
    }
}

uint8_t get_rfid_rx_complete(void)   //++++++++++++++++++++++++++++
{
		return rfid_rx_complete;
}

void clean_rfid_rebuff(void)
{
    //memset(usart_buff_rfid, 0, RFID_RX_BUFFER_SIZE);  //++++++++++++++++++++++++++++++
    rfid_rx_len = 0;
    rfid_rx_complete = 0;
}

void RFID_SendStr(uint8_t *str, uint32_t strlen)
{
		if(rfid_tx_complete == 0)
		{
				return;
		}
		
		rfid_tx_complete = 0;
    HAL_UART_Transmit_DMA(&huart8, str, strlen);
    //m_Delay(1000);
}

void RFID_DMA_Rx_ReStart(void)    //++++++++++++++++++++++++
{
		HAL_UART_AbortReceive(&huart8);
	  huart8.RxState = HAL_UART_STATE_READY;
    HAL_UART_MspInit(&huart8);  // ??????MSP??
    HAL_UART_Receive_DMA(&huart8, usart_buff_rfid, RFID_RX_BUFFER_SIZE);
	   hdma_uart8_rx.State = HAL_DMA_STATE_READY;
    __HAL_UART_ENABLE_IT(&huart8, UART_IT_IDLE);
}
void uart8_tx_cplt(void)
{
		rfid_tx_complete = 1;
}




