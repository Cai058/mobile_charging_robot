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
uint16_t pushrod_cnt = 0;          //ȡ��׮�Ƹ�����ʱ��
uint32_t heart_cnt = 0;
uint32_t switch_cnt = 0;           //�����ж���λ�����Ƿ����ʱ��δ����
uint16_t charge_cnt = 0;           //�ӻ�������ɷ�׮����ǰ�����ĵȴ�ʱ��
uint16_t rail_cnt = 0;             //ͣ�Ⱥ󣬵ȴ�һ��ʱ�����������
uint16_t rfid_rx_cnt = 0;          // RFIDֹͣ���ʱ��
uint16_t charge_pushrod_cnt = 0;   //����Ƹ�����ʱ��
uint16_t adjust_cnt = 0;           //�ȴ�������ͣ�ȵ�ʱ��
uint16_t charging_adjust_cnt = 0;  //�������ƶ�ʱ��
uint16_t charge_err_cnt = 0;       //���δͣ��ʱ��
uint16_t find_cnt = 0;

// Flag
uint8_t slow_flag = 0;  // �Ƿ�������״̬
uint8_t charge_flag = 0;  // ���ĵ��Ƹ��Ƿ��Ƴ�
uint8_t push_flag = 0;    // �Ƿ��Ƴ�����
uint8_t rfid_flag = 0;    // If the UID is changed
uint8_t charging_adjust_flag = 0; // �����ж��ǵ���ǰ�������У�������

// ERROR
uint8_t error_code = 0;
const char* error_list[] = {
    "����",            // 0
    "�����ϰ�",        // 1
    "����쳣",        // 2
    "ǰ��λ����δ����", // 3
    "����λ����δ����"  // 4
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
	if(m_ctrl.number != 0){m_ctrl.location_id = m_ctrl.number;}  // ��¼��ǰ�����ĵ�λ
	m_ctrl.front_state = Get_frontlimitState();
	m_ctrl.rear_state = Get_rearlimitState();
	m_ctrl.need_charge = Battery_ifNeedCharge();
	m_ctrl.is_charging = Battery_isCharging();
	m_ctrl.m_soc = Get_SOC();
	
}

