#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "datatype.h"
#include "protocol_config.h"

#define PROTOCOL_HEAD       (0xAA55)        /*协议帧头*/

#define PROTOCOL_HEAD_LENGTH    (6)         /*协议帧帧头的长度--2字节帧头1字节的cmd 1字节的序包1字节的数据长度和1字节的CRC*/

#define PROTOCOL_SUCCESS   (1)     /*返回值，1表示成功(正确)*/
#define PROTOCOL_FAILURE   (0)     /*0表示失败(错误)*/


/*两字节的数据大小端变换*/
#define ENDIAN_SWITCH(data) (((((data) >> 8) & 0xFF) | (((data) & 0xFF) << 8)) & 0xFFFF)

#pragma pack(1)                   /*以1字节对齐，编译器相关*/
typedef struct              /*通信协议帧*/
{
    UINT16 magic_number;    /*0xAA55*/
    UINT8  frame_type;      /*帧类型--命令字cmd*/
    UINT8  length;          /*数据的长度*/
    UINT8  sequence_number; /*包序号*/
    UINT8  *body;            /*帧内容.这里只是用到了body的地址，向其地址中写入数据。该结构体申请空间时会申请body的空间*/
    UINT8  crc;             /*校验和*/
}FRAME_STRUCT;
#pragma pack()

/*协议帧链表，存储收到的帧*/
typedef struct protocol_list
{
    FRAME_STRUCT* frame;        /*帧内容*/
    struct protocol_list *next; /*链表中下一个节点*/
}PROTOCOL_LIST_STRUCT;

PROTOCOL_LIST_STRUCT* Protocol_Package_Recv_Data(UINT8 *buf, UINT16 len);
PROTOCOL_LIST_STRUCT* Protocol_Get_Next_Frame(PROTOCOL_LIST_STRUCT *frame_node);
FRAME_STRUCT* Protocol_Malloc_Frame(UINT16 body_len);
UINT8* Protocol_Package_Frame(FRAME_STRUCT *frame, UINT8 frame_type, UINT8 *body, UINT16 body_len,UINT8 seq_number);
void Protocol_Free_Frame(FRAME_STRUCT *frame);
void Prototol_Package_Recv_Data(UINT8 *buf, UINT16 len);
UINT8 Protocol_Calc_Checksum(UINT8 *buf, UINT16 len);
UINT8 Protocol_Init(void);

#endif

