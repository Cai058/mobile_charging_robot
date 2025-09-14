#include "RC.h"
#include "bsp_debug_usart.h"

char *pbuf_rc;
uint16_t len_rc;
uint16_t rc_cnt = 0;

RC_raw_t m_rc_raw;

void RC_Init(void)
{
	bsp_rc_Config();
}

void RC_Update(void)
{
	pbuf_rc = get_rc_rebuff(&len_rc);
if (pbuf_rc != NULL && len_rc > 0)
{
	
	SBUS_TO_RC((uint8_t *)pbuf_rc, &m_rc_raw); 
    clean_rc_rebuff(); // 清除数据，等待下一帧
	rc_cnt = 0;
}
else  // 当遥控器关闭时，将遥控器数据清零
{
	if(rc_cnt > 2000)  // 因为正常接收中间也会有数据为0的时候，所以等待1s后再清零
	{
		memset(&m_rc_raw, 0, sizeof(RC_raw_t));
	}
	else
	{
		rc_cnt++;
	}
}

}



static void SBUS_TO_RC(volatile const uint8_t *sbus_buf, RC_raw_t *rc_raw)
{
	if (sbus_buf == NULL || rc_raw == NULL)
		return;

	rc_raw->rc.ch[0] = (sbus_buf[0] | (sbus_buf[1] << 8)) & 0x07ff;
	rc_raw->rc.ch[1] = ((sbus_buf[1] >> 3) | (sbus_buf[2] << 5)) & 0x07ff;
	rc_raw->rc.ch[2] = ((sbus_buf[2] >> 6) | (sbus_buf[3] << 2) |
						(sbus_buf[4] << 10)) & 0x07ff;
	rc_raw->rc.ch[3] = ((sbus_buf[4] >> 1) | (sbus_buf[5] << 7)) & 0x07ff;
	rc_raw->rc.s[0] = ((sbus_buf[5] >> 4) & 0x0003);
	rc_raw->rc.s[1] = ((sbus_buf[5] >> 4) & 0x000C) >> 2;
	rc_raw->mouse.x = sbus_buf[6] | (sbus_buf[7] << 8);
	rc_raw->mouse.y = sbus_buf[8] | (sbus_buf[9] << 8);
	rc_raw->mouse.z = sbus_buf[10] | (sbus_buf[11] << 8);
	rc_raw->mouse.press_l = sbus_buf[12];
	rc_raw->mouse.press_r = sbus_buf[13];
	rc_raw->key.v = sbus_buf[14] | (sbus_buf[15] << 8);
	rc_raw->rc.ch[4] = ((sbus_buf[16]) | (sbus_buf[17] << 8)) & 0x07ff;

	for (int i = 0; i < 5; ++i)
		rc_raw->rc.ch[i] -= RC_CH_VALUE_OFFSET;
}

RC_raw_t RC_GetData(void)
{
    return m_rc_raw;
}

char RC_GetMode(void)
{
	  if(m_rc_raw.rc.s[1] == 0){   // if the remove is off, then default mode 1
			return 1;
		}
		
		return m_rc_raw.rc.s[1];
}

char RC_GetDirection(void)
{
	 if(m_rc_raw.rc.s[0] == 0)
	 {
			return 3;
	 }
		return m_rc_raw.rc.s[0];
}
