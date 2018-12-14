/******************************************************************************
**文 件 名: protocol.c
**作     者: sdc
**生成日期: 2018年11月11日
**功能描述: 用于打包、解析和发送协议数据
******************************************************************************/
//Standard head file
#include <string.h>
#include <stdio.h>
//RTK8710 head file
#include "FreeRTOS.h"
#include "semphr.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
//IOT SDK head

//Adapter head file
#include "protocol.h"


#ifndef PROTOCOL_MALLOC
    #error "PROTOCOL_MALLOC is undefined, like:#define PROTOCOL_MALLOC(size)  malloc(size)"
#endif
#ifndef PROTOCOL_FREE
    #error "PROTOCOL_FREE is undefined, like:#define PROTOCOL_FREE(prt)  free(ptr)"
#endif
#ifndef PROTOCOL_MEMCPY
    #error "PROTOCOL_MEMCPY is undefined, like:#define PROTOCOL_MEMCPY(dst,src,len)  memcpy(dst,src,len)"
#endif
#ifndef PROTOCOL_SPRINTF
    #error "PROTOCOL_SPRINTF is undefined, like:#define PROTOCOL_SPRINTF(buf, fmt, arg...) sprintf(buf, fmt, ##arg)"
#endif
#ifndef PROTOCOL_PRINTF
    #error "PROTOCOL_PRINTF is undefined, like:#define PROTOCOL_PRINTF(fmt, arg...) printf(fmt, ##arg)"
#endif
#ifndef PROTOCOL_DEBUG_PRINTF
    #error "PROTOCOL_DEBUG_PRINTF is undefined, it can be NULL"
#endif

/*用于表明log中的帧信息是接收还是发送*/
enum
{
    SEND_FRAME_FLG = 0,     /*发送协议帧*/
    RECV_FRAME_FLG,         /*接收协议帧*/
};

/*根据不同的状态解析串口数据*/
enum
{
    FRAME_HEAD_AA_STATE = 0,    /*接收帧头0xAA*/
    FRAME_HEAD_55_STATE,        /*接收帧头0x55*/
    FRAME_TYPE_STATE,           /*接收帧类型*/
    FRAME_LENGTH_STATE,         /*接收帧数据长度*/
    FRAME_SEQUENCE_STATE,       /*接收帧序号*/
    FRAME_BODY,                  /*接收帧(body)内容*/
    FRAME_CRC_STATE       /*接收帧中的校验*/
};

/*解析和存储收到的数据*/
typedef struct
{
    UINT8  recv_state;         /*接收状态*/
    UINT16 temp_len;           /*收到的数据长度记录*/
    FRAME_STRUCT temp_frame;/*暂时存储帧的信息*/
    FRAME_STRUCT* frame;    /*收到的内容*/
}ORIGINAL_DATA_STRUCT;

static xSemaphoreHandle s_Protocol_Package_Lock = NULL;    /*打包时用的锁*/
static xSemaphoreHandle s_Protocol_Printf_Lock = NULL;      /*输出协议信息时使用的锁*/
static ORIGINAL_DATA_STRUCT s_Orig_Data_Handle = {0};    /*存储接收的原始(串口)数据*/
static PROTOCOL_LIST_STRUCT s_Frame_List_Head = {0};        /*存储打包完成的帧,Head中不存储数据*/

