#ifndef RC_H
#define	RC_H

#include "bsp_rc.h"
#include "stdlib.h"

#define RC_BUFFER_SIZE 512  // »º³åÇø´óÐ¡

extern char *pbuf_rc;
extern uint16_t len_rc;

#define RC_CH_VALUE_MIN ((uint16_t)364)
#define RC_CH_VALUE_OFFSET ((uint16_t)1024)
#define RC_CH_VALUE_MAX ((uint16_t)1684)
#define RC_CH_OFFSET_MAX ((uint16_t)660)

typedef __packed struct
{
    __packed struct
    {
        int16_t ch[5];
        char s[2];
    } rc;
    __packed struct
    {
        int16_t x;
        int16_t y;
        int16_t z;
        uint8_t press_l;
        uint8_t press_r;
    } mouse;
    __packed struct
    {
        uint16_t v;
    } key;

} RC_raw_t;

void RC_Init(void);
void RC_Update(void);

RC_raw_t RC_GetData(void);
char RC_GetMode(void); 
char RC_GetDirection(void);
static void SBUS_TO_RC(volatile const uint8_t *sbus_buf, RC_raw_t *rc_raw);

#endif
