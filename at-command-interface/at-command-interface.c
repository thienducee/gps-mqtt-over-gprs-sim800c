/*
 * =====================================================================================
 *
 *       Filename:  at-command-interface.c
 *
 *    Description:  the source file
 *
 *        Version:  1.0
 *        Created:  2017-6-18 15:58:20
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Le Thien Duc, thienduc.ee@gmail.com
 *   Organization:  none
 *
 * =====================================================================================
 */
 #include "stdio.h"
 #include "string.h"
 #include "eat_interface.h"
 #include "eat_type.h"
 #include "at-command-interface.h"
 
/**
 *  Structs
 *
 */
typedef struct __AtCmdQueueList
{
    u8 currentCmd;              /* Current Cmd is excecuting */
    u8 lastCmd;                 /* Last Cmd executive */
    AtCmdElement_t atCmdElementArray[AT_COMMAND_QUEUE_MAX]; 
} AtCmdQueueList_t;

/**
 * Functions prototype
 */
static AtCmdRsp_enum_t AtCmd_CheckRspCallback(u8* modeRspString);
static void AtCmd_queueList_init(void);
static eat_bool AtCmd_delayExe(u16 delay);
static eat_bool AtCmd_timeOutStart(void);
static eat_bool AtCmd_timeOutStop(void);
static eat_bool AtCmd_deleteQueueHead(void);
static eat_bool AtCmd_deleteQueueTail(void);
static void AtCmd_delayTimer_handler(void);
static void AtCmd_modemRsp_handler(void);
 
static AtCmdQueueList_t atCmdQueueList = {0};
static u8 modem_read_buf[MODEM_READ_BUFFER_LEN]; 
static u8* s_atCmdWriteAgain = NULL;
static u8 s_atExecute = 0;
static u16 modem_state_ready = 0;

static ResultNotifyCallback_t pModemReadyCallback = NULL;

/**
 * @brief AT-Command interface task
 *
 * @func AtCmd_interfaceThread()
 *
 * @param data
 * @return none
 */
void AtCmd_interfaceTask(void *data)
{
    EatEvent_st event;
    while(EAT_TRUE)
    {
        eat_get_event_for_user(EAT_USER_1, &event);
        switch(event.event)
        {
            case EAT_EVENT_TIMER :
                
                switch ( event.data.timer.timer_id ) {
                    case DELAY_TIMER:           /* delay to excute AT-Command */
                        eat_trace("INFO: DELAY_TIMER expire!");
                        AtCmd_delayTimer_handler();
                        break;

                    case TIMEOUT_TIMER:           /* AT-Command time out */
						AtCmd_queueList_init();
                    	AtCmd_timeOutStop();
						if(atCmdQueueList.atCmdElementArray[atCmdQueueList.currentCmd].pResultNotifyCallback)
						{
							atCmdQueueList.atCmdElementArray[atCmdQueueList.currentCmd].pResultNotifyCallback(EAT_FALSE);
							atCmdQueueList.atCmdElementArray[atCmdQueueList.currentCmd].pResultNotifyCallback = NULL;
						}
                        eat_trace("FATAL: At-Command is time out!");
                        break;

                    default:	
                        break;
                }				/* -----  end switch  ----- */
                break;

            case EAT_EVENT_MDM_READY_RD:
                AtCmd_modemRsp_handler();
                break;

            case EAT_EVENT_MDM_READY_WR:
                if (s_atCmdWriteAgain){
                    u16 lenAct = 0;
                    u16 len = strlen((const char*)s_atCmdWriteAgain);
                    lenAct = eat_modem_write(s_atCmdWriteAgain,len);
                    eat_mem_free(s_atCmdWriteAgain);
                    if(lenAct<len){
                        eat_trace("FATAL: modem write buffer is overflow!");
                    }

                }
                break;
            case EAT_EVENT_USER_MSG:
      			AtCmd_delayExe(0);
                break;
            default:
                break;

        }
    }
}
/**
 * @brief register Modem Ready Callback
 *
 * @func AtCmd_registerModemReadyCallback()
 *
 * @param  atCmdElement an element to append
 * @return boolen result
 *                - 0 if error
 *                - 1 if success
 */
