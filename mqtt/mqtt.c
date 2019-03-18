/*******************************************************************************
 * Copyright (c) 2009, 2014 VNPT Technology Company.
 *
 * Contributors:
 *       Filename:  mqtt.c
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
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "mqtt.h"
#include "MQTTPacket.h"
#include "eat_interface.h"
#include "../linkedList/linkedList.h"
 
#define MQTT_BROKER_ADDR "ansvhcm.vn"
#define MQTT_BROKER_PORT 41883
#define MQTT_CLIENT_ID	 "thienduc2212"
#define MQTT_KEEP_ALIVE  120
#define MQTT_USERNAME	 ""
#define MQTT_PASSWORD	 ""
/*---------------------------------------------------------------------------*/
static mqtt_conn_t mqtt_conn;

linkedList_t* topicHandlerList = NULL;
/*---------------------------------------------------------------------------*/
/**
 * Functions prototype
 */
static void getHostNameNotifyCb(u32 request_id,eat_bool result,u8 ip_addr[4]);
static void tcpMsgReceived_callback(s8 connection_id);
static void tcpConnect_callback(s8 connection_id);
static int tcpGetData(unsigned char* buf,int count);
static void mqtt_connetcSuccess_callback(void);
static void mqtt_messageArrived_handler(char* topic, u8* payload, int payload_len);

/*---------------------------------------------------------------------------*/
static void getHostNameNotifyCb(u32 request_id, eat_bool result, u8 ip_addr[4]) {
    memcpy(mqtt_conn.tcp_conn.server.addr, ip_addr, 4);
    eat_trace("INFO: getHostNameNotifyCb serverAddr = %d.%d.%d.%d",
    															   mqtt_conn.tcp_conn.server.addr[0],
    															   mqtt_conn.tcp_conn.server.addr[1],
    															   mqtt_conn.tcp_conn.server.addr[2], 
    															   mqtt_conn.tcp_conn.server.addr[3]);
    tcp_connect(&mqtt_conn.tcp_conn); 
}

static void tcpConnect_callback(s8 connection_id){
	if(connection_id == mqtt_conn.tcp_conn.connection_id){
		eat_trace("INFO: tcp connected to borker");
		mqtt_connect(MQTT_CLIENT_ID, MQTT_KEEP_ALIVE, MQTT_PASSWORD , MQTT_USERNAME);
		mqtt_conn.mqtt_state = MQTT_CONNECT; 
	}
}


static int tcpGetData(u8* buf,int count){
	int len = 0;
	eat_soc_recv(mqtt_conn.tcp_conn.connection_id, buf, count);
	len = count;
	return len;
}

static void tcpMsgReceived_callback(s8 connection_id){
	u8 buf[1024];
    int buflen = 1024;
    int typePacket = MQTTPacket_read(buf, buflen, tcpGetData);
	if(connection_id == mqtt_conn.tcp_conn.connection_id){
		    if(typePacket == SUBACK) 	/* wait for suback */
    			{
    				u16 submsgid;
    				int subcount;
    				int granted_qos;
    				eat_trace("INFO: RECEIVED SUBACK\r\n");
    				MQTTDeserialize_suback(&submsgid, 1, &subcount, &granted_qos, buf, buflen);
    				mqtt_conn.mqtt_state = MQTT_SUBSCRIBED;
    			}else if(typePacket == CONNACK) 	/* wait for conack */
    			{
    				//u8 sessionPresent, connack_rc;
    				eat_trace("INFO: RECEIVED CONNACK\r\n");
    				//if (MQTTDeserialize_connack(&sessionPresent, &connack_rc, buf, buflen) != 1 || connack_rc != 0)
//					{
//						eat_trace("INFO: Unable to connect, return code %d\n", connack_rc);
//						return;
//					}
    				mqtt_conn.mqttConnSuccess_cb();
    				mqtt_conn.mqtt_state = MQTT_CONNECTED;
    			}else if (typePacket == PUBLISH)
    			{
					unsigned char dup;
					int qos;
					unsigned char retained;
					unsigned short msgid;
					int payloadlen_in = 0;
					unsigned char* payload_in;
					MQTTString receivedTopic;
    				
    				if(MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &receivedTopic,
    	    								   &payload_in, &payloadlen_in, buf, buflen) == 1){
    	    							
    	    				//eat_trace("INFO: Message arrived at topic = %.*s, payload = %.*s\n ",receivedTopic.lenstring.len, receivedTopic.lenstring.data, payloadlen_in, payload_in);
    	    				mqtt_messageArrived_handler(receivedTopic.lenstring.data, payload_in, payloadlen_in);
    	    			}
    			}
	}
	
}

