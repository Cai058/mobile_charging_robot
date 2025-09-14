#ifndef BATTERY_H
#define	BATTERY_H

#include "bsp_485_battery.h"
#include "stdlib.h"

#define BATTERY_BUFFER_SIZE 512  // ��������С
#define NeedCharge_Threshold 85 //�������ֵ����Ҫ���
#define StopCharge_Threshold 86  //�������ֵ��ֹͣ���
#define Low_Battery_Threshold 30 //���ڸ���ֵ�������˲��ɶ�

extern char *pbuf_battery;
extern uint16_t len_battery;

typedef struct {      
    float current_A;       // ������A��
    float capacity_Ah;     // ʣ��������Ah��
    uint8_t soc;           // ʣ������ٷֱȣ�0~100��
} BatteryInfo_t;

void Battery_Init(void);
void Battery_Update(void);

void Battery_t_init(void);
uint8_t battery_if_rx_lock(const uint8_t *data);

static uint16_t Modbus_CRC16(uint8_t *data, uint8_t len);
void Battery_SendRequest(uint16_t num);
void Battery_ParseFrame(const uint8_t *data, uint16_t len);

// ����ֵ
uint8_t Battery_ifNeedCharge(void);   //�Ƿ���Ҫ���
uint8_t Battery_isCharging(void);     //�Ƿ����ڳ��
uint8_t Get_SOC(void);

#endif

