#ifndef CONTROLLER_DUMMY_H
#define	CONTROLLER_DUMMY_H

#include "bsp_ultrawave.h"
#include "bsp_photogate.h"
#include "bsp_led.h" 
#include "bsp_can.h"
#include "bsp_key.h"
#include "pid.h"
#include "bsp_debug_usart.h"
#include "bsp_limitSwitch.h"
#include "bsp_L298N.h"
#include "RFID.h"
#include "Server.h"
#include "bsp_rgb.h"
#include "bsp_rc.h"
#include "Battery.h"
#include "RC.h"
#include "first_order_filter.h"

#define TOTAL_POINTS 18
#define charge_id 2
#define turn_num1 1
#define turn_num2 9
#define turn_num3 10
#define turn_num4 18
#define adjust_speed 50   //调整时的速度
#define slow_speed_2nd 2000  //第二阶段慢速
#define slow_speed 4000  //慢速的速度
#define fast_speed 5000  //快速的速度
#define working_speed 7000  //工作时的速度

typedef struct{
uint16_t ultra_stop;
uint8_t  pg_state;
uint8_t action;
uint8_t number;
uint8_t front_state;
uint8_t rear_state;
uint8_t location_id;
uint8_t previous_id;
uint8_t if_finished;
uint8_t need_charge;
uint8_t is_charging;
uint8_t charge_mode;
int move_direction;
uint8_t current_target_id; //当前目标（充电点位或者从平台获取的）
uint8_t available;
uint8_t place_complete;
uint8_t m_soc;
//ServerMsg_t server_msg;
}Sensor_t;

typedef enum {
	STATE_IDLE,       
	STATE_ERROR,
	STATE_PUSH,
	STATE_PULL,
  STATE_RUNNING,
	STATE_ADJUST,
	STATE_WORKING,
	STATE_CHARGING
} State;


typedef struct {
	State lastState;
	State currentState;
} StateMachine;


void Sensor_t_Update(void);
void Dummy_Update(void);
void SwitchState(void);

//初始化结构体
void Dummy_Init(void);
void Sensor_t_Init(void);
void StateMachine_Init(void);

//切换回自动状态时，重置变量
void Dummy_Reset(void);
void Sensor_t_Reset(void);

//返回值
void Get_MotorSpeed(int16_t *_speed);

//设置电机速度
void SetxSpeed(int _speed);
void SetySpeed(int _speed);

//计算方向和前一个点位位置
int get_direction(uint8_t  location_id, uint8_t  target_id);
uint8_t  get_previous_point(uint8_t  target_id, int  direction);

//进入空闲状态的时候，重置所有flag
void Reset_flag(void);

//传递sensor_t
Sensor_t Get_SensorData(void);

// Get test index
uint8_t Get_test_index(uint8_t _mode,uint8_t _current);

// 判断目标点位是否在场站范围内
uint8_t if_target_valid(uint8_t _id);
#endif
