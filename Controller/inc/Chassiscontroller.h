#ifndef CHASSISCONTROLLER_H
#define CHASSISCONTROLLER_H

#include "can.h"
//#include "usart.h"
#include "gpio.h"

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


#include "ControllerDummy.h"
#include "ControllerRC.h"

#include "Robot_Config.h"


//typedef struct{
//uint16_t ultra_stop;
//GPIO_PinState pg_state;
//uint8_t action;
//uint8_t number;
//uint8_t front_state;
//uint8_t rear_state;
//uint8_t location_id;
//uint8_t previous_id;
//uint8_t if_finished;
//uint8_t need_charge;
//int move_direction;
//}Sensor_Struct;



//typedef enum {
//    STATE_IDLE,       
//    STATE_STOP,      
//    STATE_SLOWDOWN,
//	  STATE_PUSH,
//	  STATE_PULL,
//    STATE_RUNNING,
//		STATE_WORKING
//} State;


//typedef struct {
//	  State lastState;
//    State currentState;
//} StateMachine;


void Init(void);
void Update(void);

void Set_RGB(void);
//void SetxSpeed(int _speed);
//void SetySpeed(int _speed);
//int get_direction(uint8_t  location_id, uint8_t  target_id);
//uint8_t  get_previous_point(uint8_t  target_id, uint8_t  direction);
//void SwitchState(StateMachine *_sm);

#endif
