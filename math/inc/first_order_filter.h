#ifndef FIRST_ORDER_FILTER_H_
#define FIRST_ORDER_FILTER_H_

#include <stdint.h>
#include <math.h>
#include "main.h"

typedef struct{
uint32_t last_tick;
uint32_t current_tick;
float period;
float tau;
float output_speed;
}filter_t;

typedef struct {
    filter_t stage1;
    filter_t stage2;
    filter_t stage3;
} VxFilter_t;

void VxFilter_Init(VxFilter_t *f, float tau1, float tau2, float tau3, float period);
void VxFilter_Update(int16_t input_speed, VxFilter_t *f);
int16_t VxFilter_GetResult(VxFilter_t *f);

void Filter_Init(filter_t *m_filter,float tau, float period);
void Filter_Update(int16_t input_speed,filter_t *m_filter);


#endif
