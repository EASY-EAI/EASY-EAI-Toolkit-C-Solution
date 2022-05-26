//=====================  C++  =====================
#include <iostream>
#include <string>
#include <queue>
//=====================   C   =====================
#include "system.h"
#include "ipcData.h"
#include "config.h"
//=====================  SDK  =====================
#include "ini_wrapper.h"
#include "system_opt.h"
#include "frame_queue.h"
#include "rtsp.h"
//=====================  PRJ  =====================
#include "rtspCapturer.h"

using namespace std;

typedef struct{
	RtspCapturer *pSelf;
	
}RtspCapturer_para_t;

int32_t VideoHandle(void *pCapturer, VideoNodeDesc *pNodeDesc, uint8_t *pData)
{
    if(NULL == pCapturer)
        return -1;
    RtspCapturer *pSelf = (RtspCapturer *)pCapturer;

    static uint32_t frameCount = 0;
    static uint64_t oldTimeVal = 0;
    uint64_t newTimeVal = get_timeval_ms();
    uint64_t interval = 0;
    if(newTimeVal > oldTimeVal){
        interval = newTimeVal - oldTimeVal;
        oldTimeVal = newTimeVal;
    }
    
    if( NULL != pData &&        /*有帧数据*/
        0 != pSelf->IsInited() &&       /*Rtsp取流器已被初始化好*/
        0 < pNodeDesc->dwDataLen 
    ){
        frameCount++;
        //成功取流，送流进入取流器输出队列
        pNodeDesc->dwFrameIndex = frameCount;
        #if 0   //Debug
        printf("tempNode.read_size = %d\n", tempNode.read_size);
        #endif
        #if 0   //Debug
        printf("Format[%d](0-H264,1-MJPEG,2-MPEG4,3-H265)---Type[%d](0-P,1-I,2B)---TimeStamp[%llu]\n", 
                pNodeDesc->eVdecChnFormat, pNodeDesc->bySubType, pNodeDesc->ddwTimeStamp);
        #endif
        #if 0   //Debug
        if(1 == pNodeDesc->dwSignalVout){
		    printf("[input]--- chn[%d] -- %d fps  --- frameIndex = %u --- interval = %llu ms\n",pNodeDesc->dwSignalVout, pNodeDesc->dwTargetFrameRate, pNodeDesc->dwFrameIndex ,interval);
        }
        #endif
        if((0 <= pSelf->channelId()) && (pSelf->channelId() < MAX_VIDEO_CHN_NUMBER)){
            push_node_to_video_channel(pSelf->channelId(), pNodeDesc, pData);
        }
    }

    return 0;
}

RtspCapturer::RtspCapturer(std::string strSection) :
    m_strSection(strSection),
    m_dwChnId(-1),
	bObjIsInited(0)
{

}

RtspCapturer::~RtspCapturer()
{
	printf("destroy thread succ...\n");
}

void RtspCapturer::init(int32_t chnId)
{
    create_video_frame_queue_pool(MAX_VIDEO_CHN_NUMBER);
    
	bObjIsInited = 1;
	
	RTSP_Chn_t rtspChn;
	memset(&rtspChn, 0, sizeof(RTSP_Chn_t));
	
	rtspChn.uDecChn = 0;
	
    char cProgName[32] = {0};
    ini_read_string(RTSP_CLIENT_PATH, strSection(), "progName", cProgName,sizeof(cProgName));
	memset(rtspChn.progName, 0, sizeof(rtspChn.progName));
	memcpy(rtspChn.progName, cProgName, strlen(cProgName));
	
    char cRtspUrl[128] = {0};
    ini_read_string(RTSP_CLIENT_PATH, strSection(), "rtspUrl", cRtspUrl,sizeof(cRtspUrl));
	memset(rtspChn.rtspUrl, 0, sizeof(rtspChn.rtspUrl));
	memcpy(rtspChn.rtspUrl, cRtspUrl, strlen(cRtspUrl));
	
    char cUserName[32] = {0};
    ini_read_string(RTSP_CLIENT_PATH, strSection(), "userName", cUserName,sizeof(cUserName));
	memset(rtspChn.userName, 0, sizeof(rtspChn.userName));
	memcpy(rtspChn.userName, cUserName, strlen(cUserName));
	
    char cPassword[32] = {0};
    ini_read_string(RTSP_CLIENT_PATH, strSection(), "password", cPassword,sizeof(cPassword));
	memset(rtspChn.password, 0, sizeof(rtspChn.password));
	memcpy(rtspChn.password, cPassword, strlen(cPassword));

	int frameRate = 0;
	ini_read_int(RTSP_CLIENT_PATH, strSection(), "frameRate", &frameRate);
	rtspChn.uFrameRate = frameRate;
	
    rtspChn.uDecChn = chnId;
	rtspChn.bOutputTestRecordFile = true;

    m_dwChnId = chnId;
    
    set_rtsp_client_video_callback(VideoHandle, (void *)this);
	create_rtsp_client_channel(&rtspChn);

}

int rtspSignalInit(int argc, char** argv)
{
	if(argc < 2){
		::exit(0);
	}

    char channelName[64] = {0};
    sprintf(channelName, "%s_%s", argv[1], argv[2]);
    
    RtspCapturer *pRtspCapturer = new RtspCapturer(channelName);    
    pRtspCapturer->init(atoi(argv[2]));

    return 0;
}


