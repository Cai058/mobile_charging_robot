#include "Server.h"

// 485���յ�����Ϣ
char *pbuf_server;
uint16_t len_server;
uint8_t recv_flag; // �Ƿ���ճɹ�
uint8_t recv_cnt = 0; // ��10ms����recv_flagΪ0����֤controller�ܹ��յ�recv_flag = 1��ʱ��

// Tx
char server_tx_buffer[SERVER_TX_BUFFER_SIZE];
uint16_t m_CurrentSendBufferLen = 0;
char sent_msg[SERVER_TX_BUFFER_SIZE];
uint8_t _tx_complete = 1;
uint16_t sent_cnt;

// ��� SOC
uint8_t m_need_charge = 0;

// Rx
char server_rx_buffer[SERVER_RX_BUFFER_SIZE];  // ���ջ�����
uint16_t server_rx_buffer_len = 0;          // ��ǰ��������Ч���ݳ���

// Structure
ServerMsg_t m_server;

// Msg queue
MsgQueue txQueue;





void Server_Init(void)
{
	UART7_Config();
	ServerMsg_Init(&m_server);
	MsgQueue_Init(&txQueue);
	//send_json_response("�������");
}

void Server_Update(void)
{
		// ��ȡƽ̨��Ϣ
		uint16_t len_server = 0;
    char *pbuf_server = get_server_rebuff(&len_server);
	
		// ��ȡ���SOC
		m_need_charge = Battery_ifNeedCharge();
		
		//send_json_response("���óɹ�");
		// ��10ms����recv_flag = 0
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
        // ������׷�ӵ�����������ֹ�����
		if (server_rx_buffer_len + len_server < SERVER_RX_BUFFER_SIZE) {
				memcpy(&server_rx_buffer[server_rx_buffer_len], pbuf_server, len_server);
				server_rx_buffer_len += len_server;
		} else {
				printf("Error: server buffer overflow\n");
				server_rx_buffer_len = 0;
				clean_server_rebuff();  // ������ݷ�ֹ���
				return;
		}

		// ����Ƿ��յ� JSON ������ `}`��δ�յ�ʱ���������գ�����server_rx_buffer��
		if (server_rx_buffer_len > 0 && server_rx_buffer[server_rx_buffer_len - 1] == JSON_END_CHAR) {
				server_rx_buffer[server_rx_buffer_len] = '\0';  // ȷ���ַ�������
				recv_flag = process_json(server_rx_buffer); // ���� JSON
			
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
				
				server_rx_buffer_len = 0;                   // ��ջ�����
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

/* ����ƽ̨������json�ļ�*/
uint8_t  process_json(const char *json_str)
{
	 cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        printf("JSON ����ʧ��\n");
        return 0;
    }

    // 1. �˶� JSON �Ƿ���������������ֶΣ�
    cJSON *robot_id = cJSON_GetObjectItem(root, "robot_id");
    cJSON *command = cJSON_GetObjectItem(root, "command");

    if (!cJSON_IsString(robot_id) || !cJSON_IsString(command)) {
        printf("JSON ���ݲ��������ʽ����\n");
        cJSON_Delete(root);
        return 0;
    }

    // 2. �˶Ի����� ID �Ƿ�ƥ��
    if (strcmp(robot_id->valuestring, ROBOT_ID) != 0) {
        printf("robot_id ��ƥ��, Ԥ��: %s, ʵ��: %s\n", ROBOT_ID, robot_id->valuestring);
        cJSON_Delete(root);
        return 0;
    }

    // 3. ��ȡ������
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
				printf("δ֪����");
				return 0;
		}
    printf("�յ�������: %d\n", m_server.command);
		
		
		// 4. �����ץȡ���߷��ã���ȡtake_id, give_id, chg_id, task_id
		if(m_server.command == 0 || m_server.command == 1)
		{
				// ��¼ take_id �� give_id��������ڣ�
				cJSON *p_id = cJSON_GetObjectItem(root, "take_id");
			
				if(p_id != NULL)
				{
						strcpy(m_server.target_id_char,p_id->valuestring);
						m_server.target_id = parseStringToUint8((char *)m_server.target_id_char);
						printf("��¼ take_id: %d\n", 	m_server.target_id);
				}
				else
				{
						p_id = cJSON_GetObjectItem(root, "give_id");
						if(p_id != NULL)
						{
								strcpy(m_server.target_id_char,p_id->valuestring);
								m_server.target_id = parseStringToUint8((char *)m_server.target_id_char);
								printf("��¼ give_id: %d\n", m_server.target_id);
						}
						else
						{
								printf("take id �� give id����ȷ");
								return 0;
						}
				}
				
				// ��¼ chg_id��������ڣ�
				p_id = cJSON_GetObjectItem(root, "chg_id");
				if(p_id != NULL)
				{
						strcpy(m_server.chg_id,p_id->valuestring);
						printf("��¼ chg_id: %s\n", m_server.chg_id );
				}
				else
				{
						return 0;
				}
				// ��¼ task_id (�������)
				p_id = cJSON_GetObjectItem(root,"task_id");
				if(p_id != NULL)
				{
					strcpy(m_server.task_id,p_id->valuestring);
					printf("��¼ task_id: %s\n", m_server.task_id);
				}
				else
				{
						return 0;
				}
		}
    
		//5. ����ǳ�������ȡ�Ƿ���
		if(m_server.command == 2)
		{
				cJSON *is_charge = cJSON_GetObjectItem(root, "is_charge");
				if(is_charge != NULL)
				{
						strcpy(m_server.is_charge_char,is_charge->valuestring);
						m_server.is_charge = (uint8_t)atoi(m_server.is_charge_char);
						printf("��¼ is_charge: %d\n", 	m_server.is_charge);
				}
				else
				{
						printf("is_charge����");
						return 0;
				}
				
		}
    // �ͷ� JSON ����
    cJSON_Delete(root);
		
		return 1;
}