#ifdef PROTOCOL_DEBUG
/*****************************************************************************
**函 数 名: _Protocol_Print_Frame
**输入参数: FRAME_STRUCT *frame:帧内容
             UINT8 flg:表示输出的是接收还是发送。0:表示发送；1:表示接收
**输出参数: 无
**返 回 值:
**功能描述: 调用该函数时，如果输出由Protocol_Package_Frame()打包的帧，flg传入0，表示发送;
            对于通过Protocol_Package_Recv_Data()收到的帧，flg传入1，表示接收。
**作     者: wqw
*****************************************************************************/
void _Protocol_Print_Frame(FRAME_STRUCT *frame, UINT8 flg,UINT8 body_len)
{
    UINT16 len = 0,i = 0;
    UINT8 *temp_buf = NULL;
    UINT8 *temp_buf1 = NULL;

    if(NULL == frame)
    {
        PROTOCOL_DEBUG_PRINTF("Para Frame is NULL\n");
        return;
    }

    xSemaphoreTake(s_Protocol_Printf_Lock, portMAX_DELAY);

    len = body_len + PROTOCOL_HEAD_LENGTH;

    temp_buf=(UINT8 *)PROTOCOL_MALLOC(len);

     for(i=0;i<len-PROTOCOL_HEAD_LENGTH;i++){
         temp_buf[i+PROTOCOL_HEAD_LENGTH-1]=frame->body[i];
     }

     temp_buf1 = (UINT8 *)frame;
      for(i=0;i<PROTOCOL_HEAD_LENGTH-1;i++){
         temp_buf[i]=temp_buf1[i];
     }
     temp_buf[len-1]=frame->crc;

    if(SEND_FRAME_FLG == flg)
    {
        for(i = 0; i < len; i++)
        {
            log_debug0("Send Frame--temp_buf[%d]=0x%x\n", i,temp_buf[i]);
        }
    }
    else
    {
        for(i = 0; i < len; i++)
        {
            log_debug0("Recv Frame--temp_buf[%d]=0x%x\n", i,temp_buf[i]);
        }
    }

    PROTOCOL_FREE(temp_buf);
    xSemaphoreGive(s_Protocol_Printf_Lock);
}
#endif
/*****************************************************************************
**函 数 名: _Protocol_Insert_Frame_Node
**输入参数: FRAME_STRUCT *frame_node:存储协议帧信息的指针
**输出参数: 无
**返 回 值: static
**功能描述: 将打包好的协议帧放到链表中
**作     者: wqw
*****************************************************************************/
static void _Protocol_Insert_Frame_Node(FRAME_STRUCT *frame_node)
{
    PROTOCOL_LIST_STRUCT *node = NULL;
    PROTOCOL_LIST_STRUCT *head = &s_Frame_List_Head;
    PROTOCOL_LIST_STRUCT *q = head;

    node = (PROTOCOL_LIST_STRUCT *)PROTOCOL_MALLOC(sizeof(PROTOCOL_LIST_STRUCT));
    if(NULL == node)
    {
        PROTOCOL_DEBUG_PRINTF("Frame node malloc failed\n");
        return;
    }
    PROTOCOL_DEBUG_PRINTF("malloc:%p\n", node);
    node->frame = frame_node;
    node->next = NULL;

    /*找到最后一个节点*/
    while(NULL != q->next)
    {
        q = q->next;
    }

    q->next = node;
}

/*****************************************************************************
**函 数 名: Protocol_Calc_Checksum
**输入参数: UINT8 *buf:需要计算校验的内容
            UINT16 len:内容长度
**输出参数: 无
**返 回 值: crc
**功能描述: 计算crc，数据和的低八位
**作     者: wqw
*****************************************************************************/
UINT8 Protocol_Calc_Checksum(UINT8 *buf, UINT16 len)
{
    UINT32 num;
    UINT16 sum=0;
    log_debug0("Protocol_Calc_Checksum---buf_len=%d\n",len);

    for(num=0;num<len-1;num++)
    {
        //log_debug0("buf[%d]=0x%x---sum=0x%x\n",num,buf[num],sum);
        sum=sum+*(buf+num);
    }
    sum=sum&0x00ff;
    log_debug0("Protocol_Calc_Checksum CRC=0x%x\n",sum);
    return sum;
}

