#ifndef ROBOT_CONFIG_H
#define ROBOT_CONFIG_H

#include <stdint.h>
#include <string.h>

#define slow_speed 4000  
#define fast_speed 5000  
#define working_speed 5000  

#define BATTERY_BUFFER_SIZE 512  // 缓冲区大小
#define NeedCharge_Threshold 62 //到达该阈值，需要充电
#define StopCharge_Threshold 95  //到达该阈值，停止充电
#define Low_Battery_Threshold 30 //低于该阈值，机器人不可动


// ===========================================010===========================================
// #define Robot_ID "010"
// #define Total_Points 18
// #define Direction_Point 18
// #define Charge_ID 18
// #define Rear_LimitSwitch 1
// #define Move_Direction -1
// #define Pushrod_Mode 1
// #define turn_num1 1
// #define turn_num2 9
// #define turn_num3 10
// #define turn_num4 18


// ===========================================011===========================================
#define Robot_ID "011"
#define Total_Points 3
#define Direction_Point 18
#define Charge_ID 2
#define Rear_LimitSwitch 1
#define Move_Direction 1
#define Pushrod_Mode 0
#define turn_num1 0
#define turn_num2 0
#define turn_num3 0
#define turn_num4 0


// ===========================================014===========================================
// #define Robot_ID "014"
// #define Total_Points 18
// #define Direction_Point 18
// #define Charge_ID 1
// #define Rear_LimitSwitch 0
// #define Move_Direction -1
// #define Pushrod_Mode 1
// #define turn_num1 1
// #define turn_num2 9
// #define turn_num3 10
// #define turn_num4 18

#endif
