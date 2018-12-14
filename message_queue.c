
/******************************************************************************
**文 件 名: message_queue.c
**作     者: wqw
**生成日期: 2018年11月23日
**功能描述: 使用队列发送数据
******************************************************************************/
//Standard head file
#include <string.h>
#include <stdio.h>
//RTK8710 head file
#include "FreeRTOS.h"
#include "semphr.h"
#include "cmsis_os.h"
#include "task.h"
//IOT SDK head

//Adapter head file
#include "message_queue.h"

#define QUEUE_BUF_SIZE        (128)       /*消息队列缓冲区大小*/

/*****************************************************************************
**函 数 名: Message_Queue_Semaphore_Start
**输入参数:
**输出参数: 无
**返 回 值:
**功能描述: 消息队列使用信号量加锁
**作     者: wqw
*****************************************************************************/
UINT8 Message_Queue_Semaphore_Start( QueueHandle_t xQueue,TickType_t xTicksToWait)
{
    xSemaphoreTake(xQueue, xTicksToWait);
    return QUEUE_SUCCESS;
}

/*****************************************************************************
**函 数 名: Message_Queue_Semaphore_Stop
**输入参数:
**输出参数: 无
**返 回 值:
**功能描述: 消息队列使用信号量解锁
**作     者: wqw
*****************************************************************************/
UINT8 Message_Queue_Semaphore_Stop( QueueHandle_t xQueue)
{
    xSemaphoreGive(xQueue);
    return QUEUE_SUCCESS;
}

/*****************************************************************************
**函 数 名: Message_Queue_QueueReceive
**输入参数: *
 *@param xQueue The handle to the queue from which the item is to be
 * received.
 *
 * @param pvBuffer Pointer to the buffer into which the received item will
 * be copied.
 *
 * @param xTicksToWait The maximum amount of time the task should block
 * waiting for an item to receive should the queue be empty at the time
 * of the call.	 xQueueReceive() will return immediately if xTicksToWait
 * is zero and the queue is empty.  The time is defined in tick periods so the
 * constant portTICK_PERIOD_MS should be used to convert to real time if this is
 * required.
**输出参数: 无
**返 回 值:
* @return pdTRUE if an item was successfully received from the queue,
* otherwise pdFALSE.

**功能描述: 消息队列接收接口
**作     者: wqw
*****************************************************************************/
UINT8 Message_Queue_QueueReceive(QueueHandle_t xQueue,void *pvBuffer,TickType_t xTicksToWait)
{

    if(pdTRUE==xQueueReceive(xQueue,(void * )pvBuffer,xTicksToWait))
        return QUEUE_SUCCESS;
    else
        return QUEUE_FAILURE;
}
/*****************************************************************************
**函 数 名: Message_Queue_QueueSend
**输入参数: *
 * @param xQueue The handle to the queue on which the item is to be posted.
 *
 * @param pvItemToQueue A pointer to the item that is to be placed on the
 * queue.  The size of the items the queue will hold was defined when the
 * queue was created, so this many bytes will be copied from pvItemToQueue
 * into the queue storage area.
 *
 * @param xTicksToWait The maximum amount of time the task should block
 * waiting for space to become available on the queue, should it already
 * be full.  The call will return immediately if this is set to 0 and the
 * queue is full.  The time is defined in tick periods so the constant
 * portTICK_PERIOD_MS should be used to convert to real time if this is required.
 *
**输出参数: 无
**返 回 值:
* @return pdTRUE if the item was successfully posted, otherwise errQUEUE_FULL.
**功能描述: 消息队列发送接口
**作     者: wqw
*****************************************************************************/
UINT8 Message_Queue_QueueSend(QueueHandle_t xQueue,const void * pvItemToQueue,TickType_t xTicksToWait)
{
    if(pdTRUE==xQueueSend(xQueue,pvItemToQueue,xTicksToWait))
        return QUEUE_SUCCESS;
     else
        return QUEUE_FAILURE;
}

/*****************************************************************************
**函 数 名: Message_Queue_Create_xSemaphoreCreateMutex
**输入参数:
**输出参数: 无
**返 回 值:
**功能描述: 创建一个互斥信号量
**作     者: wqw
*****************************************************************************/
void* Message_Queue_Create_xSemaphoreCreateMutex(void)
{
    xSemaphoreHandle send_data_mutex;

    send_data_mutex = xSemaphoreCreateMutex();
    if(NULL == send_data_mutex)
    {
        return QUEUE_FAILURE;
    }
    return send_data_mutex;
}
/*****************************************************************************
**函 数 名: Message_Queue_Delete_xSemaphoreCreateMutex
**输入参数:
**输出参数: 无
**返 回 值:
**功能描述: 删除一个互斥信号量
**作     者: wqw
*****************************************************************************/
void Message_Queue_Delete_xSemaphoreCreateMutex( QueueHandle_t  xSemaphore)
{
    vSemaphoreDelete(xSemaphore);
}

