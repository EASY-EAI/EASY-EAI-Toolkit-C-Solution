//=====================  C++  =====================
#include <iostream>
#include <fstream>
//=====================   C   =====================
#include "system.h"
//=====================  SDK  =====================
#include <rga/RgaApi.h>
#include "mpp_mem.h"
#include "system_opt.h"
#include "frame_queue.h"
#include "endeCode_api.h"
#include "disp.h"
//=====================  PRJ  =====================
#include "player.h"

using namespace std;

typedef struct{
	Player *pSelf;
	
}Player_para_t;

void *cruiseCtrl_thread(void *para)
{
	Player_para_t *pPlayerPara = (Player_para_t *)para;
    Player *pSelf = pPlayerPara->pSelf;
    uint32_t chnId = 0;

    if(NULL == pSelf) {
	    goto exit_thread_body;
    }

	while(1){
        pSelf->setPlayingChn(chnId);

        chnId++;
        chnId %= pSelf->maxChnNum();
    
		sleep(5);
	}

exit_thread_body:
	free(pPlayerPara);
	pthread_exit(NULL);
}

static int32_t VideoPlayerHandle(void *pPlayer,  VideoFrameData *pData)
{
    if(NULL == pPlayer){
        return -1;
    }

    #if 0 //Debug
	uint64_t t1 = get_timeval_us();
	printf("chn[0] --- timestamp = %llu.%06llu ms\n", t1/1000000, t1%1000000);
    #endif
	
    if((0 != pData->err_info) || (0 != pData->discard)){
        printf("[output] chn 0 ----- errinfo : %u discard : %u\n", pData->err_info, pData->discard);
    }
    
    Player *pSelf = (Player *)pPlayer;
    if(NULL != pSelf){
        if(0 != pSelf->playingChn()){
            return 0;
        }
    }
	
    
    if((0 == pData->err_info) && (0 == pData->discard)){
	    pSelf->displayCommit(pData->pBuf, 0, RK_FORMAT_YCbCr_420_SP, pData->width, pData->height, 0);
    }

    return 0;
}
static int32_t VideoPlayerHandle_1(void *pPlayer,  VideoFrameData *pData)
{
    if(NULL == pPlayer){
        return -1;
    }
    
    #if 0 //Debug	
	uint64_t t1 = get_timeval_us();
	printf("chn[1] --- timestamp = %llu.%06llu ms\n", t1/1000000, t1%1000000);
    #endif
	
    if((0 != pData->err_info) || (0 != pData->discard)){
        printf("[output] chn 1 ----- errinfo : %u discard : %u\n", pData->err_info, pData->discard);
    }
    
    Player *pSelf = (Player *)pPlayer;
    if(NULL != pSelf){
        if(1 != pSelf->playingChn()){
            return 0;
        }
    }
    
    if((0 == pData->err_info) && (0 == pData->discard)){
	    pSelf->displayCommit(pData->pBuf, 0, RK_FORMAT_YCbCr_420_SP, pData->width, pData->height, 0);
    }

    return 0;
}
static int32_t VideoPlayerHandle_2(void *pPlayer,  VideoFrameData *pData)
{
    if(NULL == pPlayer){
        return -1;
    }
    
    #if 0 //Debug
	uint64_t t1 = get_timeval_us();
	printf("chn[2] --- timestamp = %llu.%06llu ms\n", t1/1000000, t1%1000000);
    #endif
    
    if((0 != pData->err_info) || (0 != pData->discard)){
        printf("[output] chn 2 ----- errinfo : %u discard : %u\n", pData->err_info, pData->discard);
    }
	
    Player *pSelf = (Player *)pPlayer;
    if(NULL != pSelf){
        if(2 != pSelf->playingChn()){
            return 0;
        }
    }
    
    if((0 == pData->err_info) && (0 == pData->discard)){
	    pSelf->displayCommit(pData->pBuf, 0, RK_FORMAT_YCbCr_420_SP, pData->width, pData->height, 0);
    }

    return 0;
}
static int32_t VideoPlayerHandle_3(void *pPlayer,  VideoFrameData *pData)
{
    if(NULL == pPlayer){
        return -1;
    }

    #if 0 //Debug
	uint64_t t1 = get_timeval_us();
	printf("chn[3] --- timestamp = %llu.%06llu ms\n", t1/1000000, t1%1000000);
    #endif
    
    if((0 != pData->err_info) || (0 != pData->discard)){
        printf("[output] chn 3 ----- errinfo : %u discard : %u\n", pData->err_info, pData->discard);
    }
    
    Player *pSelf = (Player *)pPlayer;
    if(NULL != pSelf){
        if(3 != pSelf->playingChn()){
            return 0;
        }
    }
    
    if((0 == pData->err_info) && (0 == pData->discard)){
	    pSelf->displayCommit(pData->pBuf, 0, RK_FORMAT_YCbCr_420_SP, pData->width, pData->height, 0);
    }

    return 0;
}

