// uart_callback.c
#include "usart_callback.h"
#include "bsp_485_battery.h"
#include "bsp_485_rfid.h"
#include "bsp_485_server.h"

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART7)
        uart7_tx_cplt();
    else if (huart->Instance == UART8)
        uart8_tx_cplt();
		else if (huart->Instance == USART6)
        usart6_tx_cplt();
}
