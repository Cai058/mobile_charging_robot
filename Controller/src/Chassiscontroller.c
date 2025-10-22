#include "Chassiscontroller.h"

// PID controller const
float m_Kp = 3.0f;
float m_Ki = 0.15f;
float m_Kd = 0.0001f;
//float m_Kp = 1.0f;
//float m_Ki = 0.0;
//float m_Kd = 0.0f;
float m_Kp_y = 1.0f;
float m_Ki_y = 0.0f;
float m_Kd_y = 0.0f;
float m_maxout = 20000;
float m_intergral_limit = 4000; // 积分限幅
float m_deadband = 0.0f;
float m_I_band = 300.0f; // 积分分离 


// Motor 
int16_t motor_speed[4];   // save the speed of motor 
//int16_t limited_motor_speed[4];  // 限制后的速度
int16_t motor_speed_filtered[4]; // speed after filtered
Motor_measure_t Motor_measure[4];  // in bsp_can.h
PID_typedef Motor_pid[4];          // in pid.h  
VxFilter_t vx_filter[4];           // in first_order_filter.h
// Mode
char ctrl_mode = 0;         // 自动还是遥控
char last_ctrl_mode = 0;
char dummy_mode = 0;        // 自动: 平台控制还是测试
char last_dummy_mode = 0;


//is charge
uint8_t m_ischarge = 0;

void Init(void)
{
  // 初始化配置
  //LoadRobotConfig();
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
	VxFilter_Init(&vx_filter[0],0.01f,0.01f,0.00f,1); //tau1 = 0.01, tau2 = 0.01, tau3 = 0.00, period = 1ms
  	PID_Init(&Motor_pid[0],PID_POSITION_SPEED,m_Kp_y,m_Ki_y,m_Kd_y,m_intergral_limit,m_maxout,m_deadband,m_I_band);	
	//Init X
	for(int i=1; i<4; i++)
	{
	 // Filter
	VxFilter_Init(&vx_filter[i],0.3f,0.01f,0.00f,1); //tau1 = 0.15, tau2 = 0.05, tau3 = 0.01, period = 1ms
   	PID_Init(&Motor_pid[i],PID_POSITION_SPEED,m_Kp,m_Ki,m_Kd,m_intergral_limit,m_maxout,m_deadband,m_I_band);		//4 motos angular rate closeloop.
	}
	
	motor_speed[0] = motor_speed[1] = motor_speed[2] = motor_speed[3] = 0;

}


void Update(void)
{
// 更新各个传感器,获得各个传感器的值
Sensor_t_Update();
ctrl_mode = RC_GetMode();
dummy_mode = RC_GetDummyMode();
m_ischarge = Battery_isCharging();

	
// Update State
if(ctrl_mode == 1)  // 左上  进入自动
{
	if(last_ctrl_mode != 1)  // The first siwtch, init the state to IDLE
	{
		last_ctrl_mode = 1;
		Dummy_Reset();
		SetxSpeed(0,2);
		SetySpeed(0);
		  //send_json_response("free");
	}
	if(last_dummy_mode != dummy_mode) // 如果平台和测试模式切换了，重置 （注意，变成遥控状态时，不会切换）
	{
		last_dummy_mode = dummy_mode;
		Dummy_Reset();
	}

	Dummy_Update();
	SwitchState();
	
	
}
else if(ctrl_mode == 2 || ctrl_mode == 3) // 左中/左下  进入遥控
{
	if(last_ctrl_mode != 2)
	{
		  SetxSpeed(0,2);
		  SetySpeed(0);
		 last_ctrl_mode = 2;
	}
	// 获得遥控器的值
	RC_ctrl_Update();
	RC_Control(ctrl_mode);
}

// 获得Motor目标速度值

Get_MotorSpeed(motor_speed);


// limited_motor_speed[0] = motor_speed[0];  //小盒子不作速度限制
// limited_motor_speed[1] = limit_speed(Motor_measure[1].speed, motor_speed[1], 500); // 每次最大变化200
// limited_motor_speed[2] = -limited_motor_speed[1];
//m_stop_flag= Get_stop_flag();
// Send Msg to motor	
for(int i=0;i<4;i++)
{
	// 如果是正常的
	// if(m_stop_flag == 1){VxFilter_Update(motor_speed[i],&vx_filter[i]);}
	// else{VxFilter_Update(limited_motor_speed[i],&vx_filter[i]);}
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
