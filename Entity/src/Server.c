#include "Server.h"

// RobotConfig_t RobotConfig_server;

// 485接收到的消息
char *pbuf_server;
uint16_t len_server;
uint8_t recv_flag; // 是否接收成功
uint16_t recv_cnt = 0; // 过10ms设置recv_flag为0（保证controller能够收到recv_flag = 1的时候）

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

// // 在每次发送取放桩完成后过3s才可继续发消息，防止收发冲突
// uint8_t protect_flag = 0;
// uint32_t protect_cnt = 0;
// uint8_t protect_mode = 0; // 0: protect_flag无效，对于取放桩完成而言 1: protect_flag有效，对于其它反馈


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
			if(recv_cnt % 1000 == 0)
			{
				recv_flag = 0;	
				recv_cnt = 0;
			}
		}
		
        // if(protect_flag == 1){
        //     protect_cnt ++;
        //     if(protect_cnt % 6000 == 0){
        //         protect_flag = 0;
        //         protect_cnt = 0;
        //     }
        // }
//    
        // 将数据追加到缓冲区（防止溢出）
		if (server_rx_buffer_len + len_server < SERVER_RX_BUFFER_SIZE) {
				memcpy(&server_rx_buffer[server_rx_buffer_len], pbuf_server, len_server);
				server_rx_buffer_len += len_server;
		} else {
				//printf("Error: server buffer overflow\n");
				server_rx_buffer_len = 0;
				clean_server_rebuff();  // 清除数据防止溢出
				return;
		}

		// 检测是否收到 JSON 结束符 `}`，未收到时，继续接收，存入server_rx_buffer里
		if (server_rx_buffer_len > 0 && server_rx_buffer[server_rx_buffer_len - 1] == JSON_END_CHAR) {
				server_rx_buffer[server_rx_buffer_len] = '\0';  // 确保字符串结束
				recv_flag = process_json(server_rx_buffer); // 解析 JSON*/
				
				server_rx_buffer_len = 0;                   // 清空缓冲区
				clean_server_rebuff();
		}
    
		sent_cnt ++;                                       //控制每条信息发送速率不要太快
		_tx_complete = if_server_complete();               //查询上次发送是否完成
		//MsgQueue_PeekProtectMode(&txQueue);
		//if (sent_cnt % 1000 == 0  && (protect_flag == 0 || protect_mode == 0)){       //server_rx_buffer_len在接收完整数据之后才会清0，在接收之后10ms才可发送消息
		if (sent_cnt % 1000 == 0){
            if(_tx_complete && txQueue.count > 0)
		{
			MsgQueue_Pop(&txQueue, sent_msg);
			Server_SendString((uint8_t *)sent_msg,strlen(sent_msg));
		}
	}
		
}

/* 解析平台发来的json文件*/
uint8_t process_json(const char *json_str)
{
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        //printf("JSON 解析失败\n");
        return 0;
    }

    // 1. 核对 JSON 是否完整（检查必须的字段）
    cJSON *robot_id = cJSON_GetObjectItem(root, "robot_id");
    cJSON *command = cJSON_GetObjectItem(root, "command");

    if (!cJSON_IsString(robot_id) || !cJSON_IsString(command)) {
        //printf("JSON 数据不完整或格式错误\n");
        cJSON_Delete(root);
        return 0;
    }

    // 2. 核对机器人 ID 是否匹配
    if (strcmp(robot_id->valuestring, Robot_ID) != 0) {
        //printf("robot_id 不匹配, 预期: %s, 实际: %s\n", Robot_ID, robot_id->valuestring);
        cJSON_Delete(root);
        return 0;
    }

    // 3. 获取命令编号
    if (strcmp(command->valuestring, "0") == 0) {
        m_server.command = 0;
    }
    else if (strcmp(command->valuestring, "1") == 0) {
        m_server.command = 1;
    }
    else if (strcmp(command->valuestring, "3") == 0) {
        m_server.command = 3;
    }
    else {
        //printf("未知命令");
        cJSON_Delete(root); // 记得释放内存
        return 0;
    }
    //printf("收到命令编号: %d\n", m_server.command);

    // 4. 解析具体参数 (chg_id, task_id, take_id, give_id)
    // 只要是 0(抓), 1(放), 3(抓+放) 都进入此逻辑
    if (m_server.command == 0 || m_server.command == 1 || m_server.command == 3)
    {
        cJSON *p_id = NULL;

        // --- A. 公共参数解析 (chg_id 和 task_id) ---
        
        // 记录 chg_id
        p_id = cJSON_GetObjectItem(root, "chg_id");
        if (p_id != NULL) {
            strcpy(m_server.chg_id, p_id->valuestring);
        } else {
            cJSON_Delete(root);
            return 0; // 缺少 chg_id
        }

        // 记录 task_id
        p_id = cJSON_GetObjectItem(root, "task_id");
        if (p_id != NULL) {
            strcpy(m_server.task_id, p_id->valuestring);
        } else {
            cJSON_Delete(root);
            return 0; // 缺少 task_id
        }

        // --- B. 按需解析 take_id (命令为 0 或 3 时需要) ---
        if (m_server.command == 0 || m_server.command == 3)
        {
            p_id = cJSON_GetObjectItem(root, "take_id");
            if (p_id != NULL) {
                strcpy(m_server.take_id_char, p_id->valuestring);
                m_server.take_id = parseStringToUint8((char *)m_server.take_id_char);
            } else {
                cJSON_Delete(root);
                return 0; // 命令包含抓取但缺少 take_id
            }
        }

        // --- C. 按需解析 give_id (命令为 1 或 3 时需要) ---
        if (m_server.command == 1 || m_server.command == 3)
        {
            p_id = cJSON_GetObjectItem(root, "give_id");
            if (p_id != NULL) {
                strcpy(m_server.give_id_char, p_id->valuestring);
                m_server.give_id = parseStringToUint8((char *)m_server.give_id_char);
            } else {
                cJSON_Delete(root);
                return 0; // 命令包含放置但缺少 give_id
            }
        }
    }

    // 释放 JSON 对象
    cJSON_Delete(root);
    return 1;
}


