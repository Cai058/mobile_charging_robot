#include "Server.h"

// 485接收到的消息
char *pbuf_server;
uint16_t len_server;
uint8_t recv_flag; // 是否接收成功
uint8_t recv_cnt = 0; // 过10ms设置recv_flag为0（保证controller能够收到recv_flag = 1的时候）

// Tx
char server_tx_buffer[SERVER_TX_BUFFER_SIZE];
uint16_t m_CurrentSendBufferLen = 0;
char sent_msg[SERVER_TX_BUFFER_SIZE];
uint8_t _tx_complete = 1;
uint16_t sent_cnt;

// 电池 SOC
uint8_t m_need_charge = 0;

// Rx
char server_rx_buffer[SERVER_RX_BUFFER_SIZE];  // 接收缓冲区
uint16_t server_rx_buffer_len = 0;          // 当前缓冲区有效数据长度

// Structure
ServerMsg_t m_server;

// Msg queue
MsgQueue txQueue;





void Server_Init(void)
{
	UART7_Config();
	ServerMsg_Init(&m_server);
	MsgQueue_Init(&txQueue);
	//send_json_response("放置完成");
}

void Server_Update(void)
{
		// 获取平台消息
		uint16_t len_server = 0;
    char *pbuf_server = get_server_rebuff(&len_server);
	
		// 获取电池SOC
		m_need_charge = Battery_ifNeedCharge();
		
		//send_json_response("调用成功");
		// 过10ms设置recv_flag = 0
		if(recv_flag == 1)
		{
			
			recv_cnt ++;
			if(recv_cnt % 2 == 0)
			{
				recv_flag = 0;	
				recv_cnt = 0;
			}
		}
		
//    
        // 将数据追加到缓冲区（防止溢出）
		if (server_rx_buffer_len + len_server < SERVER_RX_BUFFER_SIZE) {
				memcpy(&server_rx_buffer[server_rx_buffer_len], pbuf_server, len_server);
				server_rx_buffer_len += len_server;
		} else {
				printf("Error: server buffer overflow\n");
				server_rx_buffer_len = 0;
				clean_server_rebuff();  // 清除数据防止溢出
				return;
		}

		// 检测是否收到 JSON 结束符 `}`，未收到时，继续接收，存入server_rx_buffer里
		if (server_rx_buffer_len > 0 && server_rx_buffer[server_rx_buffer_len - 1] == JSON_END_CHAR) {
				server_rx_buffer[server_rx_buffer_len] = '\0';  // 确保字符串结束
				recv_flag = process_json(server_rx_buffer); // 解析 JSON
			
				/**************************************TODO***********************************************/
//				if(recv_flag == 1)
//				{
//						if(m_need_charge == 0)
//						{
//								send_json_response("Success");
//						}
//						else
//						{
//								send_json_response("Fail");
//						}
//				}
				/*****************************************************************************************/
				
				server_rx_buffer_len = 0;                   // 清空缓冲区
				clean_server_rebuff();
		}
    
		sent_cnt ++;
		_tx_complete = if_server_complete();
		if (sent_cnt % 500 == 0 ){
		if(_tx_complete && txQueue.count > 0)
		{
				MsgQueue_Pop(&txQueue, sent_msg);
				Server_SendString((uint8_t *)sent_msg,strlen(sent_msg));
		}
	}
		
}

