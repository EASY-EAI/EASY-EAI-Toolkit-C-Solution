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

#include "camera.h"
#include "enCoder.h"

#define CAMERA_WIDTH	720
#define CAMERA_HEIGHT	1280
#define	IMGRATIO		1.5
#define	IMAGE_SIZE		(CAMERA_WIDTH*CAMERA_HEIGHT*IMGRATIO)

#define MAXCHNNUM 4

/* ======================== 编码好的NAL单元在此处输出 ======================== */
int32_t StreamOutpuHandle(void *obj, VideoNodeDesc *pNodeDesc, uint8_t *pNALUData)
{
    time_t tm = (time_t)(pNodeDesc->ddwTimeStamp/1000);
    char tmp[64];
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&tm));
    printf("=====[frame time]: %s\n", tmp);
    
    printf("[%03u][0x%02x][%d]------------- dataLen = %d\n", pNodeDesc->dwFrameIndex,
                                                    pNALUData[4],
                                                    pNodeDesc->bySubType,
                                                    pNodeDesc->dwDataLen);
    
    push_node_to_video_channel(0, pNodeDesc, pNALUData);

    return 0;
}


int enCoderInit(const char *moduleName)
{
    int ret = 0;

    // camera
    char *pbuf = NULL;
    int skip = 0;

    // encode
    WorkPara wp;
    //AdvanceWorkPara awp;
	
	uint32_t encodeChn_Id;
    create_encoder(MAXCHNNUM);
	create_encMedia_channel(&encodeChn_Id);
	if((0 <= encodeChn_Id) && (encodeChn_Id < 4)){
		printf("decode channel create succ, channel Id = %u \n", encodeChn_Id);
	}else{
		printf("decode channel create faild !\n");
        goto exit3;
	}

    // 初始化共享内存通道--用于rtsp发送
    create_video_frame_queue_pool(2);

    /* ============================= 初始化摄像头 ============================= */
    ret = rgbcamera_init(CAMERA_WIDTH, CAMERA_HEIGHT, 90);
    if (ret) {
        printf("error: %s, %d\n", __func__, __LINE__);
        goto exit3;
    }

    rgbcamera_set_format(RK_FORMAT_YCbCr_420_SP);

    pbuf = (char *)malloc(IMAGE_SIZE);
    if (!pbuf) {
        printf("error: %s, %d\n", __func__, __LINE__);
        ret = -1;
        goto exit2;
    }

    //跳过前10帧
    skip = 10;
    while(skip--) {
        ret = rgbcamera_getframe(pbuf);
        if (ret) {
            printf("error: %s, %d\n", __func__, __LINE__);
            goto exit1;
        }
    }

    /* ============================= 初始化编码通道 ============================= */
	set_encMedia_channel_callback(encodeChn_Id, StreamOutpuHandle, NULL);
    memset(&wp, 0, sizeof(WorkPara));
    wp.in_fmt  = VFRAME_TYPE_NV12;
    wp.out_fmt = VCODING_TYPE_AVC;
    wp.out_fps = 25;
    wp.width   = CAMERA_WIDTH;
    wp.height  = CAMERA_HEIGHT;
	ret = set_encMedia_channel_workPara(encodeChn_Id, &wp, NULL);
    if(ret){
        goto exit0;
    }



    /* ============================ 送帧进入编码通道 ============================= */
    while(1){
        ret = rgbcamera_getframe(pbuf);
        if(ret){
            usleep(10*1000); continue;
        }
        
		push_frame_to_encMedia_channel(encodeChn_Id, pbuf, IMAGE_SIZE);
        usleep(10*1000);
    }

    
exit0:
    close_encMedia_channel(encodeChn_Id);

exit1:
    free(pbuf);
    pbuf = NULL;
exit2:
    rgbcamera_exit();
exit3:
    return ret;

}

