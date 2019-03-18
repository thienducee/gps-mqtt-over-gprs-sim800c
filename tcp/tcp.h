/*******************************************************************************
 * Copyright (c) 2009, 2014 VNPT Technology Company.
 *
 * Contributors:
 *       Filename:  tcp.h
 *
 *    Description:  the header file
 *
 *        Version:  1.0
 *        Created:  2017-6-18 15:58:20
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  Le Thien Duc, thienduc.ee@gmail.com - initial implementation
 *   Organization:  none
 *******************************************************************************/
#ifndef __TCP_H__
#define __TCP_H__
#include "eat_modem.h"
#include "eat_socket.h"

/*---------------------------------------------------------------------------*/
//#define THING_SPEAK_DATA  "GET /update?api_key=FMWIUCBQGH4HAXS2&field1=%d&field2=%d HTTP/1.1\r\nHost: api.thingspeak.com\r\nConnection: close\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n\r\n"AT_CMD_END
//#define CARMUDA_DATA 	  "GET /api/get?data={dev:%27ACV3200%27,dri:000,tim:034543,dat:140715,lat:%0.6f,lon:%0.6f,wng:00000000,anl:13505.00,pul:0.00,ope:1005915,dig:00001100,vgp:0.0,dir:0.0,vsr:0.0,mil:581,old:0,sat:3,hwv:1.0,fwv:2.924,clt:0,clg:0,sig:21,hdo:1.35,bat:98,epw:13.505,drt:0000,cdt:0000} HTTP/1.1\r\nHost: carmuda.com\r\nConnection: close\r\nContent-Type: application/json\r\n\r\n"
#define CARMUDA_DATA1	  "GET /api/get?data={dev:%27ACV0002%27,dri:000,tim:034543,dat:140715,"
#define CARMURA_DATA2     "wng:00000000,anl:13505.00,pul:0.00,ope:1005915,dig:00001100,vgp:0.0,dir:0.0,vsr:0.0,mil:581,old:0,sat:3,hwv:1.0,fwv:2.924,clt:0,clg:0,sig:21,hdo:1.35,bat:98,epw:13.505,drt:0000,cdt:0000} HTTP/1.1\r\nHost: carmuda.com\r\nConnection: open\r\nContent-Type: application/json\r\n\r\n"
#define CARMURA_LOCATION  "lat:%0.6f,lon:%0.6f,"
#define GPS_UPDATE_DATA_TIME 60000
#define GPS_UPDATE_TIMER EAT_TIMER_3

/*---------------------------------------------------------------------------*/
typedef void (*tcp_msgReceived_callback_t) (s8 connection_id);
typedef void (*tcp_connectSucces_callback_t) (s8 connection_id);
typedef void (*tcp_connectFailure_callback_t) (s8 connection_id);
typedef void (*tcp_connectionLost_callback_t) (s8 error);

typedef enum 
{
	TCP_INIT,
	TCP_FOUND_ADDR,
	TCP_CONNECTED,
	TCP_CONNECTION_CLOSE,
	TCP_CONNECTION_ERROR,
} tcp_state_t;

typedef struct __tcp_connection {
	tcp_state_t tcp_state;
	const char* hostName;
	sockaddr_struct  server;
	s8 connection_id;
	tcp_msgReceived_callback_t msg_cb;
	tcp_connectionLost_callback_t con_lost_cb;
	tcp_connectSucces_callback_t  connSuccess_cb;
} tcp_connection_t;


/*---------------------------------------------------------------------------*/

eat_bool tcp_init(tcp_connection_t *conn);
eat_bool tcp_connect(tcp_connection_t *conn);
eat_bool tcp_disconnect(tcp_connection_t *conn);
eat_bool tcp_connRegister(tcp_connection_t *conn);
eat_bool tcp_getServerIpAddr(const char* domain, u8* ipAddr, eat_hostname_notify call_back);
eat_bool tcp_sendDataToServer(tcp_connection_t *conn, u8 *data, int dataLen);
void soc_notify_cb(s8 s,soc_event_enum event,eat_bool result, u16 ack_size);

#endif /*TCP_H_*/
