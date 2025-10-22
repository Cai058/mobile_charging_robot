#include "Battery.h"
#include "main.h"

char *pbuf_battery;
uint16_t len_battery;
uint16_t battery_cnt = 0; // 用于计时，每2s发送读取一次
uint8_t battery_rx_lock_flag = 0;
uint8_t battery_restart_flag = 0;

static uint8_t tx_buffer[256];  // 发送缓存区（包含CRC）

BatteryInfo_t battery_info;

void Battery_Init(void)
{
		BSP_Battery_Config();
		Battery_t_init();
}

void Battery_Update(void)
{
		battery_rx_lock_flag = battery_if_rx_lock((uint8_t *)pbuf_battery);
	  if(battery_rx_lock_flag == 1)
		{
				BATTERY_DMA_Rx_ReStart();
			  battery_restart_flag = 1;
		}
		
		uint8_t _battery_rx_complete = get_battery_rx_complete();
		if(_battery_rx_complete)
		{
				pbuf_battery = get_battery_rebuff(&len_battery);
			  if (pbuf_battery != NULL && len_battery > 0)
				{
						Battery_ParseFrame((uint8_t *)pbuf_battery, len_battery);
						clean_battery_rebuff(); // 清除数据，等待下一帧
				}
		}
		battery_cnt ++;
		if(battery_cnt == 1000)
		{
				Battery_SendRequest(2);
		}
		else if(battery_cnt == 2000)
		{
				battery_cnt = 0;
				Battery_SendRequest(1);
		}

}

uint8_t battery_if_rx_lock(const uint8_t *data)
{
		if(data[0] == 0x00){return 1;}
		else{return 0;}
}

static uint16_t Modbus_CRC16(uint8_t *data, uint8_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

//static uint8_t req_buf[8];

void battery_uart_send_frame(uint8_t *data, uint8_t len_no_crc)
{
    if (len_no_crc + 2 > sizeof(tx_buffer)) {
        printf("发送数据过长，超过缓冲区！\r\n");
        return;
    }

    // 拷贝内容到内部 buffer
    memcpy(tx_buffer, data, len_no_crc);

    // 计算并添加 CRC16
    uint16_t crc = Modbus_CRC16(tx_buffer, len_no_crc);
    tx_buffer[len_no_crc] = crc & 0xFF;
    tx_buffer[len_no_crc + 1] = (crc >> 8) & 0xFF;

    uint8_t total_len = len_no_crc + 2;

    // ? 打印调试信息
//    printf("Sending frame (%d bytes):", total_len);
//    for (uint8_t i = 0; i < total_len; i++) {
//        printf(" %02X", tx_buffer[i]);
//    }
//    printf("\r\n");
//		
		battery_SendString(tx_buffer,total_len);
		
}

void Battery_SendRequest(uint16_t num)
{
    uint8_t frame1[] = {
        0x01,  // 从机地址
        0x03,  // 功能码
        0x00, 0x13,  // 起始寄存器地址
        0x00, 0x02   // 读取寄存器数量
    };
		
		uint8_t frame2[] = {
        0x01,  // 从机地址
        0x03,  // 功能码
        0x00, 0x28,  // 起始寄存器地址
        0x00, 0x01   // 读取寄存器数量
    };

		if(num == 1)battery_uart_send_frame(frame1, sizeof(frame1)); // 自动附加CRC并发送
		else if(num == 2)battery_uart_send_frame(frame2, sizeof(frame2));
}



void Battery_ParseFrame(const uint8_t *data, uint16_t len)
{
    if (len < 5 || data[0] != 0x01 || data[1] != 0x03) return;

    // 电流+容量帧：长度为 9，data length = 4
    if (len >= 9 && data[2] == 0x04)
    {
        int16_t raw_current = (int16_t)((data[3] << 8) | data[4]);
        uint16_t raw_capacity = (data[5] << 8) | data[6];

        battery_info.current_A = raw_current / 100.0f;     // 10mA -> A
        battery_info.capacity_Ah = raw_capacity / 100.0f;  // 10mAh -> Ah

        //printf("Current: %.2f A\n", battery_info.current_A);
        //printf("Capacity: %.2f Ah\n", battery_info.capacity_Ah);
    }

    // RSOC帧：长度为 7，data length = 2
    else if (len >= 7 && data[2] == 0x02)
    {
        battery_info.soc = data[4]; // 高字节预留，低字节才是有效值
        printf("RSOC: %d %%\n", battery_info.soc);
    }
}

void Battery_t_init(void)
{
		battery_info.capacity_Ah = 0;
		battery_info.current_A = 0;
		battery_info.soc = 100;
}

uint8_t Battery_ifNeedCharge(void)  // test!!!!!!!!!!!!!!!!!!!!!!! 85?
{
		if(battery_info.soc < NeedCharge_Threshold) return 1;
		
	return 0;
}

uint8_t Battery_isCharging(void)
{
		if(battery_info.current_A > 0) return 1;
		else return 0;
}

uint8_t Get_SOC(void)
{
		return battery_info.soc;
}

float Get_current_A(void)
{
        return battery_info.current_A;
}