 /**
 *
 * Copyright 2021 by Guangzhou Easy EAI Technologny Co.,Ltd.
 * website: www.easy-eai.com
 *
 * Author: Jiehao.Zhong <zhongjiehao@easy-eai.com>
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * License file for more details.
 * 
 */

#ifndef RTSP_H
#define RTSP_H

#include <sys/types.h>

#include <stdint.h>
#include <stdbool.h>

#include "node_desc.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * 注意：对live555的API封装出RtspClient，使用全局变量。因此该套接口不允许在同一进程中开启多路Rtsp流。
 * 若需要开启多路Rtsp流：
 *     可通过启动多条进程，使用共享内存解决
 */


//Client
typedef	void (*printMessage)(const char *);
typedef	int32_t (*RtspClientVideoCB)(void *, VideoNodeDesc *, uint8_t *);


typedef struct {
    uint32_t uDecChn;       // 被绑定的解码通道
	char progName[128];
	char rtspUrl[128];
	char userName[32];
	char password[32];
    bool bOutputTestRecordFile; /* 除了该标志为true以外，还需要确保存在/tmp/rtspRecFiles目录 */

    bool bIsRunning;
    bool bSkipNOIFrame;
	void *pAuthenticator;
	pid_t rtspClientPid;
    
    uint32_t uWidth;        // 实际宽度
    uint32_t uHeight;       // 实际高度
    uint32_t uAllWidth;     // 总宽度
    uint32_t uAllHeight;    // 总高度
    uint32_t uFrameRate;    // 帧率
    int32_t sStreamDecType; // 码流格式类型
	uint8_t byCheckSDPSps;
} RTSP_Chn_t;

/*********************************************************************
Function:  set_rtsp_client_printf
Description:
	设置自定义rtsp信息打印输出函数
Example:
    void myPrintFunction(const char *strMsg)
    {
    	printf("[myPrintFunction]----%s\n", strMsg);
    }
    set_rtsp_client_printf(myPrintFunction);
parameter:
    pPrintFunc: 自定义打印函数的函数指针。
Return:
    无
********************************************************************/
extern void set_rtsp_client_printf(printMessage pPrintFunc);

/*********************************************************************
Function:  set_rtsp_client_video_callback
Description:
	设置rtsp取流回调函数，rtsp客户端从服务器获取的码流数据会送入回调函数
Example:
    int32_t video_handle(void *pCapturer, VideoNodeDesc_t *pNodeDesc, uint8_t *pData)
    {
    	printf("Width = %u, Height = %u\n", pNodeDesc->u32_Width, pNodeDesc->u32_Height);
    	return 0;
    }
    set_rtsp_client_video_callback(video_handle, NULL);
parameter:
    pCBFunc: 回调函数入口。
    *para:   回调函数的传入参数，可以为解码器等指针，也可以为NULL。
Return:
	0：成功
	-1：失败
********************************************************************/
extern int32_t set_rtsp_client_video_callback(RtspClientVideoCB pCBFunc, void *para);

/*********************************************************************
Function:  create_rtsp_client_channel
Description:
	创建rtsp客户端。
	注意：一旦调用该函数，本进程将会阻塞在live555事件循环。
Example:
parameter:
    *pChnInfo:用于创建客户端的参数，部分为必填成员，部分为返回信息。
        [必填] uDecChn：被绑定的解码通道
      [非必填] progName：程序名
        [必填] rtspUrl：url名称
        [必填] userName：服务器的登录用户名
        [必填] password：服务器的登录密码
Return:
	0：成功
	-1：失败
********************************************************************/
extern int32_t create_rtsp_client_channel(RTSP_Chn_t *pChnInfo);
extern int32_t close_rtsp_client_channel(RTSP_Chn_t *pChnInfo);


//Server

#if defined(__cplusplus)
}
#endif
#endif // RTSP_H
