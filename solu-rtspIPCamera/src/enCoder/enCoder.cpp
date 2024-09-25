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
#include "config.h"

#include "enCoder.h"

#define CAMERA_WIDTH	720
#define CAMERA_HEIGHT	1280
#define	IMGRATIO		1.5
#define	IMAGE_SIZE		(CAMERA_WIDTH*CAMERA_HEIGHT*IMGRATIO)

#define PCM_FORMAT     SND_PCM_FORMAT_FLOAT_LE   //由于ffmpeg的AAC编码器仅支持FLTP格式输入，因此声卡用FLOAT采用的运算量最少
#define PCM_SAMPLERATE 44100
#define PCM_CHANNEL    2

#define MAXCHNNUM 4

typedef struct{
	int32_t  videoChn_Id;
	int32_t  audioChn_Id;
	uint16_t vCaptureIsRunning;
	uint16_t aCaptureIsRunning;
	
}Recorder_para_t;

/* ======================== 编码好的NAL单元在此处输出 ======================== */
int32_t StreamOutpuHandle(void *obj, VideoNodeDesc *pNodeDesc, uint8_t *pNALUData)
{
    static uint64_t preTime = 0;
    static  int32_t frameCount = 0;
    uint64_t curTime;
    static uint64_t sumDataLen = 0;
    sumDataLen += pNodeDesc->dwDataLen;
#if 0
    time_t tm = (time_t)(pNodeDesc->ddwTimeStamp/1000);
    char tmp[64];
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&tm));
    printf("=====[Video frame time]: %s\n", tmp);
    
    printf("[%03u][0x%02x][%d]------------- dataLen = %d (sum:%llu)\n", pNodeDesc->dwFrameIndex,
                                                    pNALUData[4],
                                                    pNodeDesc->bySubType,
                                                    pNodeDesc->dwDataLen, sumDataLen);
#endif
    curTime = get_timeval_ms();
    if(curTime - preTime > 1000){
        printf("======================= frameCount = %d\n", frameCount);
        frameCount = 0;
        preTime = curTime;
    }
    frameCount++;

    push_node_to_video_channel(CHANNEL_0, pNodeDesc, pNALUData);

    return 0;
}

void *videoCapture_thread(void *para)
{
    // 播放器对象
    Recorder_para_t *pPara = (Recorder_para_t *)para;

    // encode
    bool bIsInited = true;
    WorkPara wp;
    //AdvanceWorkPara awp;

    // camera
    int ret = 0;
    int skip = 10;
    if(pPara){
        /* ============================= 初始化摄像头 ============================= */
        ret = rgbcamera_init(CAMERA_WIDTH, CAMERA_HEIGHT, 270);
        if (ret) {
            printf("error: %s, %d\n", __func__, __LINE__);
            bIsInited = false;
        }
        rgbcamera_set_format(RK_FORMAT_YCbCr_420_SP);

        /* ============================= 初始化视频编码通道 ============================= */
    	set_encMedia_channel_callback(pPara->videoChn_Id, StreamOutpuHandle, NULL);
        memset(&wp, 0, sizeof(WorkPara));
        wp.in_fmt  = VFRAME_TYPE_NV12;
        wp.out_fmt = VDEC_CHN_FORMAT_H265;
        wp.width   = CAMERA_WIDTH;
        wp.height  = CAMERA_HEIGHT;
        //wp.hor_stride  = wp.width;    //如果从camera取出来的图像没按图像跨距标准排列，请手动按实际情况设置水平跨距
        //wp.ver_stride  = wp.height;   //如果从camera取出来的图像没按图像跨距标准排列，请手动按实际情况设置垂直跨距
        //不了解图像跨距概念可参考这篇第三方博客: https://blog.csdn.net/2201_76108770/article/details/128969384
    	ret = set_encMedia_channel_workPara(pPara->videoChn_Id, &wp, NULL);
        if(ret){
            bIsInited = false;
        }
        
        size_t frameMaxSize = 0;
        char *pbuf = (char *)map_inBuffer_from_encMedia_channel(pPara->videoChn_Id, &frameMaxSize);
        if((NULL != pbuf)&&(IMAGE_SIZE <= frameMaxSize)){
            // 跳过前10帧
            while(skip--) {
                ret = rgbcamera_getframe(pbuf);
                if (ret) {
                    printf("error: %s, %d\n", __func__, __LINE__);
                    bIsInited = false;
                }
            }
            
            /* ============================ 送帧进入编码通道 ============================= */
            if(bIsInited){
                pPara->vCaptureIsRunning = true;
                while(1){
                    ret = rgbcamera_getframe(pbuf);
                    if(ret){
                        usleep(10*1000); continue;
                    }
                    commit_buffer_to_encMedia_channel(pPara->videoChn_Id, false);
                    usleep(10*1000);
                }
            }else{
                rgbcamera_exit();
            }
        }
    }
    
    if(bIsInited){
        rgbcamera_exit();
    }
    close_encMedia_channel(pPara->videoChn_Id);
    printf("==============================[exit video Capture Thread]==============================\n");
    
    pPara->vCaptureIsRunning = false;
	pthread_exit(NULL);
}

