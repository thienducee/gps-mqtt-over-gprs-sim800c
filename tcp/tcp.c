/*******************************************************************************
 * Copyright (c) 2009, 2014 VNPT Technology Company.
 *
 * Contributors:
 *       Filename:  tcp.c
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
 #include "string.h"
 #include "eat_type.h"
 #include "eat_uart.h"
 #include "tcp.h"
 #include "eat_interface.h"
 #include "../at-command-interface/at-command-interface.h"
 #include "../gps/gps.h"
  #include "../serial-interface/serialInterface.h"


#define TCP_CONNECT_PARAM "AT+CIPSTART=\"TCP\",\"%s\",\"%d\""AT_CMD_END
#define TCP_SEND_CMD 	  "AT+CIPSEND=%d"AT_CMD_END

u8 *SOC_EVENT[]={
    "SOC_READ",
    "SOC_WRITE",  
    "SOC_ACCEPT", 
    "SOC_CONNECT",
    "SOC_CLOSE", 
    "SOC_ACKED"
};
static u8 serverAddr[4] = {0};
sockaddr_struct server;
eat_bool isTcpConnected = EAT_FALSE;
s8 socket_id;

/**
 * Functions prototype
 */

static tcp_connection_t *tcp_connection;


static eat_bool tcp_postTrackingData_handler(u8* buf)
{	
	return tcp_sendDataToServer(tcp_connection, buf , strlen(buf));
}

static eat_bool tcp_connect_handler()
{
	//return tcp_connect();
	return 0;
}

static eat_bool tcp_disconnect_handler()
{
	
	//return tcp_disconnect();
	return 0;
}

void tcp_registerSerialProcessHandler(void)
{
	serialInterface_regisConnectHandler(tcp_connect_handler);
	serialInterface_regisDisconnectHandler(tcp_disconnect_handler);
	serialInterface_regisPostTrackingDataHandler(tcp_postTrackingData_handler);
}

eat_bool tcp_connRegister(tcp_connection_t *conn){
    
	tcp_connection = conn;
	
}

eat_bool tcp_init(tcp_connection_t *conn){
	
	eat_soc_notify_register(soc_notify_cb);

}

eat_bool tcp_getServerIpAddr(const char* domain, u8* ipAddr, eat_hostname_notify call_back)
{
    s8 result = 1;
    u8 addrLen = 4;
    eat_soc_gethost_notify_register(call_back);
    result = eat_soc_gethostbyname((char *)domain, ipAddr, &addrLen, 2212);
    eat_trace("INFO: Get server IP Address");
    switch(result)
    {
        case SOC_SUCCESS:
            eat_trace("INFO: Get server IP Address success!");
            return EAT_TRUE;
        case SOC_INVAL:
            eat_trace("ERROR: Invalid arguments: null domain_name, etc.!");
            return EAT_FALSE;
        case SOC_WOULDBLOCK:
            eat_trace("FATAL: Wait response from network");
            return EAT_FALSE;
        case SOC_ERROR:
            eat_trace("ERROR: Get IP address failure!");
            return EAT_FALSE;
        default:
            return EAT_FALSE;
    }
}

