#ifndef __BSP_RGB_H
#define	__BSP_RGB_H

#include "stm32f4xx.h"

//引脚定义
/*******************************************************/


typedef enum{
		RED,
		GREEN,
		YELLOW,
		CYAN
}RGB_COLOR;

void RGB_ON(RGB_COLOR color);
void RGB_OFF(void);



#endif     
