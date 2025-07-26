#ifndef BSP_RC_H_
#define BSP_RC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

//#define RC_CH_VALUE_MIN ((uint16_t)364)
//#define RC_CH_VALUE_OFFSET ((uint16_t)1024)
//#define RC_CH_VALUE_MAX ((uint16_t)1684)
//#define RC_CH_OFFSET_MAX ((uint16_t)660)

//typedef __packed struct
//{
//    __packed struct
//    {
//        int16_t ch[5];
//        char s[2];
//    } rc;
//    __packed struct
//    {
//        int16_t x;
//        int16_t y;
//        int16_t z;
//        uint8_t press_l;
//        uint8_t press_r;
//    } mouse;
//    __packed struct
//    {
//        uint16_t v;
//    } key;

//} RC_raw_t;

//RC_raw_t* get_remote_control_raw(void);

void bsp_rc_Config(void);
void bsp_rc_disable(void);
void bsp_rc_enable(void);
void bsp_rc_restart(uint16_t dma_buf_num);
uint8_t bsp_read_rc_update_flag(void);

char *get_rc_rebuff(uint16_t *len);
void clean_rc_rebuff(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_RC_H_ */
