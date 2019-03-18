#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eat_interface.h"
#include "eat_uart.h"
#include "../cJSON/cJSON.h"
#include "serialInterface.h"
#include "../gps/gps.h"


const EatUart_enum eat_uart_app= EAT_UART_1;
const EatUart_enum eat_uart_debug = EAT_UART_USB;
const EatUart_enum eat_uart_at = EAT_UART_2;

static u8 RX_buf[EAT_UART_RX_BUF_LEN_MAX + 1] = {0};



PostTrackingData_handler_t pPostTrackingData_handler;
Connect_handler_t 		   pConnect_handler;
Disconnect_handler_t	   pDisconnect_handler;

//static void serialInterface_RX_handler(const EatEvent_st* event);

void serialInterface_init(void)
{
	EatUartConfig_st uart_config;
	if(eat_uart_open(eat_uart_app) == EAT_FALSE)
    {
	    eat_trace("ERROR: func=%s, uart=%d open fail!", __FUNCTION__, eat_uart_app);
    }else
    {
    	uart_config.baud = EAT_UART_BAUD_9600;
    	uart_config.dataBits = EAT_UART_DATA_BITS_8;
    	uart_config.parity = EAT_UART_PARITY_NONE;
    	uart_config.stopBits = EAT_UART_STOP_BITS_1;	
    	if(EAT_FALSE == eat_uart_set_config(eat_uart_app, &uart_config))
    	{
        	eat_trace("ERROR: [%s] uart(%d) set config fail!", __FUNCTION__, eat_uart_app);
    	}
    }
}

void serialInterface_regisConnectHandler(Connect_handler_t pFunction)
{
	pConnect_handler = 	pFunction;
}

void serialInterface_regisDisconnectHandler(Disconnect_handler_t pFunction)
{
	pDisconnect_handler = pFunction;
}

void serialInterface_regisPostTrackingDataHandler(PostTrackingData_handler_t pFunction)
{
	pPostTrackingData_handler = pFunction;
}

void serialInterface_processCommand(u8* cmdBuf)
{
	cJSON *inComingCmd;
	cJSON *dataType;
	
	inComingCmd = cJSON_Parse((char*)cmdBuf);
	dataType = cJSON_GetObjectItem(inComingCmd, "dataType");
	if(strcmp(dataType->valuestring, (char*)COMMAND_TYPE) == 0)
	{
		cJSON *command 	= cJSON_GetObjectItem(inComingCmd, "cmd");
		if(strcmp(command->valuestring, CONNECT ) == 0)
		{
			pConnect_handler();
		}
		if(strcmp(command->valuestring, DISCONNECT ) == 0)
		{
			pDisconnect_handler();
		}
		if(strcmp(command->valuestring, POST_TRACKING_DATA ) == 0)
		{
			cJSON *data 	= cJSON_GetObjectItem(inComingCmd, "data");
			pPostTrackingData_handler((u8*)data->valuestring);
		}
	}
	cJSON_Delete(inComingCmd);
}
/* 
 * ===  FUNCTION  ======================================================================
 *         Name: uart_rx_proc
 *  Description: 
 *        Input:
 			data:
 *       Output:
 *       Return:
 *       author: dingfen.zhu
 * =====================================================================================
 */
void serialInterface_RX_handler(const EatEvent_st* event)
{
    u16 len;
    EatUart_enum uart = event->data.uart.uart;
	u8* pRX_buf = RX_buf;
    len = eat_uart_read(uart, RX_buf, EAT_UART_RX_BUF_LEN_MAX);
    if(len != 0)
    {
		RX_buf[len] = '\0';
		
		//eat_trace("INFO: [%s] recv data form UART port=%d",__FUNCTION__, uart);
					
		if (uart == eat_uart_debug)//eat_uart_app)
		{
#ifdef defined(SINGLE_MODE)
			//eat_trace("INFO: recv data gps rxBuf=%s",GPS_Buff);
			while(len--)
			{
				GPS_ComnandParser(*pRX_buf++);
			}
			GpsTask_Control();
#elif defined(MCU_MODE)
			eat_trace("INFO: recv data for MCU rxBuf=%s",pRX_buf);
			serialInterface_processCommand(pRX_buf);
#endif
			memset(RX_buf, 0, len+1);
		}
			
    }
}
void serialInterface_task(void *data)
{
	EatEvent_st event;
	serialInterface_init();
    while(EAT_TRUE)
    {
        eat_get_event_for_user(EAT_USER_2, &event);
        switch(event.event)
        {
            case EAT_EVENT_UART_READY_RD:
			{
				serialInterface_RX_handler(&event);
                break;
            }
            case EAT_EVENT_UART_SEND_COMPLETE :
			{
                break;
            }
            default:
                break;

        }
    }
}

