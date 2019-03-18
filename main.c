/*******************************************************************************
 * Copyright (c) 2009, 2014 VNPT Technology Company.
 *
 * Contributors:
 *       Filename:  main.c
 *
 *    Description:  the source file
 *
 *        Version:  1.0
 *        Created:  2017-6-18 15:58:20
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  Le Thien Duc, thienduc.ee@gmail.com - initial implementation
 *   Organization:  none
 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eat_modem.h"
#include "eat_interface.h"
#include "eat_uart.h"
#include "eat_clib_define.h" //only in main.c

 #include "at-command-interface/at-command-interface.h"
 #include "tcp/tcp.h"
 #include "mqtt/mqtt.h"
 #include "serial-interface/serialInterface.h"
 #include "gps/gps-task.h"

/********************************************************************
* Macros
 ********************************************************************/
#define EAT_MEM_MAX_SIZE 310*1024


/********************************************************************
* Types
 ********************************************************************/
typedef void (*app_user_func)(void*);

u8 *BEARER_STATE[]={
    "DEACTIVATED",
    "ACTIVATING",
    "ACTIVATED",
    "DEACTIVATING",
    "CSD_AUTO_DISC_TIMEOUT",
    "GPRS_AUTO_DISC_TIMEOUT",
    "NWK_NEG_QOS_MODIFY",
    "CBM_WIFI_STA_INFO_MODIF",
};
/********************************************************************
* Extern Variables (Extern /Global)
 ********************************************************************/
 
/********************************************************************
* Local Variables:  STATIC
 ********************************************************************/
static u8 s_memPool[EAT_MEM_MAX_SIZE]; 
static u8 buf[EAT_UART_RX_BUF_LEN_MAX + 1] = {0};//for receive data from uart
/********************************************************************
* External Functions declaration
 ********************************************************************/
extern void APP_InitRegions(void);

/********************************************************************
* Local Function declaration
 ********************************************************************/
void app_main(void *data);
void app_func_ext1(void *data);
static eat_bool app_mem_init();
static eat_bool eat_modem_data_parse(u8* buffer, u16 len, u8* param1, u8* param2);
static void AtCmd_modemReadyCallback(eat_bool result);
static void AtCmd_gsmInitNotifyCallback(eat_bool result);
static void bear_notify_cb(cbm_bearer_state_enum state,u8 ip_addr[4]);


/********************************************************************
* Local Function
 ********************************************************************/
#pragma arm section rodata = "APP_CFG"
APP_ENTRY_FLAG 
#pragma arm section rodata

#pragma arm section rodata="APPENTRY"
const EatEntry_st AppEntry = 
{
    app_main,
    app_func_ext1,
    (app_user_func)AtCmd_interfaceTask,//app_user1,
    (app_user_func)serialInterface_task,//app_user2,
    (app_user_func)mqtt_task,//app_user3,
    (app_user_func)EAT_NULL,//app_user4,
    (app_user_func)EAT_NULL,//app_user5,
    (app_user_func)EAT_NULL,//app_user6,
    (app_user_func)EAT_NULL,//app_user7,
    (app_user_func)EAT_NULL,//app_user8,
    EAT_NULL,
    EAT_NULL,
    EAT_NULL,
    EAT_NULL,
    EAT_NULL,
    EAT_NULL
};
#pragma arm section rodata


static void AtCmd_modemReadyCallback(eat_bool result)
{
	if (result == EAT_TRUE)
	{
		eat_gprs_bearer_open("m-wap",NULL,NULL,bear_notify_cb);
		eat_gprs_bearer_hold();
	}
}

static void AtCmd_gsmInitNotifyCallback(eat_bool result)
{
	if (result == EAT_TRUE)
	{
		
	}
}
/**
 * @brief AT-Command gsm init
 *
 * @func AtCmd_gsm_init()
 *
 * @param pResultNotifyCallback is a callback function
 * @return none
 */
