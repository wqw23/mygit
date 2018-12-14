#ifndef PROTOCOL_CONFIG_H
#define PROTOCOL_CONFIG_H

#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "log.h"

/********************************************以下定义可修改************************************************/
#ifndef PROTOCOL_DEBUG
#define PROTOCOL_DEBUG                  /*针对传入的参数进行log输出*/
#endif

/*以下几个定义在protocol.c中用到，可以自己实现并修改此处定义。如果没有定义，那么会编译不通过。
  如果定义为空(比如:#define PROTOCOL_MALLOC(SIZE)           ),那么protocol.c中的功能不可用，但是能够编译通过*/
#define PROTOCOL_MALLOC(size)   malloc(size)
#define PROTOCOL_FREE(ptr)      free(ptr)

#define PROTOCOL_MEMCPY(dst,src,len)            memcpy(dst,src,len)

#define PROTOCOL_SPRINTF(buf, fmt, arg...)      sprintf(buf, fmt, ##arg)
#define PROTOCOL_PRINTF(fmt, arg...)            log_debug0(fmt, ##arg)

#ifdef PROTOCOL_DEBUG
#define PROTOCOL_DEBUG_PRINTF(fmt, arg...)  PROTOCOL_PRINTF("<%s:%d>:"fmt, __FUNCTION__, __LINE__,  ##arg)
#else
#define PROTOCOL_DEBUG_PRINTF(fmt, arg...)
#endif
#endif
