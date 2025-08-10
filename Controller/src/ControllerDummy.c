#include "ControllerDummy.h"

#define TOTAL_POINTS 18
#define charge_id 1
#define turn_num1 1
#define turn_num2 9
#define turn_num3 10
#define turn_num4 18

uint16_t debug_flag;

Sensor_t m_ctrl;              //Sensor
ServerMsg_t m_server_ctrl;    //Server

// Test 
//direction
char m_direction = 0;
char m_direction1 = 1; //direction of mode 1
char m_mode_ctrl = 1;  // default dummy
//test_index
uint8_t test_index = 1;
uint8_t test_mode = 1;

StateMachine sm;              //State Machine

int16_t m_motor_speed[4];     //Motor_speed

// Count
uint8_t i;
uint16_t pushrod_cnt = 0;
uint32_t heart_cnt = 0;
uint32_t switch_cnt = 0;
uint16_t charge_cnt = 0;
uint16_t rail_cnt = 0;
uint16_t rfid_rx_cnt = 0;
uint16_t stop_charge_cnt = 0;

// Flag
uint8_t slow_flag = 0;  // 是否是慢速状态
uint8_t charge_flag = 0;  // 充电的电推杆是否推出
uint8_t push_flag = 0;    // 是否推出滑杆
uint8_t rfid_flag = 0;    // If the UID is changed

// ERROR
uint8_t error_code = 0;
const char* error_list[] = {
    "正常",            // 0
    "遇到障碍",        // 1
    "充电异常",        // 2
    "前限位开关未触发", // 3
    "后限位开关未触发"  // 4
};
void Sensor_t_Update(void)
{	
	// *********Update***************
	Ultrawave_Update();
	//== RFID ==
	if(m_ctrl.number != 0){rfid_flag = 1;}
	if(rfid_flag == 1)
	{
		  m_ctrl.number = 0;
			if(rfid_rx_cnt < 1500){rfid_rx_cnt++;}
			else
			{
					rfid_rx_cnt = 0;
					rfid_flag = 0;
			}
  }
	else{
		RFID_Update();
		m_ctrl.number = Get_Number();   // only get the number when it update.
	}
	
	//RFID_Update();
	Photogate_Update();
	Server_Update();
	Battery_Update();
	RC_Update();
	
	// Assign to variable
	//m_ctrl.ultra_stop = Ultrawave_IfStop(); 
	
	m_ctrl.ultra_stop =0;
	m_ctrl.pg_state = Get_PGState();
	if(m_ctrl.number != 0){m_ctrl.location_id = m_ctrl.number;}  // 记录当前经过的点位
	m_ctrl.front_state = Get_frontlimitState();
	m_ctrl.rear_state = Get_rearlimitState();
	m_ctrl.need_charge = Battery_ifNeedCharge();
	m_ctrl.is_charging = Battery_isCharging();
	m_ctrl.m_soc = Get_SOC();
	
}

void Dummy_Update(void)
{
		m_direction = RC_GetDirection();
		//m_mode_ctrl = RC_GetMode();
		m_ctrl.action = (m_direction == 3) ? Get_ifaction() : 1;  //if test or server control
	  if(m_ctrl.action == 1) {m_ctrl.if_finished = 0;}  // 只要收到消息就是0（表示未完成）, 它变为0也不受影响，只有当后限位开关触发（表示完成时），if_finished = 1
		
		if(m_ctrl.available == 1)
		{
				if(m_direction == 3) // server control
				{
						m_server_ctrl =  Get_serverMsg();
				}
				else               // test
				{
						m_server_ctrl.target_id = test_index;
						m_server_ctrl.command = (m_ctrl.place_complete == 0) ? 1:0;
						if(m_server_ctrl.command == 0)
						{
								//test_index = (m_direction == 1) ? ((test_index %18)+1) : ((test_index + 16 )%18 + 1);
								test_index = Get_test_index(test_mode,test_index);
						}
				}
		}

}
void Dummy_Init(void)
{
		StateMachine_Init();
		Sensor_t_Init();
		PushRod_StopCharge();	
		Reset_flag();
		heart_cnt = 0;
}

