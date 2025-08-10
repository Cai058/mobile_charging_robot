#include "Chassiscontroller.h"

// PID controller const
float m_Kp = 15.0f;
float m_Ki = 0.15f;
float m_Kd = 0.0001f;
float m_Kp_y = 1.0f;
float m_Ki_y = 0.0f;
float m_Kd_y = 0.0f;
float m_maxout = 20000;
float m_intergral_limit = 20000;
float m_deadband = 0.0f;


// Motor 
int16_t motor_speed[4];   // save the speed of motor
int16_t motor_speed_filtered[4]; // speed after filtered
Motor_measure_t Motor_measure[4];  // in bsp_can.h
PID_typedef Motor_pid[4];          // in pid.h
VxFilter_t vx_filter[4];           // in first_order_filter.h
// Mode
char mode_ctrl = 0;
char last_mode_ctrl = 0;


//is charge
uint8_t m_ischarge = 0;


void Init(void)
{
  //CAN
	MX_GPIO_Init();
  MX_CAN1_Init();
	can_filter_init();
	
	//sensors GPIO Init
	LED_GPIO_Config();
	Key_GPIO_Config();
	Ultrawave_Config();
	RFID_Config();  
	Photogate_Config();
	Limit_Switch_Config();
	L298N_Config();
	RGB_Config();
	Battery_Init();
	RC_Init();
	
	// Server Msg Init
	Server_Init();
	
  // Sensor_t and Statemachine Init
	Dummy_Init();
	
	//PID and Motor Init
	//Init Y
	VxFilter_Init(&vx_filter[0],0.01f,0.01f,0.00f,1); //tau1 = 0.15, tau2 = 0.05, tau3 = 0.01, period = 1ms
  PID_Init(&Motor_pid[0],PID_POSITION_SPEED,m_Kp_y,m_Ki_y,m_Kd_y,m_intergral_limit,m_maxout,m_deadband);	
	//Init X
	for(int i=1; i<4; i++)
	{
	 // Filter
	 VxFilter_Init(&vx_filter[i],0.01f,0.01f,0.00f,1); //tau1 = 0.15, tau2 = 0.05, tau3 = 0.01, period = 1ms
   PID_Init(&Motor_pid[i],PID_POSITION_SPEED,m_Kp,m_Ki,m_Kd,m_intergral_limit,m_maxout,m_deadband);		//4 motos angular rate closeloop.
	}
	
	motor_speed[0] = motor_speed[1] = motor_speed[2] = motor_speed[3] = 0;

}


void Update(void)
{
// 更新各个传感器,获得各个传感器的值
Sensor_t_Update();
mode_ctrl = RC_GetMode();
m_ischarge = Battery_isCharging();

	
// Update State
if(mode_ctrl == 1)  // 左上
{
	if(last_mode_ctrl != 1)  // The first siwtch, init the state to IDLE
	{
			last_mode_ctrl = 1;
		  Dummy_Init();
			SetxSpeed(0);
		  SetySpeed(0);
		  //send_json_response("free");
	}
		Dummy_Update();
		SwitchState();
	
	
}
else if(mode_ctrl == 2 || mode_ctrl == 3) // 左下
{
	if(last_mode_ctrl != 2)
	{
		  SetxSpeed(0);
		  SetySpeed(0);
			last_mode_ctrl = 2;
	}
	// 获得遥控器的值
	RC_ctrl_Update();
	RC_Control(mode_ctrl);
}

// 获得Motor目标速度值

Get_MotorSpeed(motor_speed);

// Send Msg to motor	

for(int i=0;i<4;i++)
{
	VxFilter_Update(motor_speed[i],&vx_filter[i]);
	motor_speed_filtered[i] = VxFilter_GetResult(&vx_filter[i]);
	Motor_measure[i].Output = PID_calc(&Motor_pid[i],Motor_measure[i].speed,motor_speed_filtered[i]);
}
	Set_motor_cmd(&hcan1,First_STDID,Motor_measure[0].Output,Motor_measure[1].Output,Motor_measure[2].Output,Motor_measure[3].Output);

Set_RGB();

}


void Set_RGB(void)
{
		if(motor_speed[1] != 0)
		{
				RGB_ON(YELLOW);
		}
		else if(motor_speed[0] != 0)
		{
				RGB_ON(RED);
		}
		else if(m_ischarge == 1)
		{
				RGB_ON(CYAN);
		}
		else
		{
				RGB_ON(GREEN);
		}
}