/* ======================== 编码好的AAC单元在此处输出 ======================== */
int32_t AACStreamOutpuHandle(void *obj, AudioNodeDesc *pNodeDesc, uint8_t *pADTSData)
{
#if 0
    time_t tm = (time_t)(pNodeDesc->ddwTimeStamp/1000);
    char tmp[64];
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&tm));
    printf("=====[Audio frame time]: %s\n", tmp);
    printf("[Output AAC ES]------------- dataLen = %d\n", pNodeDesc->dwDataLen);
#endif

    push_node_to_audio_channel(CHANNEL_0, pNodeDesc, pADTSData);
    return 0;
}

void *audioCapture_thread(void *para)
{
    // 播放器对象
    Recorder_para_t *pPara = (Recorder_para_t *)para;

    // encode
    bool bIsInited = true;
    AudioWorkPara wp;

    // sound card
     int32_t ret = 0;
    uint32_t periodSize = 0;    //声卡缓冲区内存大小
    uint32_t aacFrameSize = 0;  //一帧AAC数据的大小
    uint32_t readSize = 0;      //实际从声卡读出来的数据大小
    if(pPara){
        /* ============================== 初始化麦克风 =============================== */
    	ret = ai_init(25, PCM_SAMPLERATE, PCM_CHANNEL, PCM_FORMAT);
        if(ret){ bIsInited = false; }
        
        periodSize = ai_pcmPeriodSize();
        if(0 == periodSize){ bIsInited = false; }
        aacFrameSize = PCM_CHANNEL*1024*4;
        aacFrameSize = aacFrameSize <= periodSize ? aacFrameSize : periodSize;
        
        /* =========================== 初始化音频编码通道 ============================ */
    	set_encMedia_audio_channel_callback(pPara->audioChn_Id, AACStreamOutpuHandle, NULL);
        wp.channel_layout = AUDIOCH_LAYOUT_STEREO;  //声道布局要与声卡的声道数量匹配
        wp.bInterleaved = true;     //声卡采样时默认了使用声道交错的采样方式
        wp.sample_rate = PCM_SAMPLERATE;
        wp.sample_fmt = PCM_FORMAT;
        wp.bit_rate  = AAC_BITRATE_64K;
        wp.out_fmt  = ADEC_CHN_FORMAT_AAC_ADTS;
        ret = set_encMedia_audio_channel_workPara(pPara->audioChn_Id, &wp);
        if(ret){
            bIsInited = false;
        }
        if(bIsInited){
            pPara->aCaptureIsRunning = true;
            while(1){
                readSize = ai_readpcmData(aacFrameSize); //从声卡读出一aac音频帧大小的数据
                if(0 == readSize){
                    usleep(5*1000); continue;
                }
                
        	    push_frame_to_encMedia_audio_channel(pPara->audioChn_Id, ai_pcmBufptr(), readSize);
                //usleep(10*1000);此处应该不用sleep，ai_readpcmData会阻塞。
            }
        }else{
            ai_exit();
        }
    }
    
    if(bIsInited){
        ai_exit();
    }
    
    close_encMedia_audio_channel(pPara->audioChn_Id);
    printf("==============================[exit audio Capture Thread]==============================\n");
    
    pPara->aCaptureIsRunning = false;
	pthread_exit(NULL);
}

int enCoderInit(const char *moduleName)
{
    // 0-初始化日志管理系统
    log_manager_init(LOGCONFIG_PATH, moduleName);

    Recorder_para_t recPara;
    recPara.videoChn_Id = -1;
    recPara.audioChn_Id = -1;
    recPara.vCaptureIsRunning = false;
    recPara.aCaptureIsRunning = false;

    pthread_t videoTid, audioTid;

    // 1: 创建编码器
    create_encoder(MAXCHNNUM);
    
    // 2: 创建视频编码通道
    create_encMedia_channel(&recPara.videoChn_Id);
    if((0 <= recPara.videoChn_Id) && (recPara.videoChn_Id < MAXCHNNUM)){
        printf("encode video channel create succ, channel Id = %u \n", recPara.videoChn_Id);
    }else{
        printf("encode video channel create faild !\n");
       goto exit0;
    }

    // 2: 创建音频编码通道
    create_encMedia_audio_channel(&recPara.audioChn_Id);
    if((0 <= recPara.audioChn_Id) && (recPara.audioChn_Id < MAXCHNNUM)){
        printf("encode audio channel create succ, channel Id = %u \n", recPara.audioChn_Id);
    }else{
        printf("encode audio channel create faild !\n");
       goto exit1;
    }

    // 3:: 初始化共享内存通道--用于rtsp发送
    create_video_frame_queue_pool(CHANNEL_Max);
    create_audio_frame_queue_pool(CHANNEL_Max);

    // 4.1: 创建视频采集线程
    CreateNormalThread(videoCapture_thread, &recPara, &videoTid);
    // 4.2: 创建音频采集线程
    CreateNormalThread(audioCapture_thread, &recPara, &audioTid);

    // 5: 进入一个休眠循环，直到视频，音频录制结束
    sleep(2);
    while(recPara.vCaptureIsRunning || recPara.aCaptureIsRunning){
        sleep(1);
    }

    printf("exit main()\n");
    close_encMedia_audio_channel(recPara.audioChn_Id);
exit1:
    close_encMedia_channel(recPara.videoChn_Id);
exit0:
    return 0;

}

