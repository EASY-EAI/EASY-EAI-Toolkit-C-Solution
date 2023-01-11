/**
*
* Copyright 2022 by Guangzhou Easy EAI Technologny Co.,Ltd.
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

#include "system.h"

#include "rtspServer.h"

void VideoStreamConnect()
{
    flush_video_channel(DATA_INPUT_CHN);
}
int32_t VideoStreamDataIn(RTSPVideoDesc_t *pDesc, uint8_t *pData)
{
    int ret = -1;

    VideoNodeDesc node;
    memset(&node, 0, sizeof(node));
    ret = get_node_from_video_channel(DATA_INPUT_CHN, &node, pData);
    if(0 != ret){
        return ret;
    }

    pDesc->frameType  = node.bySubType;
    pDesc->frameIndex = node.dwFrameIndex;
    pDesc->dataLen    = node.dwDataLen;
    pDesc->timeStamp  = node.ddwTimeStamp;
    
    return 0;
}

int rtspServerInit(const char *moduleName)
{
    RtspServer_t srv;
    memset(&srv, 0, sizeof(RtspServer_t));
    srv.port = 554;
    srv.stream[0].bEnable = true;
    srv.stream[0].videoHooks.pConnectHook = VideoStreamConnect;
    srv.stream[0].videoHooks.pDataInHook = VideoStreamDataIn;
    srv.stream[0].audioHooks.pConnectHook = NULL;
    srv.stream[0].audioHooks.pDataInHook = NULL;
    strcpy(srv.stream[0].strName, "aabb");

    create_video_frame_queue_pool(DATA_INPUT_CHN+1);
    //create_audio_frame_queue_pool(DATA_INPUT_CHN+1);

    create_rtsp_Server(srv);

    // 正常情况下不会走到这里
    PRINT_ERROR("Fatal Error! RtspServer loop exited!");

    return -1;
}

