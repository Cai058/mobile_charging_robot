#include "RFID.h"

char *pbuf_rfid;
uint16_t rfid_len;
uint16_t m_number = 0; 
uint16_t rfid_code;
uint32_t rfid_tx_cnt = 0;

uint8_t rfid_rx_lock_flag = 0;
uint8_t restart_flag = 0;

#define RFID_CMD_LEN 4
//#define RFID_CMD_LEN 5
#define RFID_RX_BUF_LEN 64

uint8_t rfid_cmd[RFID_CMD_LEN] = {0x45, 0x46, 0x23, 0x0D}; // EF1 ����
//uint8_t rfid_cmd[RFID_CMD_LEN] = {0x45, 0x46, 0x31, 0xBC, 0x03};

void RFID_Config(void)
{
	BSP_RFID_Config();
}

//void RFID_Update(void)
//{
//			
//			pbuf_rfid = get_rfid_rebuff(&rfid_len);
//	
//			
//	    rfid_tx_cnt++;
//	
//			if(rfid_tx_cnt == 200)
//			{
//			RFID_SendCommand();
//			rfid_tx_cnt = 0;
//			}
//   if(pbuf_rfid != NULL && rfid_len > 0)
//	 {
//			m_number = ExtractRFIDNumber((uint8_t *)pbuf_rfid,rfid_len);
//		  clean_rfid_rebuff();
//	 }
//}

void RFID_Update(void)
{
			rfid_rx_lock_flag = rfid_if_rx_lock((uint8_t *)pbuf_rfid);
	    if(rfid_rx_lock_flag == 1)
			{
					RFID_DMA_Rx_ReStart();
				  restart_flag  = 1;
				  //rfid_rx_lock_flag = 0;
			}
	    uint8_t _rx_complete = get_rfid_rx_complete();
			if(_rx_complete)
			{
					pbuf_rfid = get_rfid_rebuff(&rfid_len);
					if(pbuf_rfid != NULL && rfid_len > 0)
					{
							m_number = ExtractRFIDNumber((uint8_t *)pbuf_rfid,rfid_len);
							clean_rfid_rebuff();
					}
					//RFID_DMA_Rx_ReStart();
			}
	    rfid_tx_cnt++;
	
			if(rfid_tx_cnt == 200)
			{
			RFID_SendCommand();
			rfid_tx_cnt = 0;
			}
   
}

uint8_t Get_Number(void)
{
	return m_number;
}

void RFID_SendCommand(void)
{
		RFID_SendStr(rfid_cmd, RFID_CMD_LEN);
}

// ���������ݣ���ȡ UID��
void RFID_ProcessResponse(uint8_t *data, uint16_t len) {
    if (len < 10) return;

    if (data[0] == '0') {  // Status = 0����ʾ�ɹ�
        //printf("Channel: %c\n", data[1]);

        // UID ͨ���� data[2] ��ʼ��8�ֽ�
        printf("UID: ");
        for (int i = 2; i < 10; i++) {
            printf("%02X ", data[i]);
        }
        printf("\n");
    } else {
        printf("Error Status: %c\n", data[0]);
    }
}

//uint8_t RFID2Number(uint16_t rfid)
//{
//		uint8_t rfid_number = 0;
//		switch(rfid){
//			case 
//		}
//		return rfid_number;
//}

uint8_t rfid_if_rx_lock(const uint8_t *data)
{
		if(data[0] == 0x00){return 1;}
		else{return 0;}
}
uint16_t ExtractRFIDNumber(const uint8_t *data, uint16_t len)
{
	  uint8_t rfid_number;
    if (len >= 9) {
        rfid_code =  ((uint16_t)data[7] << 8) | data[8];
    } else {
        rfid_code =  0xFFFF; // ????????
    }
		
		switch(rfid_code){
			
		case 0x149F:
			//case 0xBA9B:
        rfid_number = 1;
        break;
    case 0x4409:
			//case 0x9A22:
        rfid_number = 2;
        break;
    case 0x4D6F:
        rfid_number = 3;
        break;
    case 0x4FEE:
        rfid_number = 4;
        break;
    case 0x0E7F:
        rfid_number = 5;
        break;
    case 0x1980:
        rfid_number = 6;
        break;
    case 0x4FEA:
        rfid_number = 7;
        break;
    case 0x1364:
        rfid_number = 8;
        break;
    case 0x1842:
        rfid_number = 9;
        break;
    case 0x197A:
        rfid_number = 10;
        break;
    case 0xF4FA:
        rfid_number = 11;
        break;
    case 0x4D7B:
        rfid_number = 12;
        break;
    case 0x171D:
        rfid_number = 13;
        break;
    case 0x184C:
        rfid_number = 14;
        break;
    case 0x1843:
        rfid_number = 15;
        break;
    case 0x1978:
        rfid_number = 16;
        break;
    case 0x512C:
        rfid_number = 17;
        break;
    case 0x5256:
        rfid_number = 18;
        break;
    
//    // Mid 
//    case 0x3F6B:
//        rfid_number = 19;
//        break;
//    case 0x2900:
//        rfid_number = 20;
//        break;
//    case 0x30CA:
//        rfid_number = 21;
//        break;

//    case 0xBA95:
//        rfid_number = 1;
//        break;
//    case 0xB14E:
//        rfid_number = 2;
//        break;
//    case 0xB4CE:
//        rfid_number = 3;
//        break;
			
			default:
				rfid_number = 0;
		}
		
		return rfid_number;
}