/* 解析平台发来的json文件*/
uint8_t  process_json(const char *json_str)
{
	 cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        printf("JSON 解析失败\n");
        return 0;
    }

    // 1. 核对 JSON 是否完整（检查必须的字段）
    cJSON *robot_id = cJSON_GetObjectItem(root, "robot_id");
    cJSON *command = cJSON_GetObjectItem(root, "command");

    if (!cJSON_IsString(robot_id) || !cJSON_IsString(command)) {
        printf("JSON 数据不完整或格式错误\n");
        cJSON_Delete(root);
        return 0;
    }

    // 2. 核对机器人 ID 是否匹配
    if (strcmp(robot_id->valuestring, ROBOT_ID) != 0) {
        printf("robot_id 不匹配, 预期: %s, 实际: %s\n", ROBOT_ID, robot_id->valuestring);
        cJSON_Delete(root);
        return 0;
    }

    // 3. 获取命令编号
    if(strcmp(command->valuestring,"0") == 0)
		{
				m_server.command = 0;
		}
		else if(strcmp(command->valuestring,"1") == 0)
		{
				m_server.command = 1;
		}
		else if (strcmp(command->valuestring,"2") == 0)
		{
				m_server.command = 2;
		}
		else
		{
				printf("未知命令");
				return 0;
		}
    printf("收到命令编号: %d\n", m_server.command);
		
		
		// 4. 如果是抓取或者放置，获取take_id, give_id, chg_id, task_id
		if(m_server.command == 0 || m_server.command == 1)
		{
				// 记录 take_id 或 give_id（如果存在）
				cJSON *p_id = cJSON_GetObjectItem(root, "take_id");
			
				if(p_id != NULL)
				{
						strcpy(m_server.target_id_char,p_id->valuestring);
						m_server.target_id = parseStringToUint8((char *)m_server.target_id_char);
						printf("记录 take_id: %d\n", 	m_server.target_id);
				}
				else
				{
						p_id = cJSON_GetObjectItem(root, "give_id");
						if(p_id != NULL)
						{
								strcpy(m_server.target_id_char,p_id->valuestring);
								m_server.target_id = parseStringToUint8((char *)m_server.target_id_char);
								printf("记录 give_id: %d\n", m_server.target_id);
						}
						else
						{
								printf("take id 或 give id不正确");
								return 0;
						}
				}
				
				// 记录 chg_id（如果存在）
				p_id = cJSON_GetObjectItem(root, "chg_id");
				if(p_id != NULL)
				{
						strcpy(m_server.chg_id,p_id->valuestring);
						printf("记录 chg_id: %s\n", m_server.chg_id );
				}
				else
				{
						return 0;
				}
				// 记录 task_id (如果存在)
				p_id = cJSON_GetObjectItem(root,"task_id");
				if(p_id != NULL)
				{
					strcpy(m_server.task_id,p_id->valuestring);
					printf("记录 task_id: %s\n", m_server.task_id);
				}
				else
				{
						return 0;
				}
		}
    
		//5. 如果是充电命令，获取是否充电
		if(m_server.command == 2)
		{
				cJSON *is_charge = cJSON_GetObjectItem(root, "is_charge");
				if(is_charge != NULL)
				{
						strcpy(m_server.is_charge_char,is_charge->valuestring);
						m_server.is_charge = (uint8_t)atoi(m_server.is_charge_char);
						printf("记录 is_charge: %d\n", 	m_server.is_charge);
				}
				else
				{
						printf("is_charge错误");
						return 0;
				}
				
		}
    // 释放 JSON 对象
    cJSON_Delete(root);
		
		return 1;
}


