//=====================  C++  =====================
#include <iostream>
#include <string>
#include <queue>
//=====================   C   =====================
#include "system.h"
#include "config.h"
//=====================  SDK  =====================
#include "ini_wrapper.h"
#include "log_manager.h"
#include "system_opt.h"
#include "frame_queue.h"
#include "rtsp.h"
//=====================  PRJ  =====================
#include "rtspCapturer.h"

using namespace std;

typedef struct{
	RtspCapturer *pSelf;
	
}RtspCapturer_para_t;

int32_t VideoHandle(void *pCapturer, RTSPVideoDesc_t *pDesc, uint8_t *pData)
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
    
    if( NULL != pData &&            /*有帧数据*/
        0 != pSelf->IsInited() &&   /*Rtsp取流器已被初始化好*/
        0 < pDesc->dataLen 
    ){
        frameCount++;
        //成功取流，送流进入取流器输出队列
        pDesc->frameIndex = frameCount;
        
        #if 0   //Debug
        printf("Format[%d](0-H264,1-MJPEG,2-MPEG4,3-H265)---Type[%d](0-?,1-I,2-P,3-B)---NALUSize[%u]---TimeStamp[%llu]\n", 
                pDesc->frameFormat, pDesc->frameType, pDesc->dataLen, pDesc->timeStamp);
        #endif
        #if 0   //Debug
        if(0 == pDesc->videoChnId){
		    printf("[input]--- chn[%d] -- %d fps  --- frameIndex = %u --- interval = %llu ms\n",pDesc->videoChnId, pDesc->frameRate, pDesc->frameIndex ,interval);
        }
        #endif
        
        if((0 <= pSelf->channelId()) && (pSelf->channelId() < MAX_VIDEO_CHN_NUMBER)){
            VideoNodeDesc NodeDesc = {0};
            NodeDesc.dwSignalVout        = pDesc->videoChnId;
            NodeDesc.dwDataLen           = pDesc->dataLen;
            NodeDesc.ddwTimeStamp        = pDesc->timeStamp;
            NodeDesc.ddwReceiveTimeStamp = pDesc->recTimeStamp;
            NodeDesc.bySubType           = pDesc->frameType;
            NodeDesc.eVdecChnFormat      = (VDEC_CHN_FORMAT_E)pDesc->frameFormat;
            NodeDesc.dwTargetFrameRate   = pDesc->frameRate;
            NodeDesc.dwWidth             = pDesc->frameWidth;
            NodeDesc.dwHeight            = pDesc->frameHeight;
            NodeDesc.dwCropEnable        = false;
            NodeDesc.stActRegion.dwX     = 0;
            NodeDesc.stActRegion.dwY     = 0;
            NodeDesc.stActRegion.dwWidth = NodeDesc.dwWidth;
            NodeDesc.stActRegion.dwHeight= NodeDesc.dwHeight;
            push_node_to_video_channel(pSelf->channelId(), &NodeDesc, pData);
        }
    }

    return 0;
}

int32_t AudioHandle(void *pCapturer, RTSPAudioDesc_t *pDesc, uint8_t *pData)
{
    if(NULL == pCapturer)
        return -1;
    RtspCapturer *pSelf = (RtspCapturer *)pCapturer;

    if( NULL != pData &&            /*有帧数据*/
        0 != pSelf->IsInited() &&   /*Rtsp取流器已被初始化好*/
        0 < pDesc->dataLen
    ){
        //成功取流，送流进入取流器输出队列
        if((0 <= pSelf->channelId()) && (pSelf->channelId() < MAX_VIDEO_CHN_NUMBER)){
            AudioNodeDesc NodeDesc = {0};
            NodeDesc.dwStreamId     = pDesc->audioChnId;
            NodeDesc.ePayloadType   = (ADEC_CHN_FORMAT_E)pDesc->frameFormat;
            NodeDesc.dwFrameIndex   = pDesc->frameIndex;
            NodeDesc.dwDataLen      = pDesc->dataLen;
            NodeDesc.ddwTimeStamp   = pDesc->timeStamp;
            NodeDesc.dwSampleRateHz = pDesc->sampleRateHz;
            NodeDesc.dwBitRate      = pDesc->bitRate;
            NodeDesc.wProfile       = pDesc->profile;
            memcpy(NodeDesc.strConfig, pDesc->strConfig, sizeof(pDesc->strConfig));
            push_node_to_audio_channel(pSelf->channelId(), &NodeDesc, pData);
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
    flush_video_channel(chnId);
    create_audio_frame_queue_pool(MAX_VIDEO_CHN_NUMBER);
    flush_audio_channel(chnId);
    
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
	//rtspChn.bUseTcpConnect = true;
	rtspChn.bOutputTestRecordFile = true;

    m_dwChnId = chnId;
    
    set_rtsp_client_callback(VideoHandle, AudioHandle, (void *)this);
	create_rtsp_client_channel(&rtspChn);

}

int rtspSignalInit(int argc, char** argv)
{
	if(argc < 2){
		::exit(0);
	}

    char channelName[64] = {0};
    sprintf(channelName, "%s_%s", argv[1], argv[2]);
    
    // 0-初始化日志管理系统
    log_manager_init(".", channelName);
    
    RtspCapturer *pRtspCapturer = new RtspCapturer(channelName);    
    pRtspCapturer->init(atoi(argv[2]));

    return 0;
}