static void mqtt_connetcSuccess_callback(void)
{
     linkedList_Element_t* currentElement = NULL;
     mqtt_topicHandlerMap_t* topicHandlerMap = NULL;
     /* Loop through the Topic Handler Map to subscribe to all the topics */
     	 eat_trace("INFO: MQTT connected");
      do {
        currentElement = linkedList_NextElement(topicHandlerList,
                                                            currentElement);
        if (currentElement != NULL) {
          topicHandlerMap = (mqtt_topicHandlerMap_t*)currentElement->content;
          mqtt_subscribe(topicHandlerMap->topic);
        }
      } while (currentElement != NULL);
}

static void mqtt_messageArrived_handler(char* topic, u8* payload, int payload_len)
{
	//cJSON* incomingMessageJson;
    linkedList_Element_t* currentElement = NULL;
    mqtt_topicHandlerMap_t* topicHandlerMap = NULL;

    //incomingMessageJson = cJSON_Parse(payload);

    // loop determine which topic handle to call
    do {
        currentElement = linkedList_NextElement(topicHandlerList,
                                               currentElement);
        if (currentElement != NULL)
        {
            topicHandlerMap = (mqtt_topicHandlerMap_t*)currentElement->content;
            // If the incoming topic matches a topic in the map, call it's handler
            if(strncmp(topic, topicHandlerMap->topic, strlen(topicHandlerMap->topic))==0)
            {
                topicHandlerMap->topicHandler(payload, payload_len);
                break;
            }
        }
    } while(currentElement != NULL);

   // cJSON_Delete(incomingMessageJson);
	
}

eat_bool mqtt_DeInit(void) {
    /* Init topicHandleList */
    topicHandlerList = linkedList_Init();
    mqtt_conn.tcp_conn.hostName = MQTT_BROKER_ADDR;
	mqtt_conn.tcp_conn.server.port = MQTT_BROKER_PORT;
	mqtt_conn.tcp_conn.server.addr_len = 4;
	mqtt_conn.tcp_conn.server.sock_type = SOC_SOCK_STREAM;
	mqtt_conn.tcp_conn.msg_cb = tcpMsgReceived_callback;
	mqtt_conn.tcp_conn.connSuccess_cb = tcpConnect_callback;
	mqtt_conn.mqttConnSuccess_cb = mqtt_connetcSuccess_callback;
}

eat_bool mqtt_init(void) {
	
	tcp_connRegister(&mqtt_conn.tcp_conn);
	tcp_init(&mqtt_conn.tcp_conn);
	tcp_getServerIpAddr(mqtt_conn.tcp_conn.hostName, mqtt_conn.tcp_conn.server.addr, getHostNameNotifyCb);

	return EAT_TRUE;
}

eat_bool mqtt_publish (char* topic, u8* payload)
{
	
	eat_bool ret;
	int payloadLen = strlen(payload);
	u8 buf[200] = {0};
	int bufLen = 200;
	int len = 0;
	
	MQTTString topicString = MQTTString_initializer;
	topicString.cstring = topic;
    eat_trace("INFO: MQTT Publish topic = %s, payload = %s \n", topic, payload);
	len = MQTTSerialize_publish(buf, bufLen, 0, 0, 0, 0, topicString, payload, payloadLen);
	ret = tcp_sendDataToServer(&mqtt_conn.tcp_conn, buf, len);
	if(!ret){
		eat_trace("ERROR: mqtt_publish failed");
		return EAT_FALSE;
		
	}
	return EAT_TRUE;
}