void AtCmd_registerModemReadyCallback(ResultNotifyCallback_t pCallback)
{
	pModemReadyCallback = pCallback;
}
/**
 * @brief init queue list
 *
 * @func AtCmd_queueList_init()
 *
 * @param  atCmdElement an element to append
 * @return boolen result
 *                - 0 if error
 *                - 1 if success
 */
void AtCmd_queueList_init(void)
{
    s16 i;
    u8 first = atCmdQueueList.currentCmd;
    for(i = first; i <= atCmdQueueList.lastCmd; i++)
    {
        AtCmdElement_t *p_atCmdElement = 
        &(atCmdQueueList.atCmdElementArray[i]);
        if (p_atCmdElement->p_atCmdString != NULL)
        {
            eat_mem_free(p_atCmdElement->p_atCmdString);
            p_atCmdElement->p_atCmdString = NULL;
        }
        p_atCmdElement->CmdLen = 0;
        p_atCmdElement->pResultNotifyCallback = NULL;
    }
    memset(&atCmdQueueList, 0, sizeof(atCmdQueueList));
    eat_trace("AtCommand queue list init");
}
/**
 * @brief Handler all respone form modem
 *
 * @func AtCmd_modemRsp_handler()
 *
 * @param  atCmdElement an element to append
 * @return boolen result
 *                - 0 if error
 *                - 1 if success
 */
static void AtCmd_modemRsp_handler(void)
{
    AtCmdRsp_enum_t rspValue = AT_RSP_ERROR;
    u16 lenght = eat_modem_read(modem_read_buf,
                                MODEM_READ_BUFFER_LEN);
    if (lenght > 0)
    {
        memset(modem_read_buf+lenght, 0, MODEM_READ_BUFFER_LEN - lenght);
        /* Check response form modem */
        eat_trace("ReFrModem:%s,%d", modem_read_buf, lenght);

           rspValue = AtCmd_CheckRspCallback(modem_read_buf);
           eat_trace("INFO: AtCmd_CheckRspCallback of the currentCmd = %d, result = %d",
                     atCmdQueueList.currentCmd, rspValue);
           switch(rspValue)
           {
                case AT_RSP_ERROR:
                     eat_trace("ERROR: Cannot execute command %s AtCmd init again", 
                                atCmdQueueList.atCmdElementArray[atCmdQueueList.currentCmd].p_atCmdString);
                     AtCmd_queueList_init();
                     AtCmd_timeOutStop();
                     break;

                case AT_RSP_FINISH:
                     AtCmd_deleteQueueHead();
                     AtCmd_timeOutStop();
                     AtCmd_delayExe(0);
                     break;

                case AT_RSP_WAIT:
                     break;

                default:

                     break;
           }
    }
     
}

/**
 * @brief Append an AtCmd element to queue list
 *
 * @func AtCmd_queueAppend()
 *
 * @param  atCmdElement an element to append
 * @return boolen result
 *                - 0 if error
 *                - 1 if success
 */
