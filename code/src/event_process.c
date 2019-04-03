#include "event_process.h"
#include "cJSON.h"

typedef void (*timercb_func)(g_msg_queue_para* g_msg_queue);
void timerout_cb(g_msg_queue_para* g_msg_queue){
	struct msg_st data;
	data.msg_type = MSG_TIMEOUT;
	data.msg_number = MSG_TIMEOUT;
	data.msg_len = 0;
	postMsgQueue(&data,g_msg_queue);
}
/*
void reset_system(){
	// stop and close csi
	zlog_info(temp_log_handler,"enter reset_system() \n");
	zlog_info(temp_log_handler,"gw_stopcsi() \n");
	gw_stopcsi();
	zlog_info(temp_log_handler,"stopReceThread : gw_close_csi() \n");
	gw_closecsi();
	// close rssi
	zlog_info(temp_log_handler,"close_rssi() \n"); 
	close_rssi();
	zlog_info(temp_log_handler,"------------------------- reset_system() \n");
}
*/

void eventLoop(g_server_para* g_server, g_msg_queue_para* g_msg_queue, g_timer_para* g_timer, 
		g_broker_para* g_broker, zlog_category_t* zlog_handler)
{
	while(1){
		zlog_info(zlog_handler,"wait getdata ----- \n");
		struct msg_st* getData = getMsgQueue(g_msg_queue);
		if(getData == NULL)
			continue;
		
		switch(getData->msg_type){
			case MSG_ACCEPT_NEW_CLIENT:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_ACCEPT_NEW_CLIENT: msg_number = %d",getData->msg_number);

				if(g_server->hasTimer == 0){
					Start_Timer(timerout_cb, 4, 0, g_timer);
					g_server->hasTimer = 1;
					zlog_info(zlog_handler," ---------- start timer ------------");
				}

				break;
			}
			case MSG_RSSI_STATE_CHANGE:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_RSSI_STATE_CHANGE: msg_number = %d",getData->msg_number);
				rssi_state_change(getData->msg_json,getData->msg_len,g_broker);
				break;
			}
			case MSG_INQUIRY_STATE:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_INQUIRY_STATE: msg_number = %d",getData->msg_number);

				inquiry_state_from(getData->msg_json,getData->msg_len,g_broker);				
				
				break;
			}
			case MSG_TIMEOUT:
			{
				zlog_info(zlog_handler," ---------------- EVENT : MSG_TIMEOUT: msg_number = %d",getData->msg_number);
				if(g_server->waiting == STATE_DISCONNECTED)
					break;
				if(g_server->g_receive->connected == 1){
					g_server->g_receive->connected = 0;
				}else{
					zlog_info(zlog_handler," ------may be disconnect ----disconnect_cnt = %d",g_server->g_receive->disconnect_cnt);
					if(g_server->g_receive->disconnect_cnt == 2){
						//confirm disconnect , then stop and clear rece thread
						stopReceThread(g_server);
						zlog_info(zlog_handler,"############# confirm disconnect , then stop and clear receive thread");
					}
					g_server->g_receive->disconnect_cnt++;
				}
				break;
			}
			case MSG_RECEIVED_HEART_BEAT:
			{
				g_server->g_receive->connected = 1;
				zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_HEART_BEAT: msg_number = %d",getData->msg_number);
				break;
			}
			default:
				break;
		}// end switch
	}// end while(1)
}