void SwitchState(void)
{
 switch(sm.currentState){
	 case STATE_IDLE:
				//1. 心跳信号
				heart_cnt ++;
			 if(heart_cnt  % 30000 == 100)  // 30s
			 {
					send_json_response("free");
			 }
	 
			 //2. need charge
			 if(m_ctrl.place_complete && m_ctrl.need_charge)
			 {
//					if(sm.lastState == STATE_PULL && charge_cnt < 5000)
//					{
//							charge_cnt++;
//					}
//					else
//					{
						  charge_cnt = 0;
							heart_cnt = 0; // 进入下一个状态，重新开始计算
							sm.currentState = STATE_RUNNING;
							sm.lastState = STATE_IDLE;
						  m_ctrl.available = 0;
							break;
					//}
			 }
			 
			 //3. 收到任务
			 if(m_ctrl.if_finished == 0)   // or need charge
			 {
					 heart_cnt = 0; // 进入下一个状态，重新开始计算
					 sm.currentState = STATE_RUNNING;
					 sm.lastState = STATE_IDLE;
				   m_ctrl.available = 0; // Server can not send task
				   m_ctrl.place_complete = 0;
					 send_json_response("Success");
					 break;
			 }
			 
			 
		 break;
		 
	 case STATE_RUNNING:
		 //1. 心跳信号
			heart_cnt ++;
			 if(heart_cnt % 60000 == 500)  // 10分钟发一次
			 {   
				  if(m_ctrl.need_charge)
					{
							send_json_response("move_charge");
					}
					else if(m_server_ctrl.command == 0)
					{
							send_json_response("move_pick");
					}
					else if(m_server_ctrl.command == 1)
					{
						send_json_response("move_place"); 
					}
			 }
			 //2. 行走
			 
			 //2.1 当前目标点位：a. 充电点位 b. 放置/抓取点位
			 if(m_ctrl.need_charge){m_ctrl.current_target_id = charge_id;}
			 else{m_ctrl.current_target_id = m_server_ctrl.target_id;}
			 
			
			//2.2 计算方向 & 前一个点位信息 (when ID is correct but pg is not)
			if(m_ctrl.location_id != m_ctrl.current_target_id)
			{
			m_ctrl.move_direction = get_direction(m_ctrl.location_id,m_ctrl.current_target_id);
			m_ctrl.previous_id = get_previous_point(m_ctrl.current_target_id,m_ctrl.move_direction);
			}
			
			//2.3 如果识别到点位 & 光电门触发
		 if(m_ctrl.pg_state == 1 && m_ctrl.location_id == m_ctrl.current_target_id)  
		 {
					SetxSpeed(0);
					if(m_ctrl.need_charge == 1)
					{
							sm.currentState = STATE_CHARGING;
							send_json_response("arrive");
							heart_cnt = 0;
					}
					else
					{
							sm.currentState = STATE_WORKING;
						  m_ctrl.available = 0;
							send_json_response("arrive");
							heart_cnt = 0;
						  rail_cnt = 0;
					}
					sm.lastState = STATE_RUNNING;
		 }
		 
		
		 //2.4 设置速度
		 if (m_ctrl.ultra_stop == 0)
		 { 
			 if(m_ctrl.location_id != m_ctrl.previous_id && slow_flag == 0)  // 没移动到前一点：正常速度  (设置slow_flag是为了如果错过了停止点位，应该继续慢速寻找）
			 {
				 debug_flag = 11;
				 if(m_ctrl.move_direction == 1)   // turn point: slow down; others: normal speed
				 {
						if(m_ctrl.location_id == turn_num2 || m_ctrl.location_id == turn_num4)
						{
								SetxSpeed(-1500);
						}
						else
						{
								SetxSpeed(-3000);
						}
				 }
				 else
					{
						if(m_ctrl.location_id == turn_num1 || m_ctrl.location_id == turn_num3)
						{
								SetxSpeed(1500);
						}
						else
						{
								SetxSpeed(3000);
						}
				 } 
			 }
			 else
       {
				   debug_flag = 10;
				   (m_ctrl.move_direction == 1) ? SetxSpeed(-1500) : SetxSpeed(1500);
						slow_flag = 1;
       }				 
		 }
		   
		break;
		 
	 case STATE_WORKING:
				SetxSpeed(0);
				rail_cnt++;
				if( push_flag != 1 ) //还没有推出
				 {
					   if(rail_cnt %300 == 0)
						 {
						 sm.currentState = STATE_PUSH;
						 sm.lastState = STATE_WORKING;
						 SetySpeed(4500);
						 switch_cnt = 0; // 用于判断是否过长时间 限位开关未触发
						 }
				 }
				 else
				 {
					 if(m_server_ctrl.command == 0){PushRod_Forward();}
					 else if(m_server_ctrl.command == 1){PushRod_Backward();}
					 pushrod_cnt++;
					 if(pushrod_cnt > 15000)
					 {
						 SetySpeed(-4500);
						 sm.currentState = STATE_PULL;
						 sm.lastState = STATE_WORKING;
						 pushrod_cnt = 0;
						 switch_cnt = 0;
					 }
				 }
	 
	 
	 break;
		 
		 
	 case STATE_PUSH:
			 switch_cnt ++;
//			 if(switch_cnt > 60000) // 超过一分钟
//       {
//						error_code = 3;
//						sm.currentState = STATE_ERROR;
//						sm.lastState = STATE_PUSH;
//			 }				 
			 
		   if(m_ctrl.front_state == 1)
			 {
				 SetySpeed(0);
				 sm.currentState = STATE_WORKING;
				 sm.lastState = STATE_PUSH;
				 push_flag = 1;
			 }
		 break;
			 
	 case STATE_PULL:
		 switch_cnt ++;
//			 if(switch_cnt > 60000) // 超过一分钟
//       {
//						error_code = 4;
//						sm.currentState = STATE_ERROR;
//						sm.lastState = STATE_PULL;
//			 }	
			 
		 if(m_ctrl.rear_state == 1)
			 {
				 SetySpeed(0);
				 sm.currentState = STATE_IDLE;
				 sm.lastState = STATE_PULL;
				 m_ctrl.available = 1;
				 m_ctrl.if_finished = 1;
				 Reset_flag();
				 
					if(m_server_ctrl.command == 0)
					{
							send_json_response("pick");
					}
					else if(m_server_ctrl.command == 1)
					{
							m_ctrl.place_complete = 1;
							send_json_response("place");
						
					}
			 }
			 
			
		 break;
		 
		 
		 case STATE_CHARGING:
		    // 1.正在充电状态反馈 & 充电异常反馈
				heart_cnt ++;
				SetxSpeed(0);
			 if(heart_cnt % 30000 == 100)  // 10分钟发一次
			 {
				  send_json_response("charge");
			 }
			 
			 // 
			 if(m_ctrl.m_soc >= 90)
			 {
				 PushRod_StopCharge();
				 if(stop_charge_cnt < 10000){stop_charge_cnt ++;}
				 else{
					sm.currentState = STATE_IDLE;
				  sm.lastState = STATE_CHARGING;
				  m_ctrl.available = 1;
					stop_charge_cnt = 0;
				 }
			 }
			 else
			 {
					PushRod_StartCharge();
				  if(m_ctrl.m_soc < 30)
					{
							m_ctrl.available = 0;
					}
			 }
			 
			 
//			 
//			 else 
//			 {
//					if(m_ctrl.if_finished == 0)
//					{
//							PushRod_StopCharge();
//							if(m_ctrl.m_soc < 30)
//							{
//									m_ctrl.available = 0;
//								  m_ctrl.if_finished = 1; // otherwise when soc >30, it will automally move.
//								  send_json_response("Fail");
//							}
//							else
//							{
//									m_ctrl.available = 1;
//								  heart_cnt = 0; // 进入下一个状态，重新开始计算
//								  sm.currentState = STATE_RUNNING;
//								  sm.lastState = STATE_IDLE;
//								  send_json_response("Success");
//								  send_json_response("move");
//								  break;
//							}
//					}
//					else
//					{
//							PushRod_StartCharge();
//					}
//			 }
		 
		 
		 break;
		 
		 	case STATE_ERROR:
				
			// 其它故障怎么解决？
		  SetxSpeed(0);
			SetySpeed(0);
			heart_cnt ++;
			 if(heart_cnt % 600000 == 0)  // 10分钟发一次
			 {
					send_json_response(error_list[error_code]); //根据error_code发送故障信息
			 }
			 
	   // 1. 处理因为障碍物停下的情况
	   if(error_code == 1 && m_ctrl.ultra_stop == 0) 
		 {
			 error_code = 0;
			 sm.currentState = sm.lastState;
			 sm.lastState = STATE_ERROR;		 
		 }
		 else
		 {
			 sm.currentState = STATE_ERROR;
		 }
		 break;
		 
   }
}

