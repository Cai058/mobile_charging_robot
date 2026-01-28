#include "pid.h"
#include <math.h>
#include <main.h>

int cha;
/**
  * @brief          输出值限制
  * @retval         none
  */
#define LimitMax(input, max)   \
    {                          \
        if (input > max)       \
        {                      \
            input = max;       \
        }                      \
        else if (input < -max) \
        {                      \
            input = -max;      \
        }                      \
    }



/**
  * @brief          pid struct data init，结构体初始化，计算的相关参数通过函数参数传到pid结构体
  * @param[out]     pid: PID结构数据指针
  * @param[in]      mode: PID_POSITION_SPEED: 位置式PID，速度
  *                 			PID_POSITION_ANGLE: 位置式PID，角度
	*  											PID_DELTA_SPEED		：增量式PID，速度
  * @param[in]      kp:PID参数p
	* @param[in] 			ki:PID参数i
	* @param[in] 			kd:PID参数d
  * @param[in]      Max_iout:pid最大积分输出
  * @param[in]      Max_out:pid最大输出
	* @param[in]			deadband:PID死区
  * @retval         none
  */
void PID_Init(PID_typedef *PID,PID_mode Mode,float kp,float ki,float kd,float Max_iout,float Max_out,float deadband,float I_band)
{
	if(PID == NULL)	return;
	
	PID->mode = Mode;																														//把函数参数传入对应的结构体中
	PID->Kp = kp;
	PID->Ki = ki;
	PID->Kd = kd;
	PID->Max_iout = Max_iout;
	PID->Max_out = Max_out;
	PID->DeadBand = deadband;
	PID->I_band = I_band;
}


/**
  * @brief          pid计算
  * @param[out]     pid: PID结构数据指针
  * @param[in]      measure:反馈测量数据
	* @param[in] 			target: 目标值
  * @retval         none
  */
float PID_calc(PID_typedef *PID, float measure, float target)
{
	if(PID == NULL)
		return 0;
	
	
	PID->error[2] = PID->error[1];																							//误差值传递，1为上一次，2为上上次
	PID->error[1] = PID->error[0];																							//误差值传递，0为最新误差，1为上一次
	PID->measure = measure;																											//参数传递，下同
	PID->target = target;
	PID->error[0] =target - measure;																						//误差值计算，目标值-测量值
	
	if(fabsf(PID->error[0]) > PID->DeadBand || PID->DeadBand==0){								//判断死区，不死区内->计算
		
		if(PID->mode == PID_POSITION_SPEED || PID->mode == PID_POSITION_ANGLE){		//位置式PID计算
			
			if(PID->mode == PID_POSITION_ANGLE){													//位置式PID-角度环
				if(PID->error[0]>4096)	PID->error[0]=PID->error[0]-8191;							//角度环误差处理，限制在-4096 ~~ +4096
				else if(PID->error[0]<-4096)	PID->error[0]=PID->error[0]+8191;
			}
			PID->Pout = PID->Kp * PID->error[0];				//p输出
			//PID->Iout += PID->Ki * PID->error[0];				//i输出，累加																			//前后误差差值传递，0为最新差值
			PID->D_item = (PID->error[0] - PID->error[1]);	  //微分项计算
			PID->Dout = PID->Kd * PID->D_item;				  //d输出，通过微分项
			// 积分分离
			if(target == 0){
				PID->Iout = 0;
			}
			else if(fabsf(PID->error[0]) < PID->I_band || PID->I_band == 0) {
				PID->Iout += PID->Ki * PID->error[0];		//i输出，累加
				// 积分限幅
				LimitMax(PID->Iout,PID->Max_iout);																	
			}
			else {
				PID->Iout = 0;
			}
																					//i输出限制
			PID->OUT = PID->Pout + PID->Iout + PID->Dout;														//总输出
			LimitMax(PID->OUT,PID->Max_out);																				//输出限制
		}
		else if(PID->mode == PID_DELTA_SPEED){																		//增量式PID-速度环
			PID->Pout = PID->Kp * (PID->error[0] - PID->error[1]);									//p输出
			PID->Iout = PID->Ki * PID->error[0];																		//i输出
			PID->D_item = (PID->error[0] - 2.0f*PID->error[1] + PID->error[2]);			//微分项计算
			PID->Dout = PID->Kd * PID->D_item;																			//d输出，通过微分项
			PID->OUT += PID->Pout + PID->Iout + PID->Dout;													//总输出
			LimitMax(PID->OUT, PID->Max_out);																				//输出限制
		}
	}
	else{
        PID->OUT=0;																														//误差在死区内，输出为0
	}
	
	return PID->OUT;																														//返回输出值
}

// 速度目标斜坡（每周期最大步进）
int16_t limit_speed(int16_t current, int16_t target, int16_t max_step) {
	  //cha = abs(target) - abs(current);
//	  if(cha > -2000)   // 当目标是停止且相差速度差2000以下，正常减速，不要慢减速
//		{
//			return target;
//		}
//    else
//		{
			if (target > current + max_step) return current + max_step;
			if (target < current - max_step ) return current - max_step;
		//}
		return target;
}


/*
void PID_Total_Init(void)
{
	PID_Init(&Motor_pid[0],PID_POSITION_SPEED ,10,0.1,0,8000,16384,0);
	
}
*/