bool send_json_response(const char *status)
{
    static char msg_buffer[256];           // msg 拼接用缓冲区

	  m_CurrentSendBufferLen = 0;
	  memset(server_tx_buffer, 0, SERVER_TX_BUFFER_SIZE);

	
    //const char *robot_id = ROBOT_ID;
    const char *success_true = "\"true";
    const char *success_false = "\"false";
    const char *success = success_true;
    const char *code = "0";
		const char *robot_sts = "0";
    const char *msg = "null";

    if (strcmp(status, "Success") == 0)
    {
        code = "20000";
        msg = "success";
    }
    else if (strcmp(status, "Fail") == 0)
    {
        code = "9999";
        snprintf(msg_buffer, sizeof(msg_buffer), "fail:robot_%s low SOC", ROBOT_ID);
        msg = msg_buffer;
    }
    else if (strcmp(status, "pick") == 0)
    {
        code = "20001";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s pick complete", ROBOT_ID);
        msg = msg_buffer;

        if (!append_string("{\"code\":\"") || !append_string(code) ||
            !append_string("\",\"robot_id\":\"") || !append_string(ROBOT_ID) ||
            !append_string("\",\"msg\":\"") || !append_string(msg) ||
            !append_string("\",\"chg_id\":\"") || !append_string(m_server.chg_id) ||
            !append_string("\",\"task_id\":\"") || !append_string(m_server.task_id) ||
            !append_string("\",\"take_id\":\"") || !append_string(m_server.target_id_char) ||
            !append_string("\",\"success\":") || !append_string(success_true) || !append_string("\"}"))
        {
            return false;
        }

        goto push_in_queue;
    }
    else if (strcmp(status, "place") == 0)
    {
        code = "20002";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s place complete", ROBOT_ID);
        msg = msg_buffer;

        if (!append_string("{\"code\":\"") || !append_string(code) ||
            !append_string("\",\"robot_id\":\"") || !append_string(ROBOT_ID) ||
            !append_string("\",\"msg\":\"") || !append_string(msg) ||
            !append_string("\",\"chg_id\":\"") || !append_string(m_server.chg_id) ||
            !append_string("\",\"task_id\":\"") || !append_string(m_server.task_id) ||
            !append_string("\",\"give_id\":\"") || !append_string(m_server.target_id_char) ||
            !append_string("\",\"success\":") || !append_string(success_true) || !append_string("\"}"))
        {
            return false;
        }

        goto push_in_queue;
    }
    else if (strcmp(status, "free") == 0)
    {
        code = "20010";
				robot_sts = "0";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s free state", ROBOT_ID);
        msg = msg_buffer;
			
				if (!append_string("{\"code\":\"") || !append_string(code) ||
						!append_string("\",\"robot_sts\":\"") || !append_string(robot_sts) ||
            !append_string("\",\"robot_id\":\"") || !append_string(ROBOT_ID) ||
            !append_string("\",\"msg\":\"") || !append_string(msg) ||
            !append_string("\",\"success\":") || !append_string(success_true) || !append_string("\"}"))
        {
            return false;
        }

        goto push_in_queue;
    }
		else if (strcmp(status, "move_pick") == 0)
    {
        code = "20011";
				robot_sts = "1";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s is moving to pick", ROBOT_ID);
        msg = msg_buffer;
			
				if (!append_string("{\"code\":\"") || !append_string(code) ||
						!append_string("\",\"robot_sts\":\"") || !append_string(robot_sts) ||
            !append_string("\",\"robot_id\":\"") || !append_string(ROBOT_ID) ||
            !append_string("\",\"msg\":\"") || !append_string(msg) ||
            !append_string("\",\"success\":") || !append_string(success_true) || !append_string("\"}"))
        {
            return false;
        }

        goto push_in_queue;
    }
		else if (strcmp(status, "move_place") == 0)
    {
        code = "20012";
				robot_sts = "2";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s is moving to place", ROBOT_ID);
        msg = msg_buffer;
			
				if (!append_string("{\"code\":\"") || !append_string(code) ||
						!append_string("\",\"robot_sts\":\"") || !append_string(robot_sts) ||
            !append_string("\",\"robot_id\":\"") || !append_string(ROBOT_ID) ||
            !append_string("\",\"msg\":\"") || !append_string(msg) ||
            !append_string("\",\"success\":") || !append_string(success_true) || !append_string("\"}"))
        {
            return false;
        }

        goto push_in_queue;
    }
		else if (strcmp(status, "move_charge") == 0)
    {
        code = "20013";
				robot_sts = "3";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s is moving to charge", ROBOT_ID);
        msg = msg_buffer;
			
				if (!append_string("{\"code\":\"") || !append_string(code) ||
						!append_string("\",\"robot_sts\":\"") || !append_string(robot_sts) ||
            !append_string("\",\"robot_id\":\"") || !append_string(ROBOT_ID) ||
            !append_string("\",\"msg\":\"") || !append_string(msg) ||
            !append_string("\",\"success\":") || !append_string(success_true) || !append_string("\"}"))
        {
            return false;
        }

        goto push_in_queue;
    }
		else if (strcmp(status, "charge") == 0)
    {
        code = "20014";
				robot_sts = "4";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s is charging", ROBOT_ID);
        msg = msg_buffer;
			
				if (!append_string("{\"code\":\"") || !append_string(code) ||
						!append_string("\",\"robot_sts\":\"") || !append_string(robot_sts) ||
            !append_string("\",\"robot_id\":\"") || !append_string(ROBOT_ID) ||
            !append_string("\",\"msg\":\"") || !append_string(msg) ||
            !append_string("\",\"success\":") || !append_string(success_true) || !append_string("\"}"))
        {
            return false;
        }

        goto push_in_queue;
    }
    else if (strcmp(status, "arrive") == 0)
    {
        code = "20003";
				snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s arrive", ROBOT_ID);
        msg = msg_buffer;
    }
//    else if (strcmp(status, "机器人充电") == 0)
//    {
//        code = "20003";
//        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s正在充电", ROBOT_ID);
//        msg = msg_buffer;
//    }
    else if (strcmp(status, "遇到障碍") == 0)
    {
        code = "20004";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s meet hints", ROBOT_ID);
        msg = msg_buffer;
        success = success_false;
    }
    else if (strcmp(status, "行走电机过载") == 0)
    {
        code = "20005";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s行走电机过载", ROBOT_ID);
        msg = msg_buffer;
        success = success_false;
    }
    else if (strcmp(status, "推送电机过载") == 0)
    {
        code = "20006";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s推送电机过载", ROBOT_ID);
        msg = msg_buffer;
        success = success_false;
    }
		else if (strcmp(status, "对接电机过载") == 0)
    {
        code = "20007";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s对接电机过载", ROBOT_ID);
        msg = msg_buffer;
        success = success_false;
    }
    else if (strcmp(status, "充电异常") == 0)
    {
        code = "20008";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s充电异常", ROBOT_ID);
        msg = msg_buffer;
        success = success_false;
    }
    else if (strcmp(status, "指令未执行") == 0)
    {
        code = "20015";
        msg = "接收指令未执行，执行异常";
        success = success_false;
    }
    else
    {
        printf("未知状态: %s\n", status);
        return false;
    }

    // 通用格式
    if (!append_string("{\"code\":\"") ||
        !append_string(code) ||
        !append_string("\",\"robot_id\":\"") ||
        !append_string(ROBOT_ID) ||
        !append_string("\",\"msg\":\"") ||
        !append_string(msg) ||
        !append_string("\",\"success\":") ||
        !append_string(success) ||
        !append_string("\"}"))
    {
        return false;
    }

push_in_queue:
    if (m_CurrentSendBufferLen < SERVER_TX_BUFFER_SIZE) {
        server_tx_buffer[m_CurrentSendBufferLen] = '\0';
			  MsgQueue_Push(&txQueue, (char*)server_tx_buffer);
        //Server_SendString((uint8_t *)server_tx_buffer,m_CurrentSendBufferLen);
        return true;
    } else {
        return false;
    }
}

