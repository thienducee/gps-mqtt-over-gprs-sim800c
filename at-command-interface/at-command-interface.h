/*
 * =====================================================================================
 *
 *       Filename:  at-command-interface.h
 *
 *    Description:  the head file
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
#ifndef __AT_COMMAND_INTERFACE_H__
#define __AT_COMMAND_INTERFACE_H__

/** 
 *  MACROS
 */
#define URC_QUEUE_MAX 10                      	/* global urc count */
#define AT_COMMAND_QUEUE_MAX 50                 /* at command count */
#define AT_CMD_EXECUTE_DELAY  10                /* 10 ms use EAT_TIMER_1*/
#define AT_CMD_EXECUTE_OVERTIME 60000           /* 60s use EAT_TIMER_2*/

#define AT_CMD_DELAY "DELAY:"
#define AT_CMD_END "\x0d\x0a\0"
#define AT_CMD_CR  '\x0d'
#define AT_CMD_LF  '\x0a'
#define AT_CMD_CTRL_Z "\x1a"

#define DELAY_TIMER     EAT_TIMER_1
#define TIMEOUT_TIMER   EAT_TIMER_2

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) > (b) ? (b) : (a))
#define MODEM_READ_BUFFER_LEN 2048
#define AT_CMD_ARRAY_SHORT_LENGTH 30
#define AT_CMD_ARRAY_MID_LENGTH 130
#define AT_CMD_ARRAY_LONG_LENGTH 300

/**
 *  TYPE DEFINITIONS
 */
 typedef enum
{
    AT_RSP_ERROR = -1,
    AT_RSP_WAIT= 0, 
    AT_RSP_CONTINUE = 1,                        /* Continue to execute the next AT command in AT command queue*/
    AT_RSP_PAUSE= 2,                            /* Suspend to execute the AT command queue*/
    AT_RSP_FINISH = 3,                          /* Stop executing the AT command queue */

    AT_RSP_FUN_OVER = 4,                        /* Finish the current AT command queue, then clear the AT command queue*/
    AT_RSP_STEP_MIN = 10,
    AT_RSP_STEP = 20,                           /*Continue to execute current AT command */
    AT_RSP_STEP_MAX = 30,

}AtCmdRsp_enum_t;

#define MODEM_CALL_READY  1
#define MODEM_SMS_READY   2
/**
 *  Callback functions type
 */
typedef AtCmdRsp_enum_t (*AtCmdCheckRspCallback_t)(u8* modemRspString);
typedef void (*ResultNotifyCallback_t)(eat_bool result);
typedef void (*ModemCallback_t)(u8 *pModemString, u16 len);

/**
 *  Structs
 */
typedef struct __AtCmdElement
{
    u8* p_atCmdString;
    u8* p_expectedString;
    u16 CmdLen;
    ResultNotifyCallback_t pResultNotifyCallback;
} AtCmdElement_t;


/**
 * @brief AT-Command interface task
 *
 * @func AtCmd_interfaceThread()
 *
 * @param data
 * @return none
 */
void AtCmd_interfaceTask(void *data);

/**
 * @brief check queue list is emplty yet?
 *
 * @func AtCmd_queueListIsEmpty()
 *
 * @return boolen result
 *                - 0 if 
 */
eat_bool AtCmd_queueListIsEmpty(void);

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
eat_bool AtCmd_queueAppend(AtCmdElement_t atCmdElement);

eat_bool AtCmd_gsm_init(ResultNotifyCallback_t pResultNotifyCallback);
eat_bool AtCmd_groupQueueAppend(AtCmdElement_t *atCmdElement,u8 groupCount);
void AtCmd_registerModemReadyCallback(ResultNotifyCallback_t pCallback);

eat_bool AtCmd_queueAppendCmd(u8 *cmdString, 
							  u8* expectedString, 
							  u16 cmdLen, 
							  ResultNotifyCallback_t pCallback);


#endif /* __AT_COMMAND_INTERFACE_H */ 
