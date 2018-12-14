#ifndef Wifi_Door_Lock_H
#define Wifi_Door_Lock_H

#include "protocol.h"

#ifndef Wifi_Door_Lock_Open_Ignore_Event
//#define Wifi_Door_Lock_Open_Ignore_Event
#endif

#define Wifi_Door_Lock_SUCCESS 1   /*Wifi_Door_Lock相关功能操作成功*/
#define Wifi_Door_Lock_FAILURE 0   /*操作失败*/

void Wifi_Door_Lock_Send_IOT_Event_Frame_To_Mcu(UINT8 event_type, UINT8 state);
void Wifi_Door_Lock_Send_IOT_Action_Frame_To_Mcu(UINT32 action_id, UINT16 para);
UINT32 Wifi_Door_Lock_Get_Int_Attribute(UINT32 attribute_id);
UINT8* Wifi_Door_Lock_Get_String_Attribute(UINT32 attribute_id);

UINT8 Wifi_Door_Lock_Init(void);

#endif