void SetxSpeed(int _speed)
	{
	// m/s   --
	m_motor_speed[1] = _speed;
	m_motor_speed[2] = -_speed;
}
	
void SetySpeed(int _speed)
{
	m_motor_speed[0] = _speed;
}

// 计算行进方向：顺时针返回 1，逆时针返回 -1
int get_direction(uint8_t  location_id, uint8_t  target_id) {
    int cw_distance = (target_id - location_id + TOTAL_POINTS) % TOTAL_POINTS;
    int ccw_distance = (location_id - target_id + TOTAL_POINTS) % TOTAL_POINTS;
    return (cw_distance <= ccw_distance) ? 1 : -1;
}

// 计算目标点位的前一个点位
uint8_t  get_previous_point(uint8_t target_id, int direction) {
    return (target_id - direction - 1 + TOTAL_POINTS) % TOTAL_POINTS + 1;
}

void Sensor_t_Init(void)
{
		//sensors state init
		m_ctrl.ultra_stop = 0;
		m_ctrl.pg_state = 0;
		m_ctrl.action = 0;
		m_ctrl.number = 0;
		m_ctrl.front_state = 0;
		m_ctrl.rear_state = 0;
		//m_ctrl.location_id = 0;
		m_ctrl.previous_id = 0;
		m_ctrl.if_finished = 1; //先是任务完成
		m_ctrl.need_charge = 0;
		m_ctrl.is_charging = 0;
		m_ctrl.move_direction = 1;
	  m_ctrl.current_target_id = 0;
		m_ctrl.available = 1;
		m_ctrl.place_complete = 1;
}