eat_bool tcp_connect(tcp_connection_t *conn)
{	
    s8 result;
    u8 val = 0;
    u32 VAL;
    
//    sockaddr_struct host;
    
//    server.sock_type = SOC_SOCK_STREAM;
//    server.port = port;
//    server.addr_len = 4;
//    
    //memcpy(server.addr, ipAddr, server.addr_len);
	//host = server;
	eat_trace("INFO: tcp connect to serverAddr = %d.%d.%d.%d, port = %d",conn->server.addr[0], conn->server.addr[1], conn->server.addr[2], conn->server.addr[3], conn->server.port);
    conn->connection_id = eat_soc_create(SOC_SOCK_STREAM, 0);
    if (conn->connection_id < 0)
        {
            eat_trace("ERROR: eat_soc_create return error, result = %d", conn->connection_id);	
            return EAT_FALSE;
        }
        
    val = (SOC_READ | SOC_WRITE | SOC_CLOSE | SOC_CONNECT|SOC_ACCEPT);
    result = eat_soc_setsockopt(conn->connection_id,SOC_ASYNC,&val,sizeof(val));
    if (result != SOC_SUCCESS)
                eat_trace("eat_soc_setsockopt 1 return error :%d",result);
              
    val = TRUE;
    result = eat_soc_setsockopt(conn->connection_id, SOC_NBIO, &val, sizeof(val));
    if (result != SOC_SUCCESS)
           eat_trace("eat_soc_setsockopt 2 return error :%d",result);
    
    val = TRUE;
    result = eat_soc_setsockopt(conn->connection_id, SOC_NODELAY, &val, sizeof(val));
    if (result != SOC_SUCCESS)
           eat_trace("eat_soc_setsockopt 3 return error :%d",result);

    result = eat_soc_getsockopt(conn->connection_id, SOC_NODELAY, &VAL, sizeof(VAL));
    if (result != SOC_SUCCESS)
         eat_trace("eat_soc_getsockopt  return error :%d",result);
      else 
         eat_trace("eat_soc_getsockopt return %d", val);
    
    result = 0;
    result = eat_soc_connect(conn->connection_id, &conn->server);
    if (result < 0 )
        {
        	switch(result)
        	{
        	case SOC_INVAL:
				eat_trace("ERROR: tcp_connect address is null");
				return EAT_FALSE;
			case SOC_INVALID_SOCKET:
				eat_trace("ERROR: tcp_connect invaild socket");
				return EAT_FALSE;
			case SOC_WOULDBLOCK:
				eat_trace("INFO: connecting...");
                return EAT_TRUE;
			case SOC_ERROR:
    			eat_trace("ERROR: eat_soc_connect return error, result = %d",result);
    			return EAT_FALSE;
        	}	
        }else {
        	    eat_trace("INFO: Have new connection with HostAddr = %d.%d.%d.%d, socket_id = %d",
                conn->server.addr[0], conn->server.addr[1], conn->server.addr[2], conn->server.addr[3],
                conn->connection_id);	
   				return EAT_TRUE;
        }

}

eat_bool tcp_disconnect(tcp_connection_t *conn)
{	
	s8 ret;
	eat_trace("INFO: disconnect to server");
	ret = eat_soc_close(conn->connection_id);
	if(ret == SOC_INVALID_SOCKET )
	{
		eat_trace("ERROR: tcp_disconnect failure, socket invaild");
		return EAT_FALSE;
	}else 
		return EAT_TRUE;
}

eat_bool tcp_sendDataToServer(tcp_connection_t *conn, u8 *data, int dataLen)
{	
	s32 result;
	//u8 dataBuf[500] = {0};
	//strncpy(dataBuf, data, dataLen);
	
	result = eat_soc_send(conn->connection_id, data, (s32) dataLen);
	if (result <0)
	{
		switch(result)
		{
			case SOC_INVALID_SOCKET:
				eat_trace("ERROR: tcp_sendDataToServer invaild socket");
				return EAT_FALSE;
			case SOC_BEARER_FAIL:
				eat_trace("ERROR: tcp_sendDataToServer bearer broken");
				//tcp_connect(conn);
				return EAT_FALSE;
			case SOC_MSGSIZE:
				eat_trace("ERROR: tcp_sendDataToServer data is too long");
				return EAT_FALSE;
			case SOC_ERROR:
				eat_trace("ERROR: tcp_sendDataToServer unknow error, result=%d", result);
				return EAT_FALSE;
			case SOC_PIPE:
				eat_trace("ERROR: tcp_sendDataToServer broken pipe => reconnect");
				//tcp_connect(conn);
				return EAT_TRUE;
			case SOC_NOTCONN:
				eat_trace("ERROR: tcp_sendDataToServer socket is not connected, socket_id=%d, result=%d", socket_id, result);
				//tcp_connect(conn);
				return EAT_FALSE;
			default:
				eat_trace("ERROR: tcp_sendDataToServer unknow error, result=%d", result);
				//tcp_connect(conn);
				return EAT_FALSE;
		}
		   }else 
		    {
			 //eat_trace("INFO: tcp send success, result =%d", result);
			 return EAT_TRUE;
			}	
}

void soc_notify_cb(s8 s,soc_event_enum event,eat_bool result, u16 ack_size)
{
    u8 id = 0;
    if(event&SOC_READ) {
        socket_id = s;
       // eat_trace("INFO: socket read, id = %d", s);
        if(tcp_connection->connection_id == s){
        	tcp_connection->msg_cb(s);
        }
    }
    else if (event&SOC_WRITE) { 
    	id = 1;
    }
    else if (event&SOC_ACCEPT) id = 2;
    else if (event&SOC_CONNECT) 
    {
    	//eat_trace("INFO: Socket connect");
    	tcp_connection->connSuccess_cb(s);
    	isTcpConnected = EAT_TRUE;	
    	
    }
    else if (event&SOC_CLOSE){ id = 4;
        eat_soc_close(s);
    }
    else if (event&SOC_ACKED) 
    {
    	//eat_trace("INFO: Socket ACK");
    }

}



	