bool append_string(const char *str)
{
    if (!str) return false;
    size_t len = strlen(str);
    if (m_CurrentSendBufferLen + len >= SERVER_TX_BUFFER_SIZE)
        return false;

    memcpy(&server_tx_buffer[m_CurrentSendBufferLen], str, len);
    m_CurrentSendBufferLen += len;
    return true;
}

/* 将字符串转化为uint8_t*/
uint8_t parseStringToUint8(const char *str) {
    if (str == NULL) {
        return 0; // 防止空指针
    }

    const char *ptr = str;
    int result = 0;

    // 找到 '-' 位置
    while (*ptr && *ptr != '-') {
        ptr++;
    }

    // 如果找到 '-'，解析 '-' 之后的部分
    if (*ptr == '-' && *(ptr + 1) != '\0') {
        ptr++;  // 移动到 '-' 之后
    } else {
        ptr = str; // 如果没有 '-', 解析整个字符串
    }

    // 逐个字符转换为整数
    while (*ptr) {
        if (*ptr >= '0' && *ptr <= '9') {
            result = result * 10 + (*ptr - '0');
            if (result > 255) { // 确保不超过 uint8_t 最大值
                return 255;
            }
        } else {
            break; // 非数字字符停止解析
        }
        ptr++;
    }

    return (uint8_t)result;
}

void ServerMsg_Init(ServerMsg_t *msg)
{
    if (msg == NULL) return;

    msg->target_id = 0;
    msg->command = -1;
    msg->is_charge = 0;

    msg->target_id_char[0] = '\0';
    msg->is_charge_char[0] = '\0';
    msg->chg_id[0] = '\0';
    msg->task_id[0] = '\0';
}


ServerMsg_t Get_serverMsg(void)
{
		return m_server;
}

void MsgQueue_Init(MsgQueue* q) {
    q->head = 0;
    q->tail = 0;
    q->count = 0;
}

bool MsgQueue_Push(MsgQueue* q, const char* msg) {
    if (q->count >= SERVER_TX_BUFFER_NUM) return false; // overflow queue

    snprintf(q->messages[q->tail], SERVER_TX_BUFFER_SIZE, "%s", msg);
    q->tail = (q->tail + 1) % SERVER_TX_BUFFER_NUM;
    q->count++;
    return true;
}

bool MsgQueue_Pop(MsgQueue* q, char* out_msg) { 
    if (q->count == 0 || _tx_complete == 0) return false; // ???

    snprintf(out_msg, SERVER_TX_BUFFER_SIZE, "%s", q->messages[q->head]);
    q->head = (q->head + 1) % SERVER_TX_BUFFER_NUM;
    q->count--;
		
		//Server_SendString((uint8_t *)out_msg,strlen(out_msg));
    return true;
}

uint8_t Get_ifaction(void)
{
		return recv_flag;
}