/*****************************************************************************
**函 数 名: Protocol_Package_Recv_Data
**输入参数: UINT8 *buf:收到的数据
            UINT32 len:收到的数据长度
**输出参数: 无
**返 回 值: 包含协议帧的链表头
**功能描述: 将从串口收到的数据打包成对应的协议帧并放入链表中。
**作     者: wqw
**注     意：放入链表中的为动态分配的内存地址，将数据处理完成后应调用
            Protocol_Free_List_Node()释放相应    的内存，包括节点、FRAME_STRUCT 和其中的body。
*****************************************************************************/
PROTOCOL_LIST_STRUCT* Protocol_Package_Recv_Data(UINT8 *buf, UINT16 len)
{
    log_debug0("enter Protocol_Package_Recv_Data\n");
    UINT16 i = 0;
    ORIGINAL_DATA_STRUCT* data_recv = &s_Orig_Data_Handle;
    PROTOCOL_LIST_STRUCT *temp = NULL;
    UINT8 *ptr = NULL;
    UINT8 body_frame_length=len-PROTOCOL_HEAD_LENGTH;

    if(FRAME_CRC_STATE == data_recv->recv_state)/*针对一帧需要收两次的情况*/
    {
        ptr = &data_recv->frame->crc;
    }

    for(i = 0; i < len; i++)
    {
        log_debug0("enter Protocol_Package_Recv_Data--buf[%d]=0x%x\n",i,buf[i]);
        switch(data_recv->recv_state)
        {
            case FRAME_HEAD_AA_STATE:  /*接收帧头*/
                if(0xAA == buf[i])
                {
                    data_recv->recv_state = FRAME_HEAD_55_STATE;
                }
                break;
            case FRAME_HEAD_55_STATE:
                if(0x55 == buf[i])
                {
                    data_recv->recv_state = FRAME_TYPE_STATE;
                }
                else
                {
                    data_recv->recv_state = FRAME_HEAD_AA_STATE;
                }
                break;
            case FRAME_TYPE_STATE:
                data_recv->temp_frame.magic_number = ENDIAN_SWITCH(PROTOCOL_HEAD);  /*为了输出接收帧时显示:AA 55...。*/
                data_recv->temp_frame.frame_type = buf[i];
                data_recv->recv_state = FRAME_LENGTH_STATE;
                data_recv->temp_len = 0;
                break;

            case FRAME_LENGTH_STATE:
                data_recv->temp_frame.length = buf[i];
                data_recv->recv_state =FRAME_SEQUENCE_STATE;
                break;

            case FRAME_SEQUENCE_STATE:
                data_recv->temp_frame.sequence_number = buf[i];
                data_recv->frame = (FRAME_STRUCT*)PROTOCOL_MALLOC(sizeof(FRAME_STRUCT));
                if(NULL == data_recv->frame)
                {
                    PROTOCOL_DEBUG_PRINTF("Frame malloc failed\n");
                    data_recv->recv_state = FRAME_HEAD_AA_STATE;
                    break;
                }
                if(body_frame_length!=0){
                    data_recv->frame->body = (UINT8*)PROTOCOL_MALLOC(body_frame_length);
                    if(NULL == data_recv->frame->body)
                    {
                        PROTOCOL_DEBUG_PRINTF("data_recv->frame->body---Frame malloc failed\n");
                        data_recv->recv_state = FRAME_HEAD_AA_STATE;
                        break;
                    }
                    PROTOCOL_DEBUG_PRINTF("Frame malloc mem:%p------data_recv->frame->body=%p\n", data_recv->frame,data_recv->frame->body);

                    ptr = data_recv->frame->body;
                    data_recv->recv_state = FRAME_BODY;
                    data_recv->temp_len=0;
                }else{
                    data_recv->recv_state = FRAME_CRC_STATE;
                }

                break;

            case FRAME_BODY:        /*接收body*/
                PROTOCOL_MEMCPY(data_recv->frame, &data_recv->temp_frame, PROTOCOL_HEAD_LENGTH-1);
                ptr[data_recv->temp_len++] = buf[i];
                if(data_recv->temp_len == body_frame_length){
                    data_recv->recv_state = FRAME_CRC_STATE;
                }
                break;

            case FRAME_CRC_STATE:
                if(body_frame_length==0){
                    PROTOCOL_MEMCPY(data_recv->frame, &data_recv->temp_frame, PROTOCOL_HEAD_LENGTH-1);
                }
                data_recv->frame->crc = buf[i];
                PROTOCOL_DEBUG_PRINTF("buf[%d]=0x%x--data_recv->temp_frame.crc=0x%x\n",i,buf[i],data_recv->frame->crc);
                {/*通知任务去处理*/
                    _Protocol_Insert_Frame_Node(data_recv->frame);
                    PROTOCOL_DEBUG_PRINTF("Recv one frame with body, insert into list\n");
#ifdef PROTOCOL_DEBUG
                    //data_recv->frame->length = ENDIAN_SWITCH(data_recv->frame->length);//转换成小端存储打印出来
                    _Protocol_Print_Frame(data_recv->frame, RECV_FRAME_FLG,body_frame_length);
                    //data_recv->frame->length = ENDIAN_SWITCH(data_recv->frame->length);//再次转换成大端存储进行保存
#endif
                    data_recv->frame = NULL;
                    data_recv->temp_len = 0;
                    data_recv->recv_state = FRAME_HEAD_AA_STATE;
                }
                break;

            default:
                break;
        }
    }

    temp = s_Frame_List_Head.next;
    s_Frame_List_Head.next = NULL;

    return temp;
}

/*****************************************************************************
**函 数 名: Protocol_Get_Next_Frame
**输入参数: PROTOCOL_LIST_STRUCT *frame:需要释放的节点
**输出参数: 无
**返 回 值: 返回链表中的下一个节点(帧)
**功能描述: 释放链表中的节点和帧空间并返回下一个帧节点
**作     者: wqw
*****************************************************************************/
PROTOCOL_LIST_STRUCT* Protocol_Get_Next_Frame(PROTOCOL_LIST_STRUCT *frame_node)
{
    PROTOCOL_LIST_STRUCT *temp = frame_node->next;
    PROTOCOL_DEBUG_PRINTF("Free frame frame_node->frame->body:%p\n", frame_node->frame->body);
    PROTOCOL_FREE(frame_node->frame->body);
    PROTOCOL_DEBUG_PRINTF("Free frame of list:%p\n", frame_node->frame);
    PROTOCOL_FREE(frame_node->frame);

    frame_node->frame = NULL;
    frame_node->next = NULL;
    PROTOCOL_DEBUG_PRINTF("Free list node:%p\n", frame_node);

    PROTOCOL_FREE(frame_node);
    frame_node = NULL;

    return temp;
}

