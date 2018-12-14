
#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

//RTK8710 head file
#include "FreeRTOS.h"
#include "semphr.h"
#include "cmsis_os.h"
//Adapter head file

#include "log.h"
#include "datatype.h"
#include "protocol_config.h"

#define QUEUE_DEBUG_PRINTF(fmt, arg...)  log_debug0("<%s:%d>:"fmt, __FUNCTION__, __LINE__,  ##arg)

#define QUEUE_SUCCESS   (1)     /*返回值，1表示成功(正确)*/
#define QUEUE_FAILURE   (0)     /*0表示失败(错误)*/

typedef UINT8 (* queue_recv_callback)(void *arg,UINT32 handle);

typedef struct Mes_QueueInfo
{
    UINT8* queue_name;
    xTaskHandle xqueueTask;
    queue_recv_callback queue_recv_cb_t;
    xQueueHandle message_queue_handle_t;
}Message_Queue_Info_t;

UINT8 Message_Queue_Semaphore_Start( QueueHandle_t xQueue,TickType_t xTicksToWait);
UINT8 Message_Queue_Semaphore_Stop( QueueHandle_t xQueue);
UINT8 Message_Queue_QueueReceive(QueueHandle_t xQueue,void *pvBuffer,TickType_t xTicksToWait);
UINT8 Message_Queue_QueueSend(QueueHandle_t xQueue,const void * pvItemToQueue,TickType_t xTicksToWait);
void* Message_Queue_Create_xSemaphoreCreateMutex(void);
void Message_Queue_Delete_xSemaphoreCreateMutex( QueueHandle_t  xSemaphore);
UINT8 Message_StartLoop(Message_Queue_Info_t* Message_Queue_Info);
void Message_StopLoop(Message_Queue_Info_t* Message_Queue_Info);
void* Message_Queue_Create(queue_recv_callback cb,UINT8* queue_name,UBaseType_t uxQueueLength,UBaseType_t uxItemSize);
void Message_Queue_Delete(Message_Queue_Info_t* Message_Queue_Info);


#endif

