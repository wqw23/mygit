#ifndef __PRODUCT_CFG_H__
#define __PRODUCT_CFG_H__


#ifndef ENADLE_FACTORY_MODE
//#define ENADLE_FACTORY_MODE
#endif

#define POWER_CONTROL_INDIVIDUAL 1

#define false 0
#define true 1

//wifi led
#define GPIO_LED_WIFI PA_11
#define LED_WIFI_ON  0
#define LED_WIFI_OFF 1
//power led
#define GPIO_LED_POWER  PA_8
#define LED_POWER_ON  0
#define LED_POWER_OFF 1


//uart communication port
#define LEN_UART_TX            PA_23
#define LEN_UART_RX            PA_18
#define LEN_BAUD_RATE          BIT_RATE_9600
#define LEN_STOP_BIT           USART_StopBits_1
#define LEN_PARITY_MODE        USART_Parity_None
#define LEN_FLOW_CTRL          USART_HardwareFlowControl_None
#define LEN_BIT_LEN            UART_WordLength_8b

#endif