bool send_json_response(const char *status)
{
    static char msg_buffer[256];           // msg ƴ���û�����

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
//    else if (strcmp(status, "�����˳��") == 0)
//    {
//        code = "20003";
//        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s���ڳ��", ROBOT_ID);
//        msg = msg_buffer;
//    }
    else if (strcmp(status, "�����ϰ�") == 0)
    {
        code = "20004";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s meet hints", ROBOT_ID);
        msg = msg_buffer;
        success = success_false;
    }
    else if (strcmp(status, "���ߵ������") == 0)
    {
        code = "20005";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s���ߵ������", ROBOT_ID);
        msg = msg_buffer;
        success = success_false;
    }
    else if (strcmp(status, "���͵������") == 0)
    {
        code = "20006";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s���͵������", ROBOT_ID);
        msg = msg_buffer;
        success = success_false;
    }
		else if (strcmp(status, "�Խӵ������") == 0)
    {
        code = "20007";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s�Խӵ������", ROBOT_ID);
        msg = msg_buffer;
        success = success_false;
    }
    else if (strcmp(status, "����쳣") == 0)
    {
        code = "20008";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s����쳣", ROBOT_ID);
        msg = msg_buffer;
        success = success_false;
    }
    else if (strcmp(status, "ָ��δִ��") == 0)
    {
        code = "20015";
        msg = "����ָ��δִ�У�ִ���쳣";
        success = success_false;
    }
    else
    {
        printf("δ֪״̬: %s\n", status);
        return false;
    }

    // ͨ�ø�ʽ
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

/* ���ַ���ת��Ϊuint8_t*/
uint8_t parseStringToUint8(const char *str) {
    if (str == NULL) {
        return 0; // ��ֹ��ָ��
    }

    const char *ptr = str;
    int result = 0;

    // �ҵ� '-' λ��
    while (*ptr && *ptr != '-') {
        ptr++;
    }

    // ����ҵ� '-'������ '-' ֮��Ĳ���
    if (*ptr == '-' && *(ptr + 1) != '\0') {
        ptr++;  // �ƶ��� '-' ֮��
    } else {
        ptr = str; // ���û�� '-', ���������ַ���
    }

    // ����ַ�ת��Ϊ����
    while (*ptr) {
        if (*ptr >= '0' && *ptr <= '9') {
            result = result * 10 + (*ptr - '0');
            if (result > 255) { // ȷ�������� uint8_t ���ֵ
                return 255;
            }
        } else {
            break; // �������ַ�ֹͣ����
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