eat_bool mqtt_subscribe( char *topic) {
    eat_bool ret;
	u8 buf[200];
	int buflen = sizeof(buf);
	int msgid = 1;
	int req_qos = 0;
	int len = 0;
	
	MQTTString topicString = MQTTString_initializer;
	/* subscribe */
	topicString.cstring = topic;
	eat_trace("INFO: MQTT Subscribe topic:%s \n",topic);
	len = MQTTSerialize_subscribe(buf, buflen, 0, msgid, 1, &topicString, &req_qos);
	ret = tcp_sendDataToServer(&mqtt_conn.tcp_conn, (u8*) buf , len);
	if(!ret){
		eat_trace("ERROR: mqtt_subscribe failed");
		return EAT_FALSE;
		
	}
	return EAT_TRUE;
}

eat_bool mqtt_connect(  char* clientID, 
						unsigned short keepAlive,
						char* password,
						char* username) {

	eat_bool ret;
	char buf[200] = {0};
	int bufLen = 200;
	int len = 0;
	
	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
				data.clientID.cstring = clientID;
				data.keepAliveInterval = keepAlive;
				data.cleansession = 1;
				data.password.cstring = password;
				data.username.cstring = username;
				
				len = MQTTSerialize_connect((u8*)buf, bufLen, &data); 
				eat_trace("INFO: MQTT connect");
				ret = tcp_sendDataToServer(&mqtt_conn.tcp_conn, (u8*) buf, len );
				if(!ret ) {//send failed
					eat_trace("ERROR: mqtt_connect failed");
					return EAT_FALSE;
				} 
				eat_trace("INFO: mqtt_connect success");
	return EAT_TRUE;

}

static
mqtt_topicHandlerMap_t* createMqttTopicHandlerElement(const char* topicString,
                                          			  mqtt_topicHandler topicHandlerFunction)
{
    mqtt_topicHandlerMap_t* topicHandlerMap =
    (mqtt_topicHandlerMap_t*)malloc(sizeof(mqtt_topicHandlerMap_t));
    topicHandlerMap->topic = (char*)topicString;
    topicHandlerMap->topicHandler = topicHandlerFunction;
    return topicHandlerMap;
}


/** @brief transportMqtt_registerTopicHandler
 *
 * Register topic handler function
 * 
 * @param topic String contains the topic for a message subscription
 * @param handler The function handler of the topic
 * @return: none
 */
void mqtt_registerTopicHandler(const char* topic, mqtt_topicHandler handler)
{
    /* Add an handel of the topic to linked-list */
    linkedList_PushBack(topicHandlerList,
                       (void*)createMqttTopicHandlerElement(topic, handler));
}


void mqtt_stateMeachineStart(){
	mqtt_conn.mqtt_state = MQTT_INIT;
}

void mqtt_task(void *data)
{
	EatEvent_st event;
	eat_timer_start(MQTT_PERIODIC_TIMER, MQTT_PERIODIC_INTERVAL);
	mqtt_conn.mqtt_state = MQTT_STANBY;
    while(EAT_TRUE)
    {
        eat_get_event_for_user(EAT_USER_3, &event);
        switch(event.event)
        {
            case EAT_EVENT_TIMER:
            	if(event.data.timer.timer_id == MQTT_PERIODIC_TIMER){
            		switch(mqtt_conn.mqtt_state){
            			case MQTT_INIT:
            				break;
            			case MQTT_CONNECT:
            				eat_trace("INFO: MQTT connect");
            				break;
            			case MQTT_CONNECTED:
            				eat_trace("INFO: MQTT connected");
            				//mqtt_publish("thienduc_mqtt_test", "hello");
            				break;
            			case MQTT_SUBSCRIBED:
            				eat_trace("INFO: MQTT subscribed");
            			    mqtt_publish("thienduc_mqtt_test", "hello");
            				break;
            			case MQTT_RECONNECT:
            				break;
            			case MQTT_DISCONNECT:
            				break;
            			case MQTT_CONNECT_LOST:
            				break;
            		}
            		eat_timer_start(MQTT_PERIODIC_TIMER, MQTT_PERIODIC_INTERVAL);
            	}
            	break;
           default:
                break;

        }
    }
}

