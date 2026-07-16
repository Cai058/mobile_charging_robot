#include "bsp_485_rfid.h"
#include "usart.h"
#include "usart_callback.h"
#include <string.h>

#define RFID_RX_BUFFER_SIZE      256

uint8_t usart_buff_rfid[RFID_RX_BUFFER_SIZE];
uint8_t usart_buff_rfid_bak[RFID_RX_BUFFER_SIZE];  //++++++++++++++++++++++++
volatile uint16_t rfid_rx_len = 0;
volatile uint8_t rfid_rx_complete = 0;  
volatile uint8_t rfid_tx_complete = 1;


void BSP_RFID_Config(void)
{
    /* UART8、GPIO、DMA 和 NVIC 由 CubeMX 统一初始化。 */
    HAL_UART_Receive_DMA(&huart8, usart_buff_rfid, RFID_RX_BUFFER_SIZE);
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
    HAL_UART_Receive_DMA(&huart8, usart_buff_rfid, RFID_RX_BUFFER_SIZE);
	   hdma_uart8_rx.State = HAL_DMA_STATE_READY;
    __HAL_UART_ENABLE_IT(&huart8, UART_IT_IDLE);
}
void uart8_tx_cplt(void)
{
		rfid_tx_complete = 1;
}




