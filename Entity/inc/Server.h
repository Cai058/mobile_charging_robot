#ifndef SERVER_H
#define	SERVER_H

#include "bsp_485_server.h"
#include "Battery.h"
#include "cJSON.h"

#include "usart_callback.h"
#include "stdlib.h"
#include <stdbool.h>

#define ROBOT_ID   "010"

#define SERVER_RX_BUFFER_SIZE 256   // 缓冲区大小
#define JSON_END_CHAR '}'       // JSON 结束标志
#define SERVER_TX_BUFFER_SIZE 256
#define SERVER_TX_BUFFER_NUM 10 

extern char *pbuf_server;
extern uint16_t len_server;
extern cJSON *root;

// 传给controller
typedef struct{
	// 经过处理后得到的uint8信息，方便之后控制处理
	uint8_t target_id;
	//uint8_t give_id;
	int command;
	
	// 从server发来的msg记录的char信息
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

uint8_t process_json(const char *json_str);   //处理接收到的json文件
bool send_json_response(const char *status,uint8_t _avaiable);  //发送json文件
uint8_t parseStringToUint8(const char *str);  //处理：将string变成uint8
bool append_string(const char *str);          //用于拼接字符串

bool MsgQueue_Push(MsgQueue* q, const char* msg); // Push in queue
bool MsgQueue_Pop(MsgQueue* q, char* out_msg);
//void MsgQueue_PeekProtectMode(MsgQueue* q);

ServerMsg_t Get_serverMsg(void);              //返回信息
uint8_t Get_ifaction(void);                   //返回是否收到消息

#endif
