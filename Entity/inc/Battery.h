#ifndef BATTERY_H
#define	BATTERY_H

#include "bsp_485_battery.h"
#include "stdlib.h"

#define BATTERY_BUFFER_SIZE 512  // 缓冲区大小
#define NeedCharge_Threshold 85 //到达该阈值，需要充电
#define StopCharge_Threshold 86  //到达该阈值，停止充电
#define Low_Battery_Threshold 30 //低于该阈值，机器人不可动

extern char *pbuf_battery;
extern uint16_t len_battery;

typedef struct {      
    float current_A;       // 电流（A）
    float capacity_Ah;     // 剩余容量（Ah）
    uint8_t soc;           // 剩余电量百分比（0~100）
} BatteryInfo_t;

void Battery_Init(void);
void Battery_Update(void);

void Battery_t_init(void);
uint8_t battery_if_rx_lock(const uint8_t *data);

static uint16_t Modbus_CRC16(uint8_t *data, uint8_t len);
void Battery_SendRequest(uint16_t num);
void Battery_ParseFrame(const uint8_t *data, uint16_t len);

// 返回值
uint8_t Battery_ifNeedCharge(void);   //是否需要充电
uint8_t Battery_isCharging(void);     //是否正在充电
uint8_t Get_SOC(void);

#endif