eat_bool AtCmd_queueAppend(AtCmdElement_t atCmdElement)
{
    /* get first index */
    u8 first =  atCmdQueueList.currentCmd;

    if (atCmdElement.p_atCmdString == NULL){
        eat_trace("ERROR: AT-Command string is null!");
        return EAT_FALSE;
    }

    if((atCmdQueueList.lastCmd + 1) % AT_COMMAND_QUEUE_MAX == first){
        eat_trace("ERROR: AT-Command queue list is full!");
        return EAT_FALSE;  /* the queue is full */
    }
    else{
        u8* pAtCmd = NULL; ;//= eat_mem_alloc(atCmdElement.CmdLen);

        pAtCmd = eat_mem_alloc(atCmdElement.CmdLen);

        if (!pAtCmd){
            eat_trace("ERROR: memory alloc error!");
            return EAT_FALSE;
        }

        memcpy(pAtCmd,  atCmdElement.p_atCmdString, atCmdElement.CmdLen);
        atCmdQueueList.atCmdElementArray[atCmdQueueList.lastCmd].CmdLen = 
        atCmdElement.CmdLen;
        atCmdQueueList.atCmdElementArray[atCmdQueueList.lastCmd].p_atCmdString = pAtCmd;
        atCmdQueueList.atCmdElementArray[atCmdQueueList.lastCmd].pResultNotifyCallback = 
        atCmdElement.pResultNotifyCallback;

        if(AtCmd_queueListIsEmpty() && s_atExecute == 0) /* add first at cmd and execute it */
            eat_send_msg_to_user(0, EAT_USER_1, EAT_FALSE, 1, 0, NULL);

        atCmdQueueList.lastCmd = (atCmdQueueList.lastCmd + 1) %  AT_COMMAND_QUEUE_MAX;
        return EAT_TRUE;
    }
}

eat_bool AtCmd_queueAppendCmd(u8 *cmdString, 
							  u8* expectedString, 
							  u16 cmdLen, 
							  ResultNotifyCallback_t pCallback)
{
	AtCmdElement_t AtCmdElement;
    AtCmdElement.p_atCmdString = cmdString;
    AtCmdElement.p_expectedString = expectedString;
    AtCmdElement.CmdLen = cmdLen;
    AtCmdElement.pResultNotifyCallback = pCallback;
    if(AtCmd_queueAppend(AtCmdElement))
    {
        return EAT_TRUE;
    }
    else
    {
        eat_trace("ERROR: Cannot append command:%s", cmdString);
        return EAT_FALSE;
    } 
}

/**
 * @brief append an group Queue command to the queue list
 *
 * @func AtCmd_groupQueueAppend()
 *
 * @param atCmdElement
 * @param groupCount
 * @param callback
 * @return boolen result
 *                - 0 if the queue list not empty
 *                - 1 if the queue list empty
 */
eat_bool AtCmd_groupQueueAppend(AtCmdElement_t *atCmdElement,u8 groupCount)
{
    u8 i = 0;
    for ( i=0; i < groupCount; i++) {
        eat_bool ret = EAT_FALSE;
        ret = AtCmd_queueAppend(atCmdElement[i]);
        if (!ret){
            break;
        }
    }

    if(i != groupCount)
    {   /* error is ocur */
        for ( i = groupCount; i > 0; i--) {
            AtCmd_deleteQueueTail();
        }
        eat_trace("ERROR: add to queue is error!,%d",groupCount);
        return EAT_FALSE;
    }

    eat_trace("INFO: CmdCurrent:%d  CmdLast:%d", 
              atCmdQueueList.currentCmd, 
              atCmdQueueList.lastCmd);
    return EAT_TRUE;
}

/**
 * @brief check queue list is emplty yet?
 *
 * @func AtCmd_queueListIsEmpty()
 *
 * @return boolen result
 *                - 0 if the queue list not empty
 *                - 1 if the queue list empty
 */
eat_bool AtCmd_queueListIsEmpty(void)
{
    return (eat_bool)(atCmdQueueList.currentCmd == atCmdQueueList.lastCmd);
}

/**
 * @brief delete the head of the queue 
 *
 * @func AtCmd_deleteQueueHead()
 *
 * @return boolen result
 */