eat_bool AtCmd_gsm_init(ResultNotifyCallback_t pResultNotifyCallback)
{
    eat_bool result = FALSE;
    AtCmdElement_t atCmdInit[]={
        {"AT"AT_CMD_END, NULL, 4, NULL},
        {"ATE0"AT_CMD_END, NULL, 6, NULL}
    };
    atCmdInit[1].pResultNotifyCallback = pResultNotifyCallback;
    result = AtCmd_groupQueueAppend(atCmdInit, sizeof(atCmdInit) / sizeof(atCmdInit[0]));

    return result;
}
void updateGPSInfo(void)
{
	u8 buf[100] = {0};
	u8 sendBuf[500] = "GET /api/get?data={dev:%27ACV0002%27,dri:000,tim:034543,dat:140715,";
	eat_trace("INFO: GPS laction\r\n+ Lat=%0.6f\r\n+ Lon=%0.6f\r\n",
							nmeaInfo.lat,
							nmeaInfo.lon);
 	sprintf(buf, CARMURA_LOCATION, nmeaInfo.lat, nmeaInfo.lon);
 	strcat(sendBuf, buf);
 	strcat(sendBuf, CARMURA_DATA2);
    eat_trace("INFO: Send data to server data=%s",sendBuf);
    
    //tcp_sendDataToServer(sendBuf);
	
}
void app_startGpsUpdateTimer(void)
{
	eat_timer_start(GPS_UPDATE_TIMER, GPS_UPDATE_DATA_TIME);
}
void app_func_ext1(void *data)
{
	/*This function can be called before Task running ,configure the GPIO,uart and etc.
	   Only these api can be used:
		 eat_uart_set_debug: set debug port
		 eat_pin_set_mode: set GPIO mode
		 eat_uart_set_at: set AT port
	*/
		EatUartConfig_st uart_config;
		uart_config.baud = EAT_UART_BAUD_115200;
    	uart_config.dataBits = EAT_UART_DATA_BITS_8;
    	uart_config.parity = EAT_UART_PARITY_NONE;
    	uart_config.stopBits = EAT_UART_STOP_BITS_1;
    	
	eat_uart_set_at_port(eat_uart_at);
	eat_uart_set_debug(eat_uart_debug);
    eat_uart_set_debug_config(EAT_UART_DEBUG_MODE_UART, &uart_config);
    //eat_modem_set_poweron_urc_dir(EAT_USER_1);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name: app_mem_init
 *  Description: init mem for this app to malloc
 *        Input:
 *       Output:
 *       Return:
 *       author: dingfen.zhu
 * =====================================================================================
 */
static eat_bool app_mem_init(void)
{
    eat_bool ret = EAT_FALSE;
	
    ret = eat_mem_init(s_memPool, EAT_MEM_MAX_SIZE);
    if(!ret)
    {
        eat_trace("ERROR: eat memory initial error!");
    }
    return ret;
}

static void handleTopicDuclt1(u8* incommingMessage, int msgLen){
	eat_trace("INFO: topic duclt1 Handled, payload = %.*s\n", msgLen, incommingMessage);
}

static void handleTopicDuclt2(u8* incommingMessage, int msgLen){
	eat_trace("INFO: topic duclt2 Handled, payload = %.*s\n", msgLen, incommingMessage);
}

static void handleTopicDuclt3(u8* incommingMessage, int msgLen){
	eat_trace("INFO: topic duclt3 Handled, payload = %.*s\n", msgLen, incommingMessage);
}

static void bear_notify_cb(cbm_bearer_state_enum state,u8 ip_addr[4])
{
    u8 buffer[128] = {0};
    eat_trace("INFO: bear_notify_cb");
    switch(state)
    {
    	case CBM_DEACTIVATED:
    		break;
    	case CBM_ACTIVATING:
    		break;	
    	case CBM_ACTIVATED:
    		sprintf(buffer,"BEAR_NOTIFY: state = %s, device IP = %d:%d:%d:%d\r\n", 
    				BEARER_STATE[2], 
    				ip_addr[0],ip_addr[1],ip_addr[2],ip_addr[3]);
    		eat_uart_write(EAT_UART_1,buffer,strlen(buffer));
    		mqtt_DeInit();
    	    eat_trace("INFO: Register topic handler\n");
            mqtt_registerTopicHandler("duclt1", handleTopicDuclt1);
            eat_sleep(50);
    		mqtt_registerTopicHandler("duclt2", handleTopicDuclt2);
    		eat_sleep(50);
    		mqtt_registerTopicHandler("duclt3", handleTopicDuclt3);
    		eat_sleep(50);
    		eat_trace("INFO: MQTT init");
    		mqtt_init();
    		break;
    	case CBM_DEACTIVATING:
    		break;
    	case CBM_CSD_AUTO_DISC_TIMEOUT:
    		break;
    	case CBM_GPRS_AUTO_DISC_TIMEOUT:
    		break;
    	case CBM_NWK_NEG_QOS_MODIFY:
    		break;
    	case CBM_WIFI_STA_INFO_MODIFY:
    		break;
    }
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name: app_main
 *  Description: 
 *        Input:
 			data:
 *       Output:
 *       Return:
 *       author: dingfen.zhu
 * =====================================================================================
 */
void app_main(void *data)
{
    EatEvent_st event;
    u16 len = 0;
	
    
    eat_trace("INFO: app_main init");
    APP_InitRegions();//Init app RAM
    APP_init_clib();
	app_mem_init();
#ifdef defined(SINGLE_MODE)
	GPSInit();
	eat_trace("INFO: GPS task init");
	GpsTask_Init();
#elif defined(MCU_MODE)
	tcp_registerSerialProcessHandler();
#endif
	AtCmd_gsm_init(AtCmd_gsmInitNotifyCallback);
	AtCmd_registerModemReadyCallback(AtCmd_modemReadyCallback);
#ifdef defined(SINGLE_MODE)
	app_startGpsUpdateTimer();
#endif
    while(EAT_TRUE)
    {
        eat_get_event(&event);
        eat_trace("MSG id%x", event.event);
        switch(event.event)
        {
            case EAT_EVENT_MDM_READY_RD:
            {
                len = 0;
                len = eat_modem_read(buf, 2048);
                if(len > 0)
                {
                    buf[len] = '\0';
                    //do something
                }
				break;
            }
            case EAT_EVENT_UART_READY_RD:
			{
				serialInterface_RX_handler(&event);
                break;
            }
#ifdef defined(SINGLE_MODE)
            case EAT_EVENT_TIMER:
            	if(event.data.timer.timer_id == GPS_UPDATE_TIMER){
            		eat_trace("INFO: GPS UPDATE timer");
            		if(isTcpConnected == EAT_TRUE){
            			eat_trace("INFO: Send data to server");
            			updateGPSInfo();
            			}
            	app_startGpsUpdateTimer();
            	}
            	break;
#endif
            case EAT_EVENT_MDM_READY_WR:
            default:
			{
                break;
            }
        }
    }
}
