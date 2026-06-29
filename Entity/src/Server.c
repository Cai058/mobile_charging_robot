#include "Server.h"

// RobotConfig_t RobotConfig_server;

// Message received over 485
char *pbuf_server;
uint16_t len_server;
uint8_t recv_flag; // Whether receive succeeded
uint16_t recv_cnt = 0; // Set recv_flag to 0 after 10ms (ensures controller can read recv_flag = 1)

// Tx
char server_tx_buffer[SERVER_TX_BUFFER_SIZE];
uint16_t m_CurrentSendBufferLen = 0;
char sent_msg[SERVER_TX_BUFFER_SIZE];
uint8_t _tx_complete = 1;
uint16_t sent_cnt;

// Battery SOC
uint8_t m_need_charge = 0;

// Rx
char server_rx_buffer[SERVER_RX_BUFFER_SIZE];  // RX buffer
uint16_t server_rx_buffer_len = 0;          // Current valid data length in buffer
// Structure
ServerMsg_t m_server;

// Msg queue
MsgQueue txQueue;

// // Wait 3s after pick/place completion before sending next message, to prevent TX/RX conflict
// uint8_t protect_flag = 0;
// uint32_t protect_cnt = 0;
// uint8_t protect_mode = 0; // 0: protect_flag disabled (for pick/place done), 1: protect_flag enabled (for other responses)


void Server_Init(void)
{
	UART7_Config();
	ServerMsg_Init(&m_server);
	MsgQueue_Init(&txQueue);
	//send_json_response("place done");
}

void Server_Update(void)
{
		// Get platform messages
		uint16_t len_server = 0;
    char *pbuf_server = get_server_rebuff(&len_server);

		// Get battery SOC
		m_need_charge = Battery_ifNeedCharge();

		//send_json_response("call success");
		// Clear recv_flag after 10ms
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
        // Append data to buffer (prevent overflow)
		if (server_rx_buffer_len + len_server < SERVER_RX_BUFFER_SIZE) {
				memcpy(&server_rx_buffer[server_rx_buffer_len], pbuf_server, len_server);
				server_rx_buffer_len += len_server;
		} else {
				//printf("Error: server buffer overflow\n");
				server_rx_buffer_len = 0;
				clean_server_rebuff();  // Clear data to prevent overflow
				return;
		}

		// Check if JSON end marker '}' received; if not, keep accumulating into server_rx_buffer
		if (server_rx_buffer_len > 0 && server_rx_buffer[server_rx_buffer_len - 1] == JSON_END_CHAR) {
				server_rx_buffer[server_rx_buffer_len] = '\0';  // Ensure string termination
				recv_flag = process_json(server_rx_buffer); // Parse JSON

				server_rx_buffer_len = 0;                   // Clear buffer
				clean_server_rebuff();
		}

		sent_cnt ++;                                       // Rate-limit message sending
		_tx_complete = if_server_complete();               // Check if previous TX is complete
		//MsgQueue_PeekProtectMode(&txQueue);
		//if (sent_cnt % 1000 == 0  && (protect_flag == 0 || protect_mode == 0)){       // Buffer is cleared only after complete reception; send only 10ms after reception
		if (sent_cnt % 1000 == 0){
            if(_tx_complete && txQueue.count > 0)
		{
			MsgQueue_Pop(&txQueue, sent_msg);
			Server_SendString((uint8_t *)sent_msg,strlen(sent_msg));
		}
	}

}

