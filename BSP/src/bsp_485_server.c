#include "bsp_485_server.h"
#include "usart.h"
#include "usart_callback.h"
#include <stdarg.h>

//static void Delay(__IO uint32_t nCount);

//中断缓存串口数据
#define UART_BUFF_SIZE      1024
volatile uint16_t uart_p_server = 1;
uint8_t uart_buff_server[UART_BUFF_SIZE];
volatile uint16_t uart_rx_len = 0;  // 记录本次接收到的长度
volatile uint8_t server_rx_complete = 0;  // 记录是否接收完成
volatile uint8_t server_tx_complete = 1;  // 记录是否发送完成

void UART7_Config(void)
{
    /* UART7、GPIO、DMA 和 NVIC 由 CubeMX 统一初始化。 */
    HAL_UART_Receive_DMA(&huart7, uart_buff_server, UART_BUFF_SIZE);
    __HAL_UART_ENABLE_IT(&huart7, UART_IT_IDLE);
}

/*****************  使用DMA发送数据 **********************/
void Server_SendString(uint8_t *str, uint16_t len)
{
    if (server_tx_complete == 0)
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
    if (__HAL_UART_GET_FLAG(&huart7, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_FLAG(&huart7, UART_FLAG_IDLE);

        uart_rx_len = UART_BUFF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_uart7_rx);
        server_rx_complete = 1;

        HAL_UART_DMAStop(&huart7);
        HAL_UART_Receive_DMA(&huart7, uart_buff_server, UART_BUFF_SIZE);
    }

    HAL_UART_IRQHandler(&huart7);
}

void DMA1_Stream1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_uart7_tx);
}

void DMA1_Stream3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_uart7_rx);
}

//static void Delay(__IO uint32_t nCount)
//{
//    for(; nCount != 0; nCount--);
//}
