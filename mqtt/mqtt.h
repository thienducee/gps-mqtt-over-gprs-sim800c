/*******************************************************************************
 * Copyright (c) 2009, 2014 VNPT Technology Company.
 *
 * Contributors:
 *       Filename:  mqtt.h
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
#ifndef MQTT_H_
#define MQTT_H_

#include "eat_type.h"
#include "stdint.h"
#include "../tcp/tcp.h"

/*---------------------------------------------------------------------------*/
#define MQTT_PERIODIC_INTERVAL  10000
#define MQTT_PERIODIC_TIMER 	EAT_TIMER_4
 
/*---------------------------------------------------------------------------*/
typedef enum {
  MQTT_FHDR_MSG_TYPE_CONNECT       = 0x10,
  MQTT_FHDR_MSG_TYPE_CONNACK       = 0x20,
  MQTT_FHDR_MSG_TYPE_PUBLISH       = 0x30,
  MQTT_FHDR_MSG_TYPE_PUBACK        = 0x40,
  MQTT_FHDR_MSG_TYPE_PUBREC        = 0x50,
  MQTT_FHDR_MSG_TYPE_PUBREL        = 0x60,
  MQTT_FHDR_MSG_TYPE_PUBCOMP       = 0x70,
  MQTT_FHDR_MSG_TYPE_SUBSCRIBE     = 0x80,
  MQTT_FHDR_MSG_TYPE_SUBACK        = 0x90,
  MQTT_FHDR_MSG_TYPE_UNSUBSCRIBE   = 0xA0,
  MQTT_FHDR_MSG_TYPE_UNSUBACK      = 0xB0,
  MQTT_FHDR_MSG_TYPE_PINGREQ       = 0xC0,
  MQTT_FHDR_MSG_TYPE_PINGRESP      = 0xD0,
  MQTT_FHDR_MSG_TYPE_DISCONNECT    = 0xE0,

  MQTT_FHDR_DUP_FLAG               = 0x08,

  MQTT_FHDR_QOS_LEVEL_0            = 0x00,
  MQTT_FHDR_QOS_LEVEL_1            = 0x02,
  MQTT_FHDR_QOS_LEVEL_2            = 0x04,

  MQTT_FHDR_RETAIN_FLAG            = 0x01,
} mqtt_fhdr_fields_t;

/*---------------------------------------------------------------------------*/
typedef void (* mqtt_messageArrived_handler_t) (char* topic, u8* payload);
typedef void (* mqtt_connectSuccess_callback_t) (void);
typedef void (* mqtt_topicHandler) (u8* incommingMessage, int msgLen); 

typedef enum {
	MQTT_STANBY = 0,
	MQTT_INIT,
	MQTT_CONNECT,
	MQTT_CONNECTED,
	MQTT_SUBSCRIBED,
	MQTT_RECONNECT,
	MQTT_DISCONNECT,
	MQTT_CONNECT_LOST
} mqtt_state_t;


typedef struct __mqtt_conn {
	mqtt_state_t mqtt_state;
	tcp_connection_t tcp_conn;
	mqtt_connectSuccess_callback_t mqttConnSuccess_cb;
}mqtt_conn_t;



typedef struct _mqtt_topicHandlerMap_t
{
  char* topic;
  mqtt_topicHandler topicHandler;
} mqtt_topicHandlerMap_t;

/*---------------------------------------------------------------------------*/
eat_bool mqtt_DeInit(void);
eat_bool mqtt_init(void);
eat_bool mqtt_subscribe( char *topic);
eat_bool mqtt_connect( char* clientID, 
						unsigned short keepAlive,
						char* password,
						char* username);
eat_bool mqtt_publish (char* topic, u8* payload);
void mqtt_registerTopicHandler(const char* topic, mqtt_topicHandler handler);
void mqtt_stateMeachineStart();
void mqtt_task(void *data);

#endif /*MQTT_H_*/
