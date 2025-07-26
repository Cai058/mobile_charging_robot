#ifndef __test_H
#define __test_H
#include "stm32f4xx.h"
#include <stdbool.h>

#define PUSH_TIM           				TIM4
#define PUSH_TIM_CLK_ENABLE()  			__TIM4_CLK_ENABLE()

#define PUSH_IN1_Pin GPIO_PIN_0
#define PUSH_IN1_Port GPIOB
#define PUSH_IN2_Pin GPIO_PIN_1
#define PUSH_IN2_Port GPIOB
#define PUSH_ENABLEA_PWM_Pin GPIO_PIN_6  // TIM3_CH1
#define PUSH_ENABLEA_PWM_Port GPIOB

#define PUSH_IN3_Pin GPIO_PIN_10
#define PUSH_IN3_Port GPIOB
#define PUSH_IN4_Pin GPIO_PIN_11
#define PUSH_IN4_Port GPIOB
#define PUSH_ENABLEB_PWM_Pin GPIO_PIN_7  // TIM3_CH2
#define PUSH_ENABLEB_PWM_Port GPIOB

#define PUSH_GPIO_CLK_ENABLE() __GPIOB_CLK_ENABLE()

#define PUSH_PWM_CHANNEL1 TIM_CHANNEL_1
#define PUSH_PWM_CHANNEL2 TIM_CHANNEL_2

#define PUSH_TIME 4000
//#define GRAB_IN1_Pin GPIO_PIN_2
//#define GRAB_IN1_Port GPIOB
//#define GRAB_IN2_Pin GPIO_PIN_3
//#define GRAB_IN2_Port GPIOB
//#define GRAB_ENABLE_PWM_Pin GPIO_PIN_7  // TIM4_CH2

// /* TIM8通道1输出引脚 */
// #define ADVANCE_OCPWM_PIN           		GPIO_PIN_4            //PA4输出PWM波到l298n的ENA端
// #define ADVANCE_OCPWM_GPIO_PORT     		GPIOC                      
// #define ADVANCE_OCPWM_GPIO_CLK_ENABLE() 	__GPIOC_CLK_ENABLE()
// //#define ADVANCE_OCPWM_AF					GPIO_AF3_TIM8

// //通过控制IN1,IN2的高低电平控制电机旋转的方向
// #define IN1_PIN            		GPIO_PIN_9              //IN1_PIN
// #define IN1_GPIO_PORT      		GPIOA                      
// #define IN1_GPIO_CLK_ENABLE()	__GPIOA_CLK_ENABLE()


// #define IN2_PIN            		GPIO_PIN_10              //IN2_PIN
// #define IN2_GPIO_PORT      		GPIOA                      
// #define IN2_GPIO_CLK_ENABLE()	__GPIOA_CLK_ENABLE()





// #define SWITCH1_INT_EXTI_IRQ 								EXTI3_IRQn
// #define SWITCH1_IRQHandler 									EXTI3_IRQHandler
// #define SWITCH2_INT_EXTI_IRQ 								EXTI4_IRQn
// #define SWITCH2_IRQHandler 									EXTI4_IRQHandler


extern TIM_HandleTypeDef TIM_TimeBaseStructure;

void l298n_GPIO_Config(void);
void TIM_Mode_Config(void);
void L298N_Config(void);


void PushRod_Forward(void);
void PushRod_Backward(void);
void PushRod_StartCharge(void);
void PushRod_StopCharge(void);
void PushRod_Stop(void);
int Check_Limit_Switch(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);



#endif