typedef struct {
    RgaSURF_FORMAT fmt;
    int width;
    int height;
    int rotation;    
    void *pBuf;
}Image;
static int yuv420_to_rgb(Image *pDst, Image *pSrc)
{
	rga_info_t src, dst;
	int ret = -1;

	if (!pSrc || !pDst) {
		printf("%s: NULL PTR!\n", __func__);
		return -1;
	}

	//图像参数转换
	memset(&src, 0, sizeof(rga_info_t));
	src.fd = -1;
	src.virAddr = pSrc->pBuf;
	src.mmuFlag = 1;
	src.rotation =  pSrc->rotation;
	rga_set_rect(&src.rect, 0, 0, pSrc->width, pSrc->height, pSrc->width, pSrc->height, pSrc->fmt);

	memset(&dst, 0, sizeof(rga_info_t));
	dst.fd = -1;
	dst.virAddr = pDst->pBuf;
	dst.mmuFlag = 1;
	dst.rotation =  pDst->rotation;
	rga_set_rect(&dst.rect, 0, 0, pDst->width, pDst->height, pDst->width, pDst->height, pDst->fmt);
	if (c_RkRgaBlit(&src, &dst, NULL)) {
		printf("%s: rga fail\n", __func__);
		ret = -1;
	}
	else {
		ret = 0;
	}

	return ret;
}

Player::Player(int iChnNum) :
    mChnannelNumber(iChnNum),
    mPlayingChnId(0),
	bObjIsInited(0)
{
    int i;
    for(i = 0; i < mChnannelNumber; i++){
        mChannelId[i] = 0;
    }

    init(720, 1280);

	// 获取文件解码类型
    MppCodingType type = MPP_VIDEO_CodingAVC;

	// 1.创建解码器
	create_decoder(mChnannelNumber);
    
	// 2.向解码器申请解码通道
    for(i = 0; i < mChnannelNumber; i++){
    	if(0 == create_decMedia_channel(&mChannelId[i], type)){
    		//printf("============= [%d]time ==============\n", i);
    		printf("create channel OK, chn = %u\n", mChannelId[i]);
    		
    		// 3.往成功申请的通道绑定解码输出处理函数
    		if(0 == mChannelId[i]){
    	    	set_decMedia_channel_callback(mChannelId[i], VideoPlayerHandle, this);
            }else if (1 == mChannelId[i]){
    	    	set_decMedia_channel_callback(mChannelId[i], VideoPlayerHandle_1, this);
            }else if (2 == mChannelId[i]){
    	    	set_decMedia_channel_callback(mChannelId[i], VideoPlayerHandle_2, this);
            }else if (3 == mChannelId[i]){
    	    	set_decMedia_channel_callback(mChannelId[i], VideoPlayerHandle_3, this);
            }
    	}
    }
    
	Player_para_t *pPlayer = (Player_para_t *)malloc(sizeof(Player_para_t));	 
	pPlayer->pSelf = this;
	
	if(0 != CreateNormalThread(cruiseCtrl_thread, pPlayer, &mTid)){
		free(pPlayer);
	}
}

Player::~Player()
{
    int i;
    for(i = 0; mChnannelNumber < 1; i++){
	    close_decMedia_channel(mChannelId[i]);
    }
    deInit();
}


void Player::init(int width, int height)
{
    if(0 == disp_init(width, height)){
        bObjIsInited = 1;
    }else{
        bObjIsInited = 0;
    }
}

void Player::deInit()
{
    if(IsInited()){
        disp_exit();
    }
}

void Player::displayCommit(void *ptr, int fd, int fmt, int w, int h, int rotation)
{
#if 1
    Image srcImage, dstImage;
    memset(&srcImage, 0, sizeof(srcImage));
    memset(&dstImage, 0, sizeof(dstImage));

    srcImage.fmt = (RgaSURF_FORMAT)fmt;
    srcImage.width = w;
    srcImage.height = h;
    srcImage.rotation = rotation;
    srcImage.pBuf = ptr;

    dstImage.fmt = RK_FORMAT_RGB_888;
    dstImage.width = 720;
    dstImage.height = 1280;
    dstImage.rotation = rotation;
    dstImage.pBuf = mpp_malloc(char, dstImage.width*dstImage.height*3);
    if(NULL == dstImage.pBuf){
        return;
    }
    
    yuv420_to_rgb(&dstImage, &srcImage);

    if(IsInited()){
        disp_commit(dstImage.pBuf, 0, 0);
    }

    mpp_free(dstImage.pBuf);
#endif
}