/*****************************************************************************
**函 数 名: Message_Queue_Receive_Handle_Task
**输入参数:
**输出参数: 无
**返 回 值:
**功能描述: 消息队列接收业务层数据
**作     者: wqw
*****************************************************************************/
void Message_Queue_Receive_Handle_Task(void *arg)
{
    UINT8 receive_quent_data[QUEUE_BUF_SIZE]={0};
    Message_Queue_Info_t* Message_Queue_Info=(Message_Queue_Info_t*)arg;

    while(1)
    {
         if(pdTRUE==Message_Queue_QueueReceive(Message_Queue_Info->message_queue_handle_t,(void * )receive_quent_data,0)){
            (Message_Queue_Info->queue_recv_cb_t)(receive_quent_data,(UINT32)Message_Queue_Info->message_queue_handle_t);
            memset(receive_quent_data, 0, sizeof(receive_quent_data));
        }else{
            vTaskDelay(100/portTICK_RATE_MS);
        }
    }
}

/*****************************************************************************
**函 数 名: Message_StartLoop
**输入参数:Message_Queue_Info_t* Message_Queue_Info,
**UINT8* queue_name task的名字
**输出参数: 无
**返 回 值:
**功能描述: 创建task
**作     者: wqw
*****************************************************************************/
UINT8 Message_StartLoop(Message_Queue_Info_t* Message_Queue_Info)
{
    QUEUE_DEBUG_PRINTF("queue_name=%s--Message_Loop\n",Message_Queue_Info->queue_name);

    if(pdPASS != xTaskCreate(Message_Queue_Receive_Handle_Task, Message_Queue_Info->queue_name, 512,(void*)Message_Queue_Info, 4,&Message_Queue_Info->xqueueTask))
    {
        return QUEUE_FAILURE;
    }
    return QUEUE_SUCCESS;
}

/*****************************************************************************
**函 数 名: Message_StopLoop
**输入参数:Message_Queue_Info_t* Message_Queue_Info,UINT8* queue_name
**输出参数: 无
**返 回 值:
**功能描述: 消息队列初始化
**作     者: wqw
*****************************************************************************/
void Message_StopLoop(Message_Queue_Info_t* Message_Queue_Info)
{
    vTaskDelete(Message_Queue_Info->xqueueTask);
}

/*****************************************************************************
**函 数 名: Message_Queue_Create
**输入参数:queue_recv_callback cb,
**UBaseType_t uxQueueLength,
**UBaseType_t uxItemSize
**输出参数: 无
**返 回 值:
**功能描述: 消息队列初始化
**作     者: wqw
*****************************************************************************/
void* Message_Queue_Create(queue_recv_callback cb,UINT8* queue_name,UBaseType_t uxQueueLength,UBaseType_t uxItemSize)
{
    QUEUE_DEBUG_PRINTF("Message_Queue_Init\n");
    Message_Queue_Info_t* Message_Queue_Info;

    Message_Queue_Info=(Message_Queue_Info_t*)malloc(sizeof(Message_Queue_Info_t));
    if(NULL == Message_Queue_Info)
    {
        log_debug0("Message_Queue_Info_t malloc error.\n");
    }
    Message_Queue_Info->queue_name=queue_name;
    Message_Queue_Info->message_queue_handle_t = xQueueCreate(uxQueueLength, uxItemSize);
    QUEUE_DEBUG_PRINTF("%s-Message_Queue_Info->message_queue_handle_t=%d\n",Message_Queue_Info->queue_name,Message_Queue_Info->message_queue_handle_t);
    if(NULL == Message_Queue_Info->message_queue_handle_t)
    {
        QUEUE_DEBUG_PRINTF("Queue was not created and must not be used.\n");
        return QUEUE_FAILURE;
    }
    Message_Queue_Info->queue_recv_cb_t=cb;

    return Message_Queue_Info;
}

/*****************************************************************************
**函 数 名: Message_Queue_Delete
**输入参数: UINT8 Queue_Type
**输出参数: 无
**返 回 值:
**功能描述: 删除消息队列
**作     者: wqw
*****************************************************************************/
void Message_Queue_Delete(Message_Queue_Info_t* Message_Queue_Info)
{
    vQueueDelete(Message_Queue_Info->message_queue_handle_t);
    free(Message_Queue_Info);
}

