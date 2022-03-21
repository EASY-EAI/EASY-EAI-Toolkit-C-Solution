#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <sys/time.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <iconv.h>

#define SYS_TRUE 1
#define SYS_FALSE 0

typedef bool SYS_BOOL;
typedef void SYS_VOID;

typedef long long SYS_S64;
typedef long SYS_S32;
typedef short SYS_S16;
typedef char SYS_S8;

typedef unsigned long long SYS_U64;
typedef unsigned long SYS_U32;
typedef unsigned short SYS_U16;
typedef unsigned char SYS_U8;


typedef struct {SYS_S32 x; SYS_S32 y; SYS_S32 w; SYS_S32 h;}SYS_RECT;
typedef struct {SYS_U16 left; SYS_U16 top; SYS_U16 right; SYS_U16 bottom;}SYS_SPACE;

enum week_t{
	WK_SUNDAY,
	WK_MONDAY,
	WK_TUESDAY,
	WK_WEDNESDAY,
	WK_THURSDAY,
	WK_FRIDAY,
	WK_SATURDAY,
};

typedef struct {
	SYS_U16 year;
	SYS_U16 month;
	SYS_U16 week;
	SYS_U16 day;
	SYS_U16 hour;
	SYS_U16 minute;
	SYS_U16 second;
}datetime_t;

typedef struct {
	SYS_U16 year;
	SYS_U16 month;
	SYS_U16 day;
	SYS_U16 hour;
	SYS_U16 minute;
	SYS_U16 second;
}SYS_TIME;

typedef struct {
	SYS_U16 year;
	SYS_U16 month;
	SYS_U16 day;
	SYS_U16 hour;
	SYS_U16 minute;
	SYS_U16 second;
}record_time;

#endif



