#include "first_order_filter.h"

void VxFilter_Init(VxFilter_t *f, float tau1, float tau2, float tau3, float period)
{
    Filter_Init(&f->stage1,tau1,period);

    Filter_Init(&f->stage2,tau2,period);
	
    Filter_Init(&f->stage3,tau3,period);
}

void VxFilter_Update(int16_t input_speed, VxFilter_t *f)
{
    // ???
    Filter_Update(input_speed, &f->stage1);

    // ???
    Filter_Update((int16_t)f->stage1.output_speed, &f->stage2);

    // ???
    Filter_Update((int16_t)f->stage2.output_speed, &f->stage3);
}

int16_t VxFilter_GetResult(VxFilter_t *f)
{
    float temp = f->stage3.output_speed;

    if (temp > 32767.0f) temp = 32767.0f;
    else if (temp < -32768.0f) temp = -32768.0f;

    return (int16_t)(temp + (temp >= 0 ? 0.5f : -0.5f));
}

void Filter_Init(filter_t *m_filter,float tau, float period)
{
		m_filter->last_tick = Get_tick();
	  m_filter->tau = tau;
	  m_filter->period = period * 0.001f;
}

void Filter_Update(int16_t input_speed,filter_t *m_filter)
{
	  m_filter->current_tick = Get_tick();
		m_filter->period = (m_filter->current_tick - m_filter->last_tick)*0.001f;
	  m_filter->last_tick = m_filter->current_tick;
	  float a = m_filter->period / (m_filter->tau + m_filter->period);
	  m_filter->output_speed = (1-a) * m_filter->output_speed + a * input_speed;
}
