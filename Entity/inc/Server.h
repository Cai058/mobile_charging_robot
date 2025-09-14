#ifndef SERVER_H
#define	SERVER_H

#include "bsp_485_server.h"
#include "Battery.h"
#include "cJSON.h"

#include "usart_callback.h"
#include "stdlib.h"
#include <stdbool.h>

#define ROBOT_ID   "010"

#define SERVER_RX_BUFFER_SIZE 256   // ��������С
#define JSON_END_CHAR '}'       // JSON ������־
#define SERVER_TX_BUFFER_SIZE 256
#define SERVER_TX_BUFFER_NUM 10 

extern char *pbuf_server;
extern uint16_t len_server;
extern cJSON *root;

// ����controller
typedef struct{
	// ���������õ���uint8��Ϣ������֮����ƴ���
	uint8_t target_id;
	//uint8_t give_id;
	int command;
	
	// ��server������msg��¼��char��Ϣ
	char target_id_char[8];
	char chg_id[15];
	char task_id[8];
}ServerMsg_t;

// tx queue
typedef struct {
    char messages[SERVER_TX_BUFFER_NUM][SERVER_TX_BUFFER_SIZE]; 
    uint8_t head;  
    uint8_t tail;  
    uint8_t count;
} MsgQueue;

void Server_Update(void);
void Server_Init(void);

void ServerMsg_Init(ServerMsg_t *msg);
void MsgQueue_Init(MsgQueue* q);

uint8_t process_json(const char *json_str);   //������յ���json�ļ�
bool send_json_response(const char *status,uint8_t _avaiable);  //����json�ļ�
uint8_t parseStringToUint8(const char *str);  //������string���uint8
bool append_string(const char *str);          //����ƴ���ַ���

bool MsgQueue_Push(MsgQueue* q, const char* msg); // Push in queue
bool MsgQueue_Pop(MsgQueue* q, char* out_msg);
//void MsgQueue_PeekProtectMode(MsgQueue* q);

ServerMsg_t Get_serverMsg(void);              //������Ϣ
uint8_t Get_ifaction(void);                   //�����Ƿ��յ���Ϣ

#endif
