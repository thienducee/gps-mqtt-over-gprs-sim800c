#ifndef SERIALINTERFACE_H_
#define SERIALINTERFACE_H_

#define EAT_UART_RX_BUF_LEN_MAX 2048

/* DATA TYPES */
#define COMMAND_TYPE 		"cmd"
#define INFO_DATA_TYPE    	"info"
/* COMMAND TYPE */
#define CONNECT				"connect"
#define DISCONNECT			"disconnect"
#define SLEEP				"sleep"
#define TIME_UPDATE			"time update"
#define POST_TRACKING_DATA 	"tracking data"

#define MCU_MODE 1

typedef eat_bool (*PostTrackingData_handler_t)(u8 *buf);
typedef eat_bool (*Connect_handler_t)();
typedef eat_bool (*Disconnect_handler_t)(); 

extern const EatUart_enum eat_uart_app;
extern const EatUart_enum eat_uart_debug;
extern const EatUart_enum eat_uart_at;

void serialInterface_init(void);
void serialInterface_registerHandler(void);
void serialInterface_regisConnectHandler(Connect_handler_t pFunction);
void serialInterface_regisDisconnectHandler(Disconnect_handler_t pFunction);
void serialInterface_regisPostTrackingDataHandler(PostTrackingData_handler_t pFunction);
void serialInterface_task(void *data);
void serialInterface_RX_handler(const EatEvent_st* event);

#endif /*SERIALINTERFACE_H_*/
