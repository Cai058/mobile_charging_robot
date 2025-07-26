#include "bsp_rc.h"
#include <string.h>

#define SBUS_RX_BUF_NUM 36u
#define RC_FRAME_LENGTH 18u

//static RC_raw_t rc_raw;
static uint8_t SBUS_buff_rc[SBUS_RX_BUF_NUM];

volatile uint16_t rc_rx_len = 0;  // 记录本次接收到的长度
volatile uint8_t rc_rx_complete = 0;  // 记录是否接收完成


UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;


//RC_raw_t *get_remote_control_raw(void)
//{
//	return &rc_raw;
//}

void bsp_rc_Config(void)
{
		/* ---------------- GPIO and UART Init ------------------- */
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_USART1_CLK_ENABLE();
	__HAL_RCC_DMA2_CLK_ENABLE();

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	huart1.Instance = USART1;
	huart1.Init.BaudRate = 100000;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_EVEN;
	huart1.Init.Mode = UART_MODE_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	HAL_UART_DeInit(&huart1);
	HAL_UART_Init(&huart1);

	/* ---------------- DMA Init ----------------------------- */
	hdma_usart1_rx.Instance = DMA2_Stream2;
	hdma_usart1_rx.Init.Channel = DMA_CHANNEL_4;
	hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
	hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
	hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	hdma_usart1_rx.Init.Mode = DMA_CIRCULAR;
	hdma_usart1_rx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
	hdma_usart1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

	HAL_DMA_DeInit(&hdma_usart1_rx);
	HAL_DMA_Init(&hdma_usart1_rx);

	__HAL_LINKDMA(&huart1, hdmarx, hdma_usart1_rx);
	
	HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(USART1_IRQn);

	/* ---------------- IDLE中断打开 ------------------------ */
	__HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);

	HAL_UART_Receive_DMA(&huart1, SBUS_buff_rc, SBUS_RX_BUF_NUM);
}


void bsp_rc_disable(void)
{
	HAL_UART_DMAStop(&huart1);
	__HAL_UART_DISABLE(&huart1);
}

void bsp_rc_enable(void)
{
	HAL_UART_Receive_DMA(&huart1, SBUS_buff_rc, SBUS_RX_BUF_NUM);
	__HAL_UART_ENABLE(&huart1);
}

void bsp_rc_restart(uint16_t dma_buf_num)
{
	bsp_rc_disable();
	HAL_UART_Receive_DMA(&huart1, SBUS_buff_rc, dma_buf_num);
	bsp_rc_enable();
}


/* USART1 IRQ Handler */
void USART1_IRQHandler(void)
{
	if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE))
	{
		__HAL_UART_CLEAR_IDLEFLAG(&huart1);
		
		rc_rx_len = SBUS_RX_BUF_NUM - __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);
		if (rc_rx_len == RC_FRAME_LENGTH)
		{
				rc_rx_complete = 1;
		}

//		if (received_len == RC_FRAME_LENGTH)
//		{
//			SBUS_TO_RC(SBUS_rx_buf, &rc_raw);
//		}

		// 重新启动DMA接收
		HAL_UART_DMAStop(&huart1);
		HAL_UART_Receive_DMA(&huart1, SBUS_buff_rc, SBUS_RX_BUF_NUM);
	}
}

/***************** 获取接收到的数据 **********************/
char *get_rc_rebuff(uint16_t *len)
{
    if (rc_rx_complete)
    {
        *len = rc_rx_len;
        rc_rx_complete = 0;  // 清除标志
        return (char *)SBUS_buff_rc;
    }
    else
    {
        *len = 0;
        return NULL;
    }
}

/***************** 清空接收缓冲区 **********************/
void clean_rc_rebuff(void) 
{
    memset(SBUS_buff_rc, 0, SBUS_RX_BUF_NUM);
		rc_rx_len = 0;
		rc_rx_complete = 0;
}

//static void SBUS_TO_RC(volatile const uint8_t *sbus_buf, RC_raw_t *rc_raw)
//{
//	if (sbus_buf == NULL || rc_raw == NULL)
//		return;

//	rc_raw->rc.ch[0] = (sbus_buf[0] | (sbus_buf[1] << 8)) & 0x07ff;
//	rc_raw->rc.ch[1] = ((sbus_buf[1] >> 3) | (sbus_buf[2] << 5)) & 0x07ff;
//	rc_raw->rc.ch[2] = ((sbus_buf[2] >> 6) | (sbus_buf[3] << 2) |
//						(sbus_buf[4] << 10)) & 0x07ff;
//	rc_raw->rc.ch[3] = ((sbus_buf[4] >> 1) | (sbus_buf[5] << 7)) & 0x07ff;
//	rc_raw->rc.s[0] = ((sbus_buf[5] >> 4) & 0x0003);
//	rc_raw->rc.s[1] = ((sbus_buf[5] >> 4) & 0x000C) >> 2;
//	rc_raw->mouse.x = sbus_buf[6] | (sbus_buf[7] << 8);
//	rc_raw->mouse.y = sbus_buf[8] | (sbus_buf[9] << 8);
//	rc_raw->mouse.z = sbus_buf[10] | (sbus_buf[11] << 8);
//	rc_raw->mouse.press_l = sbus_buf[12];
//	rc_raw->mouse.press_r = sbus_buf[13];
//	rc_raw->key.v = sbus_buf[14] | (sbus_buf[15] << 8);
//	rc_raw->rc.ch[4] = ((sbus_buf[16]) | (sbus_buf[17] << 8)) & 0x07ff;

//	for (int i = 0; i < 5; ++i)
//		rc_raw->rc.ch[i] -= RC_CH_VALUE_OFFSET;
//}
