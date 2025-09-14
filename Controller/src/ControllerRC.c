#include "ControllerRC.h"

// RC variable
RC_raw_t m_rc_ctrl;

// Sensor 
Sensor_t m_sensor_t;

float speed_y = 0;
void RC_Control(char mode)
{
	  m_sensor_t = Get_SensorData();
		if(m_rc_ctrl.rc.ch[2] != 0)
		{
				//RGB_ON(YELLOW);
				SetxSpeed(-m_rc_ctrl.rc.ch[2] * 20.0f); //左横向
//				if(m_rc_ctrl.rc.ch[2] < 0 )
//				{
//			    SetxSpeed(-4000);
//				}
//				else
//				{
//					SetxSpeed(4000);
//				}
		}
		else if(m_rc_ctrl.rc.ch[1] != 0)
		{
				//RGB_ON(RED);
				
				speed_y = m_rc_ctrl.rc.ch[1] * 20.0f;
			
//				if(m_rc_ctrl.rc.ch[1] < 0 )
//				{
//			    speed_y = -4500;
//				}
//				else
//				{
//					speed_y = 4500;
//				}

				if ((m_sensor_t.rear_state == 1 && speed_y < 0) || (m_sensor_t.front_state == 1 && speed_y > 0)) {  // 如果后限位开关触发&要向后  或  前限位开关触发&要向前 --> 设置speed = 0
						speed_y = 0;
				}

				SetySpeed(speed_y);
		}
		else
		{
			//RGB_ON(GREEN);
			 SetxSpeed(0);
			 SetySpeed(0);
				
		}
		
		if(m_rc_ctrl.rc.s[0] == 1) //右上
		{
				//RGB_ON(RED);
			  if(mode == 2)
				{
					PushRod_Forward();
				}
				else if(mode == 3)
				{
					PushRod_StartCharge();	
				}
		}
		else if(m_rc_ctrl.rc.s[0] == 2) //右下
		{
			if(mode == 2)
				{
					PushRod_Backward();
				}
				else if(mode == 3)
				{
					PushRod_StopCharge();	
				}
		}
		
}

void RC_ctrl_Update(void)
{
		m_rc_ctrl = RC_GetData();

}