static eat_bool AtCmd_deleteQueueHead(void)
{
   AtCmdElement_t* atCmdEnt = NULL;
   /* the queue is empty */
   if(atCmdQueueList.currentCmd == atCmdQueueList.lastCmd) 
   {
       return EAT_FALSE;
   }

       atCmdEnt = &(atCmdQueueList.atCmdElementArray[atCmdQueueList.currentCmd]);
	   
       if(atCmdEnt->p_atCmdString)
       {
       	   memset(atCmdEnt->p_atCmdString, 0, atCmdEnt->CmdLen );
           eat_mem_free(atCmdEnt->p_atCmdString);
           atCmdEnt->p_atCmdString = NULL;
       }
       atCmdEnt->pResultNotifyCallback = NULL;

   atCmdQueueList.currentCmd = (atCmdQueueList.currentCmd + 1) %  AT_COMMAND_QUEUE_MAX;
   return EAT_TRUE;
}

/**
 * @brief delete the tail of the queue 
 *
 * @func AtCmd_deleteQueueTail()
 *
 * @return boolen result
 */
static eat_bool AtCmd_deleteQueueTail(void)
{
    AtCmdElement_t* atCmdEnt = NULL;
    if(atCmdQueueList.currentCmd == atCmdQueueList.lastCmd)
    {
        return EAT_FALSE;
    }

    atCmdQueueList.lastCmd = 
    (atCmdQueueList.lastCmd + AT_COMMAND_QUEUE_MAX - 1) % AT_COMMAND_QUEUE_MAX;
    atCmdEnt = &(atCmdQueueList.atCmdElementArray[atCmdQueueList.lastCmd]);

    if(atCmdEnt->p_atCmdString)
    {
    	memset(atCmdEnt->p_atCmdString, 0, atCmdEnt->CmdLen );
        eat_mem_free(atCmdEnt->p_atCmdString);
        atCmdEnt->p_atCmdString = NULL;
    }

    atCmdEnt->pResultNotifyCallback = NULL;
    return EAT_TRUE;
}

/**
 * @brief delay timer handler
 *
 * @func AtCmd_delayTimer_handler()
 *
 * @return none
 */
static void AtCmd_delayTimer_handler(void)
{
    u8* pCmd = atCmdQueueList.atCmdElementArray[atCmdQueueList.currentCmd].p_atCmdString;
    u16 len = atCmdQueueList.atCmdElementArray[atCmdQueueList.currentCmd].CmdLen;
    s_atExecute = 0;

    if(pCmd){
        u16 lenAct = 0;
        eat_trace("DEBUG:at cmd is:%d,%s",atCmdQueueList.currentCmd,pCmd);
        if (!strncmp(AT_CMD_DELAY, (const char*)pCmd, strlen(AT_CMD_DELAY))) {  /* delay some seconds to run next cmd */
            u32 delay;
            sscanf((const char*)pCmd, AT_CMD_DELAY"%d", &delay);
            AtCmd_deleteQueueHead();
            AtCmd_delayExe(delay);
            return;
        }
        lenAct = eat_modem_write(pCmd,len);
        if(lenAct<len){
            eat_trace("ERROR: modem write buffer is overflow!");

            if(s_atCmdWriteAgain == NULL){
                s_atCmdWriteAgain = eat_mem_alloc(len-lenAct);
                if (s_atCmdWriteAgain)
                    memcpy(s_atCmdWriteAgain,pCmd+lenAct,len-lenAct);
                else
                    eat_trace("ERROR: mem alloc error!");
            }
            else 
                eat_trace("FATAL: EAT_EVENT_MDM_READY_WR may be lost!");
        }
        else{
            eat_trace("WrToModem:%s",pCmd);
            AtCmd_timeOutStart();
        }

    }
}

/**
 * @brief check response string form modem
 *
 * @func AtCmd_delayExe()
 *
 * @param delay is time to delay
 * @return response value
 */
static eat_bool AtCmd_delayExe(u16 delay)
{
    eat_bool result = EAT_FALSE;
    eat_trace("INFO: at cmd delay execute,%d",delay);
    if (delay == 0){
        delay = AT_CMD_EXECUTE_DELAY;
    }
    result = eat_timer_start(DELAY_TIMER, delay);
    if (result)
        s_atExecute = 1;
    return result;
}

