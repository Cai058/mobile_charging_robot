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

// Flag
uint8_t slow_flag = 0;  // �Ƿ�������״̬
uint8_t charge_flag = 0;  // ���ĵ��Ƹ��Ƿ��Ƴ�
uint8_t push_flag = 0;    // �Ƿ��Ƴ�����
uint8_t rfid_flag = 0;    // If the UID is changed

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
	// Update
	Ultrawave_Update();
	//RFID
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
		m_ctrl.action = Get_ifaction();
		if(m_ctrl.number != 0){m_ctrl.location_id = m_ctrl.number;}  // ��¼��ǰ�����ĵ�λ
		m_ctrl.front_state = Get_frontlimitState();
		m_ctrl.rear_state = Get_rearlimitState();
		m_ctrl.need_charge = Battery_ifNeedCharge();
		m_ctrl.is_charging = Battery_isCharging();
		m_ctrl.m_soc = Get_SOC();
		if(m_ctrl.action == 1) {m_ctrl.if_finished = 0;}  // ֻҪ�յ���Ϣ����0����ʾδ��ɣ�, ����Ϊ0Ҳ����Ӱ�죬ֻ�е�����λ���ش�������ʾ���ʱ����if_finished = 1
		if(m_ctrl.available == 1)
		{
				m_server_ctrl =  Get_serverMsg();
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
				//1. �����ź�
				heart_cnt ++;
			 if(heart_cnt  % 30000 == 100)  // 30s
			 {
					send_json_response("free");
			 }
	 
			 //2. �յ�����
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
			 
			 //3. need charge
			 if(m_ctrl.place_complete && m_ctrl.need_charge)
			 {
					if(sm.lastState == STATE_PULL && charge_cnt < 5000)
					{
							charge_cnt++;
					}
					else
					{
						  charge_cnt = 0;
							heart_cnt = 0; // ������һ��״̬�����¿�ʼ����
							sm.currentState = STATE_RUNNING;
							sm.lastState = STATE_IDLE;
						  m_ctrl.available = 0;
							break;
					}
			 }
		 break;
		 
	 case STATE_RUNNING:
		 //1. �����ź�
			heart_cnt ++;
			 if(heart_cnt % 60000 == 500)  // 10���ӷ�һ��
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
			 //2. ����
			 
			 //2.1 ��ǰĿ���λ��a. ����λ b. ����/ץȡ��λ
			 if(m_ctrl.need_charge){m_ctrl.current_target_id = charge_id;}
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
		 
		
		 //2.4 �����ٶ�
		 if (m_ctrl.ultra_stop == 0)
		 { 
			 if(m_ctrl.location_id != m_ctrl.previous_id && slow_flag == 0)  // û�ƶ���ǰһ�㣺�����ٶ�  (����slow_flag��Ϊ����������ֹͣ��λ��Ӧ�ü�������Ѱ�ң�
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
			 if(heart_cnt % 30000 == 100)  // 10���ӷ�һ��
			 {
				  send_json_response("charge");
			 }
			 
			 // 
			 if(m_ctrl.m_soc >= 90)
			 {
					PushRod_StopCharge();
					sm.currentState = STATE_IDLE;
				  sm.lastState = STATE_CHARGING;
				  m_ctrl.available = 1;
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
//								  heart_cnt = 0; // ������һ��״̬�����¿�ʼ����
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