/*****************************************************************************
**函 数 名: Protocol_Malloc_Frame
**输入参数: UINT16 body_len:帧中body的长度
**输出参数: 无
**返 回 值:
**功能描述: 为即将发送的帧分配内存空间
**作     者: wqw
*****************************************************************************/
FRAME_STRUCT* Protocol_Malloc_Frame(UINT16 body_len)
{
    FRAME_STRUCT *frame_temp = NULL;
    UINT32 frame_len = body_len + PROTOCOL_HEAD_LENGTH;

    frame_temp = (FRAME_STRUCT *)PROTOCOL_MALLOC(frame_len);
    if(NULL == frame_temp)
    {
        PROTOCOL_DEBUG_PRINTF("Malloc fail\n");
        return NULL;
    }

    return frame_temp;
}

/*****************************************************************************
**函 数 名: Protocol_Package_Frame
**输入参数: FRAME_STRUCT *frame:存储帧信息
            UINT8 frame_type:协议帧类型
            UINT8 *body:协议帧内容
            UINT16 body_len:协议帧内容长度
            UINT8 seq_number:协议包序号
**输出参数: 无
**返 回 值:
**功能描述: 打包协议帧
**作     者: wqw
*****************************************************************************/
UINT8* Protocol_Package_Frame(FRAME_STRUCT *frame, UINT8 frame_type, UINT8 *body, UINT16 body_len,UINT8 seq_number)
{
    //static UINT8 s_Sequence_Number = 0; /*发送帧序号，每发送一次加1，超出0xFF自动归0*/
    UINT8 *psend_buf=NULL;
    UINT8 i;
    UINT8 *temp_buf = NULL;

    if(NULL == frame)
    {
        PROTOCOL_DEBUG_PRINTF("Para wrong, frame is NULL\n");
        return NULL;
    }

    frame->magic_number = ENDIAN_SWITCH(PROTOCOL_HEAD);
    frame->frame_type = frame_type;
    frame->length = body_len+PROTOCOL_HEAD_LENGTH;

    if(0 != body_len)
    {
        PROTOCOL_MEMCPY(frame->body, body, body_len);
    }
    for(i=0;i<body_len;i++){
        PROTOCOL_DEBUG_PRINTF("frame->body[%d]=0x%x\n",i,frame->body[i]);
    }
    psend_buf=(UINT8 *)PROTOCOL_MALLOC(body_len+PROTOCOL_HEAD_LENGTH);

    for(i=0;i<body_len;i++){
        psend_buf[i+PROTOCOL_HEAD_LENGTH-1]=frame->body[i];
    }
    xSemaphoreTake(s_Protocol_Package_Lock, portMAX_DELAY);
    //frame->sequence_number = s_Sequence_Number++;
    frame->sequence_number =seq_number;
    xSemaphoreGive(s_Protocol_Package_Lock);

    temp_buf = (UINT8 *)frame;
    for(i=0;i<PROTOCOL_HEAD_LENGTH-1;i++){
       psend_buf[i]=temp_buf[i];
   }

    frame->crc = Protocol_Calc_Checksum(psend_buf, body_len+PROTOCOL_HEAD_LENGTH);
    psend_buf[body_len+PROTOCOL_HEAD_LENGTH-1]=frame->crc;

#ifdef PROTOCOL_DEBUG
    _Protocol_Print_Frame(frame, SEND_FRAME_FLG,body_len);
#endif

    return psend_buf;
}

/*****************************************************************************
**函 数 名: Protocol_Free_Frame
**输入参数: FRAME_STRUCT *frame:打包好的帧地址
**输出参数: 无
**返 回 值:
**功能描述: 释放打包时占用的空间
**作     者: wqw
*****************************************************************************/
void Protocol_Free_Frame(FRAME_STRUCT *frame)
{
    PROTOCOL_FREE(frame);
}

/*****************************************************************************
**函 数 名: Protocol_Init
**输入参数: void
**输出参数: 无
**返 回 值: 1:初始化成功
            0:初始化失败
**功能描述: 协议初始化。创建协议发送和log锁。
**作     者: wqw
*****************************************************************************/
UINT8 Protocol_Init(void)
{
    s_Protocol_Package_Lock = xSemaphoreCreateMutex();
    if(NULL == s_Protocol_Package_Lock)
    {
        PROTOCOL_DEBUG_PRINTF("Protocol send lock(Semaphore) create fail\n");
        goto PRO_ERR;
    }

#ifdef PROTOCOL_DEBUG
    s_Protocol_Printf_Lock = xSemaphoreCreateMutex();
    if(NULL == s_Protocol_Printf_Lock)
    {
        PROTOCOL_DEBUG_PRINTF("Protocol print lock(Semaphore) create fail\n");
        goto PRO_ERR;
    }
#endif
    return PROTOCOL_SUCCESS;
PRO_ERR:
    return PROTOCOL_FAILURE;
}