void Dummy_Update(void)
{
		m_direction = RC_GetDirection();
		m_ctrl.action = (m_direction == 3) ? Get_ifaction() : 1;  //if test or server control
	  if(m_ctrl.action == 1) {m_ctrl.if_finished = 0;}  // ֻҪ�յ���Ϣ����0����ʾδ��ɣ�, ����Ϊ0Ҳ����Ӱ�죬ֻ�е�����λ���ش�������ʾ���ʱ����if_finished = 1
		
		if(m_ctrl.available == 1)                        //ֻ��availableʱ�Ÿ�m_server_ctrl��ֵ
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
	if(m_ctrl.place_complete && m_ctrl.need_charge)
	{
			m_ctrl.charge_mode = 1;
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

void Dummy_Reset(void)
{
	StateMachine_Init();
	PushRod_StopCharge();
	Reset_flag();
	heart_cnt = 0;
	//����m_ctrl,ֻ������Ҫ��
	Sensor_t_Reset();
}

void Sensor_t_Reset(void)
{
	m_ctrl.action = 0;
	m_ctrl.if_finished = 1;
	m_ctrl.move_direction = 1;
	m_ctrl.charge_mode = 0;
	m_ctrl.current_target_id = 1;
	m_ctrl.available = 1;
	m_ctrl.place_complete = 1;
}

void SwitchState(void)
{
 switch(sm.currentState){
	 case STATE_IDLE:
				//1. �����ź�
				heart_cnt ++;
			 if(heart_cnt  % 30000 == 100)  // 30s
			 {
					send_json_response("free");
			 }
	 
			 //2. need charge
			 if(m_ctrl.charge_mode == 1)
			 {
//					if(sm.lastState == STATE_PULL && charge_cnt < 5000)
//					{
//							charge_cnt++;
//					}
//					else
//					{
				      //m_ctrl.charge_mode = 1;
						  charge_cnt = 0;
							heart_cnt = 0; // ������һ��״̬�����¿�ʼ����
							sm.currentState = STATE_RUNNING;
							sm.lastState = STATE_IDLE;
						  m_ctrl.available = 0;
							break;
					//}
			 }
			 
			 //3. �յ�����
			 if(m_ctrl.if_finished == 0)   // or need charge
			 {
					 heart_cnt = 0; // ������һ��״̬�����¿�ʼ����
					 sm.currentState = STATE_RUNNING;
					 sm.lastState = STATE_IDLE;
				   m_ctrl.available = 0; // Server can not send task
				   m_ctrl.place_complete = 0;
					 send_json_response("Success");
					 break;
			 }
			 
			 
		 break;
		 
	 case STATE_RUNNING:
		 //1. �����ź�
			heart_cnt ++;
			 if(heart_cnt % 60000 == 500)  // 10���ӷ�һ��
			 {   
				  if(m_ctrl.charge_mode == 1)
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
			 //2. ����
			 
			 //2.1 ��ǰĿ���λ��a. ����λ b. ����/ץȡ��λ
			 if(m_ctrl.charge_mode == 1){m_ctrl.current_target_id = charge_id;}
			 else{m_ctrl.current_target_id = m_server_ctrl.target_id;}
			 
			
			//2.2 ���㷽�� & ǰһ����λ��Ϣ (when ID is correct but pg is not)
			if(m_ctrl.location_id != m_ctrl.current_target_id)
			{
			m_ctrl.move_direction = get_direction(m_ctrl.location_id,m_ctrl.current_target_id);
			m_ctrl.previous_id = get_previous_point(m_ctrl.current_target_id,m_ctrl.move_direction);
			}
			
			//2.3 ���ʶ�𵽵�λ & ����Ŵ���
		 if(m_ctrl.pg_state == 1 && m_ctrl.location_id == m_ctrl.current_target_id)  
		 {
					SetxSpeed(0);
			    sm.currentState = STATE_ADJUST;
					sm.lastState = STATE_RUNNING;
			 break;
		 }
		 
		
		 //2.4 �����ٶ�
		 if (m_ctrl.ultra_stop == 0)
		 { 
			 
			 // Adjust
			 if(sm.lastState == STATE_ADJUST)
			 {
				 if(m_ctrl.move_direction == 1)
				 {
						SetxSpeed(-50);
				 }
				 else
				 {
						SetxSpeed(50);
				 }
			 }
			 else
			 {
			 // Normal
			 if(m_ctrl.location_id != m_ctrl.previous_id && slow_flag == 0)  // û�ƶ���ǰһ�㣺�����ٶ�  (����slow_flag��Ϊ����������ֹͣ��λ��Ӧ�ü�������Ѱ�ң�
			 {
				 debug_flag = 11;
				 if(m_ctrl.move_direction == 1)   // turn point: slow down; others: normal speed
				 {
						if(m_ctrl.location_id == turn_num2 || m_ctrl.location_id == turn_num4)
						{
								SetxSpeed(-2500);
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
								SetxSpeed(2500);
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
		 }
		   
		break;
	
	 case STATE_ADJUST:
		    
	      if(adjust_cnt > 2000)
				{
					if(m_ctrl.pg_state == 1 && m_ctrl.location_id == m_ctrl.current_target_id)  
					{
						adjust_cnt = 0;
						SetxSpeed(0);
						if(find_cnt > 2)
						{
							find_cnt = 0;
						if(m_ctrl.charge_mode == 1)
						{
								sm.currentState = STATE_CHARGING;
								send_json_response("arrive");
								heart_cnt = 0;
								m_ctrl.move_direction = m_ctrl.move_direction * -1; // ���ʱ������ת���Ա��ڳ���λ����
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
						else
						{
								find_cnt++;
						}
					}
					else
					{
						  find_cnt = 0;
							m_ctrl.move_direction = -m_ctrl.move_direction;
							sm.lastState = STATE_ADJUST;
							sm.currentState = STATE_RUNNING;
					}
			}
			else
			{
					adjust_cnt ++;
			}
		 break;
	 
	 case STATE_WORKING:
				SetxSpeed(0);
				rail_cnt++;
				if( push_flag != 1 ) //��û���Ƴ�
				 {
					   if(rail_cnt %300 == 0)
						 {
						 sm.currentState = STATE_PUSH;
						 sm.lastState = STATE_WORKING;
						 SetySpeed(4500);
						 switch_cnt = 0; // �����ж��Ƿ����ʱ�� ��λ����δ����
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
//			 if(switch_cnt > 60000) // ����һ����
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
//			 if(switch_cnt > 60000) // ����һ����
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
		    // 1.���ڳ��״̬���� & ����쳣����
			heart_cnt ++;
			SetxSpeed(0);
			// ������쳣����������ģʽ�������޷����
			// if(charge_err_cnt > 60000)
			// {
			// 	error_code = 5;
			// 	sm.currentState = STATE_ERROR;
			// 	sm.lastState = STATE_CHARGING;
			// }
//			 if(m_ctrl.is_charging == 0)
//			 {
//				charge_err_cnt++;  // ��¼���Ӵ�����ʱ��
//				Charging_Adjust(); // ��������Ƹ�λ��
//			 }
//			 else
//			 {
				if(heart_cnt % 30000 == 100)  // 10���ӷ�һ��
				{
					send_json_response("charge");
				}
				
				// 
				if(m_ctrl.m_soc >= 99)
				{
					PushRod_StopCharge();
					if(charge_pushrod_cnt < 10000){charge_pushrod_cnt ++;}
					else{
						sm.currentState = STATE_IDLE;
						sm.lastState = STATE_CHARGING;
						m_ctrl.available = 1;
						m_ctrl.charge_mode = 0;
						charge_pushrod_cnt = 0;
					}
				}
				else
				{
					charging_adjust_flag = 0;      // �����Ƹ��������λ�ò�׼�������µ���
					charge_pushrod_cnt = 0;
					PushRod_StartCharge();
					if(m_ctrl.m_soc < 30)
						{
							m_ctrl.available = 0;
							if(m_ctrl.if_finished == 0)
								{
									send_json_response("Fail");
									m_ctrl.if_finished = 1;
								}
						}
				}
			 //}

		 break;
		 
		 	case STATE_ERROR:
				
			// ����������ô�����
		    SetxSpeed(0);
			SetySpeed(0);
			heart_cnt ++;
			 if(heart_cnt % 600000 == 0)  // 10���ӷ�һ��
			 {
					send_json_response(error_list[error_code]); //����error_code���͹�����Ϣ
			 }
			 
	   // 1. ������Ϊ�ϰ���ͣ�µ����
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

// �����н�����˳ʱ�뷵�� 1����ʱ�뷵�� -1
int get_direction(uint8_t  location_id, uint8_t  target_id) {
    int cw_distance = (target_id - location_id + TOTAL_POINTS) % TOTAL_POINTS;
    int ccw_distance = (location_id - target_id + TOTAL_POINTS) % TOTAL_POINTS;
    return (cw_distance <= ccw_distance) ? 1 : -1;
}

// ����Ŀ���λ��ǰһ����λ
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
	m_ctrl.if_finished = 1; //�����������
	m_ctrl.need_charge = 0;
	m_ctrl.is_charging = 0;
	m_ctrl.charge_mode = 0;
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
	slow_flag = 0;  // �Ƿ�������״̬
    charge_flag = 0;  // ���ĵ��Ƹ��Ƿ��Ƴ�
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
				else if(_next == 17){m_direction1 = 2;} //9-1
				_direction = m_direction1;
		}
		else
		{
				_direction = m_direction;
		}
		
		if(_mode == 1 || _mode == 2)
		{
				do{_next = (_direction == 1) ? ((_next %18)+1) : ((_next + 16 )%18 + 1);}
				while(_next == 18);
		}
		else
		{
				_next = (_direction == 1) ? ((_next %18)+1) : ((_next + 16 )%18 + 1);
		}
	  
		return _next;
}

void Charging_Adjust(void)
{
	switch (charging_adjust_flag) 
	{
	case 0:
		PushRod_StopCharge();
		if(charge_pushrod_cnt < 3000){charge_pushrod_cnt ++;}
		else
		{
			charging_adjust_flag = 1;
			charging_adjust_cnt = 0;
			charge_pushrod_cnt = 0;
		}
		break;
	
	case 1:
		if(charging_adjust_cnt < 20)  // Լ0.02��
            {
                charging_adjust_cnt++;
                SetxSpeed(m_ctrl.move_direction * 500);  // �����������ƶ�,�ٶ�ԼΪ0.05m/s
            }
            else
            {
                SetxSpeed(0);
                charging_adjust_flag = 2;
                charging_adjust_cnt = 0;
            }
		break;
	
	case 2:
		PushRod_StartCharge();
		if(charge_pushrod_cnt < 5000){charge_pushrod_cnt ++;}  // ���ֳ�����
		charging_adjust_flag = 0;                              // �����Ƹ��������λ�ò�׼�������µ���
		charge_pushrod_cnt = 0;
        break;

	default:
		break;
	}
}