/**
 * @brief start timeout timer
 *
 * @func AtCmd_overTimeStart()
 *
 * @return response value
 */
static eat_bool AtCmd_timeOutStart(void)
{
    eat_bool result = EAT_FALSE;
    result = eat_timer_start(TIMEOUT_TIMER, AT_CMD_EXECUTE_OVERTIME);
    return result;
}

/**
 * @brief stop timeout timer
 *
 * @func AtCmd_overTimeStop()
 *
 * @return response value
 */
static eat_bool AtCmd_timeOutStop(void)
{
    eat_bool result = EAT_FALSE;
    result = eat_timer_stop(TIMEOUT_TIMER);
    return result;
}

/**
 * @brief check response string form modem
 *
 * @func AtCmd_CheckRspCallback()
 *
 * @param modemRspString Response string form the modem
 * @return response value
 */
static AtCmdRsp_enum_t 
AtCmd_CheckRspCallback(u8 *modemRspString)
{
    AtCmdRsp_enum_t rspValue = AT_RSP_WAIT;
    u8* rspStringTable[] = {"OK", "ERROR", "Call Ready", "SMS Ready"};
    s16 rspType = -1;
    u8 i = 0;
    u8* p_modemRspString = modemRspString;

    while(p_modemRspString)
    {
        /* ignore \r \n */
        while(*p_modemRspString == AT_CMD_CR || *p_modemRspString == AT_CMD_LF)
        {
            p_modemRspString++;
        }
        if (atCmdQueueList.atCmdElementArray[atCmdQueueList.currentCmd].p_expectedString == NULL)
        {
        	/* modem respone string compare with string table */
        	for(i = 0; i < sizeof(rspStringTable) / sizeof(rspStringTable[0]); i++ )
        	{
            	if(!strncmp((const char*)rspStringTable[i], (const char*)p_modemRspString,
                	strlen((const char*)rspStringTable[i])))
            	{
                	rspType = i;
               	 	break;
            	}
        	}
        }else
        	{
        		u8* p_expectedString = atCmdQueueList.atCmdElementArray[atCmdQueueList.currentCmd].p_expectedString;
        		/* modem respone string compare with expected String */		
        		if(!strncmp((const char*)p_expectedString, 
        					(const char*)p_modemRspString,
                			strlen((const char*)p_expectedString[i])))
            	{
                	rspType = i;
               	 	break;
            	}
        	}
		p_modemRspString = (u8*)strchr((const char*)p_modemRspString,0x0a);
	  }
      switch(rspType)
        {
            case 0:
                rspValue = AT_RSP_FINISH;
                    if(atCmdQueueList.atCmdElementArray[atCmdQueueList.currentCmd].pResultNotifyCallback)
		        {
			        atCmdQueueList.atCmdElementArray[atCmdQueueList.currentCmd].pResultNotifyCallback(EAT_TRUE);
			        atCmdQueueList.atCmdElementArray[atCmdQueueList.currentCmd].pResultNotifyCallback = NULL;
		        }
                break;
            case 1:
                rspValue = AT_RSP_ERROR;
                if(atCmdQueueList.atCmdElementArray[atCmdQueueList.currentCmd].pResultNotifyCallback)
		        {
			        atCmdQueueList.atCmdElementArray[atCmdQueueList.currentCmd].pResultNotifyCallback(EAT_FALSE);
			        atCmdQueueList.atCmdElementArray[atCmdQueueList.currentCmd].pResultNotifyCallback = NULL;
		        }
                break;
            case 2:
            	break;
            case 3:
            	pModemReadyCallback(EAT_TRUE);
            	break;
            default:
                rspValue = AT_RSP_WAIT;
                break;
        }
    return rspValue;
}