bool send_json_response(const char *status,uint8_t _avaiable,uint8_t _location, uint8_t _soc,float _current,uint8_t _charge_num)
{
    static char msg_buffer[256];           // msg 缓冲区
	  static char location_buffer[8];
		static char soc_buffer[8];
        static char current_buffer[8];
        static char charge_num_buffer[8];

	  m_CurrentSendBufferLen = 0;
	  memset(server_tx_buffer, 0, SERVER_TX_BUFFER_SIZE);
		
	
    //const char *robot_id = Robot_ID;
    const char *success_true = "\"true";
    const char *success_false = "\"false";
    const char *success = success_true;
    const char *code = "0";
	const char *robot_sts = "0";
    const char *msg = "null";
	const char *loca = "null";
	const char *soc = "null";
    const char *current = "null";
    const char *charge_num = "null";

		snprintf(location_buffer, sizeof(location_buffer), "%u", _location);
	  loca = location_buffer;
	  snprintf(soc_buffer, sizeof(soc_buffer), "%u", _soc);
		soc = soc_buffer;
        snprintf(current_buffer, sizeof(current_buffer), "%.2f", _current);
		current = current_buffer;
        snprintf(charge_num_buffer, sizeof(charge_num_buffer), "%u", _charge_num);
        charge_num = charge_num_buffer;

    if (strcmp(status, "Success") == 0)
    {
        code = "20000";
        msg = "success";
        //protect_mode = 1;
    }
    else if (strcmp(status, "Fail") == 0)
    {
        code = "9999";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s fail", Robot_ID);
        msg = msg_buffer;
        //protect_mode = 1;
    }
    else if (strcmp(status, "pick") == 0)
    {
        code = "20001";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s pick complete", Robot_ID);
        msg = msg_buffer;

        if (!append_string("{\"code\":\"") || !append_string(code) ||
            !append_string("\",\"robot_id\":\"") || !append_string(Robot_ID) ||
            !append_string("\",\"msg\":\"") || !append_string(msg) ||
				    !append_string("\",\"task_id\":\"") || !append_string(m_server.task_id) ||
            !append_string("\",\"chg_id\":\"") || !append_string(m_server.chg_id) ||
            !append_string("\",\"take_id\":\"") || !append_string(m_server.take_id_char) ||
            !append_string("\",\"available\":\"") || !append_string(_avaiable ? "1" : "0")||
            !append_string("\",\"location_id\":\"") || !append_string(loca)||
            !append_string("\",\"SOC\":\"") || !append_string(soc)||
            !append_string("\",\"success\":") || !append_string(success_true) || !append_string("\"}"))
        {
            return false;
        }

        // protect_flag = 1;
        // protect_cnt = 0;
        //protect_mode = 0;
        goto push_in_queue;
    }
    else if (strcmp(status, "place") == 0)
    {
        code = "20002";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s place complete", Robot_ID);
        msg = msg_buffer;

        if (!append_string("{\"code\":\"") || !append_string(code) ||
            !append_string("\",\"robot_id\":\"") || !append_string(Robot_ID) ||
            !append_string("\",\"msg\":\"") || !append_string(msg) ||
            !append_string("\",\"chg_id\":\"") || !append_string(m_server.chg_id) ||
            !append_string("\",\"task_id\":\"") || !append_string(m_server.task_id) ||
            !append_string("\",\"give_id\":\"") || !append_string(m_server.give_id_char) ||
            !append_string("\",\"available\":\"") || !append_string(_avaiable ? "1" : "0")||
            !append_string("\",\"location_id\":\"") || !append_string(loca)||
            !append_string("\",\"SOC\":\"") || !append_string(soc)||
            !append_string("\",\"success\":") || !append_string(success_true) || !append_string("\"}"))
        {
            return false;
        }
        // protect_flag = 1;
        // protect_cnt = 0;
        //protect_mode = 0;
        goto push_in_queue;
    }
    else if (strcmp(status, "free") == 0)
    {
        code = "20010";
				robot_sts = "0";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s free state", Robot_ID);
        msg = msg_buffer;
			
				if (!append_string("{\"code\":\"") || !append_string(code) ||
						!append_string("\",\"robot_sts\":\"") || !append_string(robot_sts) ||
            !append_string("\",\"robot_id\":\"") || !append_string(Robot_ID) ||
            !append_string("\",\"msg\":\"") || !append_string(msg) ||
            !append_string("\",\"available\":\"") || !append_string(_avaiable ? "1" : "0")||
            !append_string("\",\"location_id\":\"") || !append_string(loca)||
            !append_string("\",\"SOC\":\"") || !append_string(soc)||
            !append_string("\",\"success\":") || !append_string(success_true) || !append_string("\"}"))
        {
            return false;
        }
        //protect_mode = 1;
        goto push_in_queue;
    }
		else if (strcmp(status, "move_pick") == 0)
    {
        code = "20011";
				robot_sts = "1";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s is moving to pick", Robot_ID);
        msg = msg_buffer;
			
				if (!append_string("{\"code\":\"") || !append_string(code) ||
						!append_string("\",\"robot_sts\":\"") || !append_string(robot_sts) ||
            !append_string("\",\"robot_id\":\"") || !append_string(Robot_ID) ||
            !append_string("\",\"msg\":\"") || !append_string(msg) ||
				    !append_string("\",\"task_id\":\"") || !append_string(m_server.task_id) ||
            !append_string("\",\"available\":\"") || !append_string(_avaiable ? "1" : "0")||
            !append_string("\",\"location_id\":\"") || !append_string(loca)||
            !append_string("\",\"SOC\":\"") || !append_string(soc)||
            !append_string("\",\"success\":") || !append_string(success_true) || !append_string("\"}"))
        {
            return false;
        }
        //protect_mode = 1;
        goto push_in_queue;
    }
		else if (strcmp(status, "move_place") == 0)
    {
        code = "20012";
				robot_sts = "2";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s is moving to place", Robot_ID);
        msg = msg_buffer;
			
				if (!append_string("{\"code\":\"") || !append_string(code) ||
						!append_string("\",\"robot_sts\":\"") || !append_string(robot_sts) ||
            !append_string("\",\"robot_id\":\"") || !append_string(Robot_ID) ||
            !append_string("\",\"msg\":\"") || !append_string(msg) ||
				    !append_string("\",\"task_id\":\"") || !append_string(m_server.task_id) ||
            !append_string("\",\"available\":\"") || !append_string(_avaiable ? "1" : "0")||
            !append_string("\",\"location_id\":\"") || !append_string(loca)||
            !append_string("\",\"SOC\":\"") || !append_string(soc)||
            !append_string("\",\"success\":") || !append_string(success_true) || !append_string("\"}"))
        {
            return false;
        }
        //protect_mode = 1;
        goto push_in_queue;
    }
		else if (strcmp(status, "move_charge") == 0)
    {
        code = "20013";
				robot_sts = "3";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s is moving to charge", Robot_ID);
        msg = msg_buffer;
			
				if (!append_string("{\"code\":\"") || !append_string(code) ||
						!append_string("\",\"robot_sts\":\"") || !append_string(robot_sts) ||
            !append_string("\",\"robot_id\":\"") || !append_string(Robot_ID) ||
            !append_string("\",\"msg\":\"") || !append_string(msg) ||
            !append_string("\",\"available\":\"") || !append_string(_avaiable ? "1" : "0")||
            !append_string("\",\"location_id\":\"") || !append_string(loca)||
            !append_string("\",\"SOC\":\"") || !append_string(soc)||
            !append_string("\",\"success\":") || !append_string(success_true) || !append_string("\"}"))
        {
            return false;
        }

        //protect_mode = 1;
        goto push_in_queue;
    }
		else if (strcmp(status, "charge") == 0)
    {
        code = "20014";
				robot_sts = "4";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s is charging", Robot_ID);
        msg = msg_buffer;
			
				if (!append_string("{\"code\":\"") || !append_string(code) ||
						!append_string("\",\"robot_sts\":\"") || !append_string(robot_sts) ||
            !append_string("\",\"robot_id\":\"") || !append_string(Robot_ID) ||
            !append_string("\",\"msg\":\"") || !append_string(msg) ||
            !append_string("\",\"available\":\"") || !append_string(_avaiable ? "1" : "0")||
            !append_string("\",\"location_id\":\"") || !append_string(loca)||
            !append_string("\",\"SOC\":\"") || !append_string(soc)||
            !append_string("\",\"Current\":\"") || !append_string(current)||
            !append_string("\",\"Charge_num\":\"") || !append_string(charge_num)||
            !append_string("\",\"success\":") || !append_string(success_true) || !append_string("\"}"))
        {
            return false;
        }

        //protect_mode = 1;
        goto push_in_queue;
    }
    else if (strcmp(status, "arrive") == 0)
    {
        code = "20003";
				snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s arrive", Robot_ID);
        msg = msg_buffer;
        //protect_mode = 1;
    }
//    else if (strcmp(status, "?ú?÷??????") == 0)
//    {
//        code = "20003";
//        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s????????", Robot_ID);
//        msg = msg_buffer;
//    }
    else if (strcmp(status, "??????°?") == 0)
    {
        code = "20004";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s meet hints", Robot_ID);
        msg = msg_buffer;
        success = success_false;
    }
    else if (strcmp(status, "??×????ú????") == 0)
    {
        code = "20005";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s??×????ú????", Robot_ID);
        msg = msg_buffer;
        success = success_false;
    }
    else if (strcmp(status, "???????ú????") == 0)
    {
        code = "20006";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s???????ú????", Robot_ID);
        msg = msg_buffer;
        success = success_false;
    }
		else if (strcmp(status, "???????ú????") == 0)
    {
        code = "20007";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s???????ú????", Robot_ID);
        msg = msg_buffer;
        success = success_false;
    }
    else if (strcmp(status, "?????ì??") == 0)
    {
        code = "20008";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s?????ì??", Robot_ID);
        msg = msg_buffer;
        success = success_false;
    }
    else if (strcmp(status, "??????????") == 0)
    {
        code = "20015";
        msg = "?????????????????????ì??";
        success = success_false;
    }
    else
    {
        //printf("????×???: %s\n", status);
        return false;
    }

    // ?¨??????
    if (!append_string("{\"code\":\"") ||
        !append_string(code) ||
        !append_string("\",\"robot_id\":\"") ||
        !append_string(Robot_ID) ||
        !append_string("\",\"msg\":\"") ||
        !append_string(msg) ||
				!append_string("\",\"task_id\":\"") || 
				!append_string(m_server.task_id) ||
        !append_string("\",\"available\":\"") || 
        !append_string(_avaiable ? "1" : "0")||
        !append_string("\",\"location_id\":\"") || 
        !append_string(loca)||
        !append_string("\",\"SOC\":\"") || 
        !append_string(soc)||
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

    msg->take_id = 0;
    msg->give_id = 0;
    msg->command = -1;

    msg->take_id_char[0] = '\0';
    msg->give_id_char[0] = '\0';
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

// void MsgQueue_PeekProtectMode(MsgQueue* q)
// {
//     if (q->count == 0) {
//         return; // ????,???
//     }

//     const char *msg = q->messages[q->head];
//     if (strstr(msg, "\"code\":\"20001\"") || strstr(msg, "\"code\":\"20002\"")) {
//         protect_mode = 0;
//     } else {
//         protect_mode = 1;
//     }
// }


uint8_t Get_ifaction(void)
{
		return recv_flag;
}