/* Parse JSON command from platform */
uint8_t process_json(const char *json_str)
{
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        //printf("JSON parse failed\n");
        return 0;
    }

    // 1. Verify JSON completeness (check required fields)
    cJSON *robot_id = cJSON_GetObjectItem(root, "robot_id");
    cJSON *command = cJSON_GetObjectItem(root, "command");

    if (!cJSON_IsString(robot_id) || !cJSON_IsString(command)) {
        //printf("JSON incomplete or malformed\n");
        cJSON_Delete(root);
        return 0;
    }

    // 2. Verify robot ID matches
    if (strcmp(robot_id->valuestring, Robot_ID) != 0) {
        //printf("robot_id mismatch, expected: %s, actual: %s\n", Robot_ID, robot_id->valuestring);
        cJSON_Delete(root);
        return 0;
    }

    // 3. Parse command code
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
        //printf("Unknown command");
        cJSON_Delete(root); // Remember to free memory
        return 0;
    }
    //printf("Received command: %d\n", m_server.command);

    // 4. Parse parameters (chg_id, task_id, take_id, give_id)
    // Applies to command 0 (pick), 1 (place), 3 (pick+place)
    if (m_server.command == 0 || m_server.command == 1 || m_server.command == 3)
    {
        cJSON *p_id = NULL;

        // --- A. Common parameters (chg_id and task_id) ---

        // Record chg_id
        p_id = cJSON_GetObjectItem(root, "chg_id");
        if (p_id != NULL) {
            strncpy(m_server.chg_id, p_id->valuestring, sizeof(m_server.chg_id) - 1);
            m_server.chg_id[sizeof(m_server.chg_id) - 1] = '\0';
        } else {
            cJSON_Delete(root);
            return 0; // Missing chg_id
        }

        // Record task_id
        p_id = cJSON_GetObjectItem(root, "task_id");
        if (p_id != NULL) {
            strncpy(m_server.task_id, p_id->valuestring, sizeof(m_server.task_id) - 1);
            m_server.task_id[sizeof(m_server.task_id) - 1] = '\0';
        } else {
            cJSON_Delete(root);
            return 0; // Missing task_id
        }

        // --- B. Parse take_id (required for command 0 or 3) ---
        if (m_server.command == 0 || m_server.command == 3)
        {
            p_id = cJSON_GetObjectItem(root, "take_id");
            if (p_id != NULL) {
                strncpy(m_server.take_id_char, p_id->valuestring, sizeof(m_server.take_id_char) - 1);
                m_server.take_id_char[sizeof(m_server.take_id_char) - 1] = '\0';
                m_server.take_id = parseStringToUint8((char *)m_server.take_id_char);
            } else {
                cJSON_Delete(root);
                return 0; // Command includes pick but missing take_id
            }
        }

        // --- C. Parse give_id (required for command 1 or 3) ---
        if (m_server.command == 1 || m_server.command == 3)
        {
            p_id = cJSON_GetObjectItem(root, "give_id");
            if (p_id != NULL) {
                strncpy(m_server.give_id_char, p_id->valuestring, sizeof(m_server.give_id_char) - 1);
                m_server.give_id_char[sizeof(m_server.give_id_char) - 1] = '\0';
                m_server.give_id = parseStringToUint8((char *)m_server.give_id_char);
            } else {
                cJSON_Delete(root);
                return 0; // Command includes place but missing give_id
            }
        }
    }

    // Free JSON object
    cJSON_Delete(root);
    return 1;
}


bool send_json_response(const char *status,uint8_t _avaiable,uint8_t _location, uint8_t _soc,float _current,uint8_t _charge_num)
{
    static char msg_buffer[256];           // msg buffer
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
    else if (strcmp(status, "obstacle") == 0)
    {
        code = "20004";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s meet hints", Robot_ID);
        msg = msg_buffer;
        success = success_false;
    }
    else if (strcmp(status, "error") == 0)
    {
        code = "20005";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s error", Robot_ID);
        msg = msg_buffer;
        success = success_false;
    }
    else if (strcmp(status, "warning") == 0)
    {
        code = "20006";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s warning", Robot_ID);
        msg = msg_buffer;
        success = success_false;
    }
		else if (strcmp(status, "fault") == 0)
    {
        code = "20007";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s fault", Robot_ID);
        msg = msg_buffer;
        success = success_false;
    }
    else if (strcmp(status, "timeout") == 0)
    {
        code = "20008";
        snprintf(msg_buffer, sizeof(msg_buffer), "robot_%s timeout", Robot_ID);
        msg = msg_buffer;
        success = success_false;
    }
    else if (strcmp(status, "overload") == 0)
    {
        code = "20015";
        msg = "overload error";
        success = success_false;
    }
    else
    {
        //printf("Unknown status: %s\n", status);
        return false;
    }

    // Generic JSON builder (used by Success, Fail, arrive, and error statuses)
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

/* Convert string to uint8_t */
uint8_t parseStringToUint8(const char *str) {
    if (str == NULL) {
        return 0; // Guard against null pointer
    }

    const char *ptr = str;
    int result = 0;

    // Find '-' position
    while (*ptr && *ptr != '-') {
        ptr++;
    }

    // If found '-', parse the part after '-'
    if (*ptr == '-' && *(ptr + 1) != '\0') {
        ptr++;  // Move past '-'
    } else {
        ptr = str; // No '-', parse entire string
    }

    // Convert characters to integer
    while (*ptr) {
        if (*ptr >= '0' && *ptr <= '9') {
            result = result * 10 + (*ptr - '0');
            if (result > 255) { // Keep within uint8_t max
                return 255;
            }
        } else {
            break; // Stop on non-digit character
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
    if (q->count == 0 || _tx_complete == 0) return false;

    snprintf(out_msg, SERVER_TX_BUFFER_SIZE, "%s", q->messages[q->head]);
    q->head = (q->head + 1) % SERVER_TX_BUFFER_NUM;
    q->count--;

		//Server_SendString((uint8_t *)out_msg,strlen(out_msg));
    return true;
}

// void MsgQueue_PeekProtectMode(MsgQueue* q)
// {
//     if (q->count == 0) {
//         return;
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
