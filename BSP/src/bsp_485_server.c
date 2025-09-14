#include "bsp_485_server.h"
#include "main.h"
#include "usart_callback.h"
#include <stdarg.h>

//static void Delay(__IO uint32_t nCount); 

UART_HandleTypeDef huart7;
DMA_HandleTypeDef hdma_uart7_rx;
DMA_HandleTypeDef hdma_uart7_tx;

//中断缓存串口数据
#define UART_BUFF_SIZE      1024
volatile    uint16_t uart_p_server = 1;
uint8_t     uart_buff_server[UART_BUFF_SIZE];
volatile uint16_t uart_rx_len = 0;  // 记录本次接收到的长度
volatile uint8_t server_rx_complete = 0;  // 记录是否接收完成
volatile uint8_t server_tx_complete = 1;  // 记录是否发送完成 


void UART7_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  
  __GPIOE_CLK_ENABLE();
  //__GPIOH_CLK_ENABLE();
	
  /* 使能 UART 时钟 */
  __HAL_RCC_UART7_CLK_ENABLE();
	
	__HAL_RCC_DMA1_CLK_ENABLE();  // 启用DMA1时钟
	
	/*配置电源输出12V*/
//	GPIO_InitStruct.Pin = GPIO_PIN_2;  // 假设设置 PH2 引脚
//	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;  // 设置为输入模式
//	GPIO_InitStruct.Pull = GPIO_PULLUP;     // 设置为上拉
//	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//	HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /**USART7 GPIO Configuration    
  PE8    ------> USART7_TX
  PE7    ------> USART7_RX 
  */
  /* 配置Tx引脚为复用功能  */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF8_UART7;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
  
  /* 配置Rx引脚为复用功能 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Alternate = GPIO_AF8_UART7;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct); 
  
  /* 配置串485_USART 模式 */
  huart7.Instance = UART7;
  huart7.Init.BaudRate = 9600;
  huart7.Init.WordLength = UART_WORDLENGTH_8B;
  huart7.Init.StopBits = UART_STOPBITS_1;
  huart7.Init.Parity = UART_PARITY_NONE;
  huart7.Init.Mode = UART_MODE_TX_RX;
  huart7.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart7.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&huart7);

	/* uart7 DMA Init */
    /* uart7_RX Init */
    hdma_uart7_rx.Instance = DMA1_Stream3;
    hdma_uart7_rx.Init.Channel = DMA_CHANNEL_5;
    hdma_uart7_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_uart7_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_uart7_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_uart7_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_uart7_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_uart7_rx.Init.Mode = DMA_NORMAL;
    hdma_uart7_rx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_uart7_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_uart7_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(&huart7,hdmarx,hdma_uart7_rx);

    /* uart7_TX Init */
    hdma_uart7_tx.Instance = DMA1_Stream1;
    hdma_uart7_tx.Init.Channel = DMA_CHANNEL_5;
    hdma_uart7_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_uart7_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_uart7_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_uart7_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_uart7_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_uart7_tx.Init.Mode = DMA_NORMAL;
    hdma_uart7_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_uart7_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_uart7_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(&huart7,hdmatx,hdma_uart7_tx);
  /*串口1中断初始化 */
  //HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_0);
	HAL_NVIC_SetPriority(UART7_IRQn, 0 ,0);
	HAL_NVIC_EnableIRQ(UART7_IRQn);
		
	HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 1, 0);  // USART6 TX 是 DMA2_Stream6
	HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
		
	HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 1, 0);  // USART6 TX 是 DMA2_Stream6
	HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
  /*配置串口接收中断 */
  //__HAL_UART_ENABLE_IT(&huart7,UART_IT_RXNE);

	/* 启动 DMA 接收 */
  HAL_UART_Receive_DMA(&huart7, uart_buff_server, UART_BUFF_SIZE);

	/* 使能空闲中断（IDLE 中断） */
	__HAL_UART_ENABLE_IT(&huart7, UART_IT_IDLE);
}


/*****************  使用DMA发送数据 **********************/

void Server_SendString(uint8_t *str,uint16_t len)
{  

		if(server_tx_complete == 0)
		{
			 return;
		}
		server_tx_complete = 0;
    HAL_UART_Transmit_DMA(&huart7, str, len);
	  //Delay(1000);
}

void uart7_tx_cplt(void)
{
		server_tx_complete = 1;
}

uint8_t if_server_complete(void)
{
	return server_tx_complete;   // 0: not complete  1: complete
}
/***************** 获取接收到的数据 **********************/
char *get_server_rebuff(uint16_t *len)
{
    if (server_rx_complete)
    {
        *len = uart_rx_len;
        server_rx_complete = 0;  // 清除标志
        return (char *)uart_buff_server;
    }
    else
    {
        *len = 0;
        return NULL;
    }
}

/***************** 清空接收缓冲区 **********************/
void clean_server_rebuff(void) 
{
    memset(uart_buff_server, 0, UART_BUFF_SIZE);
		uart_rx_len = 0;
		server_rx_complete = 0;
}

/***************** 串口中断处理函数（包含空闲中断） **********************/
void UART7_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&huart7, UART_FLAG_IDLE)) // 检测空闲中断
    {
        __HAL_UART_CLEAR_FLAG(&huart7, UART_FLAG_IDLE);  // 清除空闲中断标志

        // 计算接收到的数据长度
        uart_rx_len = UART_BUFF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_uart7_rx);
        server_rx_complete = 1;  // 标记接收完成

        // 停止 DMA 传输并重新启动，以确保数据正确处理
        HAL_UART_DMAStop(&huart7);
        HAL_UART_Receive_DMA(&huart7, uart_buff_server, UART_BUFF_SIZE);
    }

    HAL_UART_IRQHandler(&huart7);
}

void DMA1_Stream1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_uart7_tx); // ???? TX
}

void DMA1_Stream3_IRQHandler(void)
{
		HAL_DMA_IRQHandler(&hdma_uart7_rx);
}
//static void Delay(__IO uint32_t nCount)	 //简单的延时函数
//{
//	for(; nCount != 0; nCount--);
//}
