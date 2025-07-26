#ifndef RFID_H
#define	RFID_H

#include "bsp_485_rfid.h"
#include "usart_callback.h"

extern char *pbuf_rfid;
extern uint16_t rfid_len;

void RFID_Update(void);
void RFID_Config(void);
uint8_t Get_Number(void);
void RFID_SendCommand(void);

uint8_t rfid_if_rx_lock(const uint8_t *data);

uint16_t ExtractRFIDNumber(const uint8_t *data, uint16_t len);
#endif