void StateMachine_Init(void)
{
		sm.lastState = STATE_IDLE;
		sm.currentState = STATE_IDLE;
}

void Get_MotorSpeed(int16_t *_speed)
{
		memcpy(_speed, m_motor_speed, 8);
}

void Reset_flag(void)
{
		slow_flag = 0;  // 是否是慢速状态
    charge_flag = 0;  // 充电的电推杆是否推出
    push_flag = 0; 
		error_code = 0;
}

Sensor_t Get_SensorData(void)
{
		return m_ctrl;

}

uint8_t Get_test_index(uint8_t _mode,uint8_t _current)
{
		uint8_t _next = _current;
	  char _direction;
	
	  if(_mode == 1)
		{
				if(_next == 1){m_direction1 = 1;}  // 1-9
				else if(_next == 9){m_direction1 = 2;} //9-1
				_direction = m_direction1;
		}
		else
		{
				_direction = m_direction;
		}
		
		if(_mode == 1 || _mode == 2)
		{
				do{_next = (_direction == 1) ? ((_next %18)+1) : ((_next + 16 )%18 + 1);}
				while(_next == 3 || _next == 10 || _next == 14 || _next == 15 || _next == 18);
		}
		else
		{
				_next = (_direction == 1) ? ((_next %18)+1) : ((_next + 16 )%18 + 1);
		}
	  
		return _next;
}
