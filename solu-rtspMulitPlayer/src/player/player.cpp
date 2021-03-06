//=====================  PRJ  =====================
#include "player.h"
//=====================  SDK  =====================
#include <rga/RgaApi.h>
#include "mpp_mem.h"
#include "system_opt.h"
#include "ini_wrapper.h"
#include "frame_queue.h"
#include "endeCode_api.h"
#include "disp.h"

/* flip source image horizontally (around the vertical axis) */
#define HAL_TRANSFORM_FLIP_H     0x01
/* flip source image vertically (around the horizontal axis)*/
#define HAL_TRANSFORM_FLIP_V     0x02
/* rotate source image 0 degrees clockwise */
#define HAL_TRANSFORM_ROT_0      0x00
/* rotate source image 90 degrees clockwise */
#define HAL_TRANSFORM_ROT_90     0x04
/* rotate source image 180 degrees */
#define HAL_TRANSFORM_ROT_180    0x03
/* rotate source image 270 degrees clockwise */
#define HAL_TRANSFORM_ROT_270    0x07

/* decoder output width & height */
#define FRAME_WIDTH 1280
#define FRAME_HEIGHT 720

typedef struct{
	Player *pSelf;
	
}Player_para_t;

void *cruiseCtrl_thread(void *para)
{
    // 播放器对象
	Player_para_t *pPlayerPara = (Player_para_t *)para;
    Player *pSelf = pPlayerPara->pSelf;
    // 本线程的控制参数
    int cout = 1;

    uint32_t chnId = 0;
	while(1){
        if(NULL == pSelf){
            msleep(5);
            break;
        }
       /* 说明：
        * if(exeAtStart == (count%TimeInterval))
        * exeAtStart: 是否在线程启动时执行。[0]-初次启动不执行，[n]-在第一个周期内的第n个单位时间执行
        * TimeInterval: 一个周期包含的时间单位
        */
        if(1 == (cout%(5*100))){    //5s执行, 且启动时执行
            pSelf->setPlayingChn(chnId);
        }
        if(0 == (cout%(5*100))){    //5s执行, 且启动时不执行
            chnId++;
            chnId %= pSelf->maxChnNum();
        }
        if(1 == (cout%4)){    //40ms执行, 且启动时执行
            //pSelf->displayAllChn();
            pSelf->displayCurChn();
        }

        // 计时操作
        cout++;
        cout %= 50000;
        // 时间单位：10ms
		usleep(10*1000);
	}

    if(pPlayerPara){
        free(pPlayerPara);
        pPlayerPara = NULL;
    }

	pthread_exit(NULL);
}

static int32_t VideoPlayerHandle(void *pPlayer,  VideoFrameData *pData)
{
	static uint64_t preTime[MAX_CHN_NUM] = {0};
    uint64_t curTime = 0;
    
    if(NULL == pPlayer){
        return -1;
    }
    
    curTime = get_timeval_us();
	
    if((0 != pData->err_info) || (0 != pData->discard)){
        printf("[output] chn %02d ----- errinfo : %u discard : %u\n", pData->channel, pData->err_info, pData->discard);
    }
#if 0
    // 过滤
    if(0/*[Chn 0]*/ == pData->channel){
	    //printf("chn[%02d] --- curTime = %llu us, preTime = %llu us, cha = %llu us\n", pData->channel, curTime, preTime, curTime - preTime);
        if(curTime - preTime[pData->channel] >= 1000000/* 每1s一张 */){
            preTime[pData->channel] = curTime;
        }else{
            return 0;
        }
    }else if(1/*[Chn 1]*/ == pData->channel){
        if(curTime - preTime[pData->channel] >= 500000/* 每500ms一张 */){
            preTime[pData->channel] = curTime;
        }else{
            return 0;
        }
    }else if(2/*[Chn 2]*/ == pData->channel){
        if(curTime - preTime[pData->channel] >= 150000/* 每1.5ms一张 */){
            preTime[pData->channel] = curTime;
        }else{
            return 0;
        }
    }else if(3/*[Chn 3]*/ == pData->channel){
        if(curTime - preTime[pData->channel] >= 70000/* 每700ms一张 */){
            preTime[pData->channel] = curTime;
        }else{
            return 0;
        }
    }
#endif
#if 1
    // 数帧 -- Debug用
    static int frameCount[MAX_CHN_NUM] = {0};
    if(curTime - preTime[pData->channel] >= 1000000){
        preTime[pData->channel] = curTime;
        if(frameCount[pData->channel] < 20){
            printf("[Chn][%02d] frame count = %d\n",  pData->channel, frameCount[pData->channel]);
        }
        frameCount[pData->channel] = 0;
    }else{
        frameCount[pData->channel]++;
	    //printf("[Chn][%02d] --- timestamp = %llu.%03llu ms\n", pData->channel, curTime/1000, curTime%1000);
    }
#endif
    
    Player *pSelf = (Player *)pPlayer;
    if(NULL == pSelf){
        return -1;
    }
    if((0 == pData->err_info) && (0 == pData->discard)){
        RgaSURF_FORMAT vFmt = RK_FORMAT_YCbCr_420_SP;
        pSelf->makeCamImg(pData->channel, pData->pBuf, vFmt, pData->width, pData->height, pData->hor_stride, pData->ver_stride);
    }

    return 0;
}

typedef struct {
    RgaSURF_FORMAT fmt;
    int width;
    int height;
    int hor_stride;
    int ver_stride;
    int rotation;
    void *pBuf;
}Image;
static int srcImg_ConvertTo_dstImg(Image *pDst, Image *pSrc)
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
	rga_set_rect(&src.rect, 0, 0, pSrc->width, pSrc->height, pSrc->hor_stride, pSrc->ver_stride, pSrc->fmt);

	memset(&dst, 0, sizeof(rga_info_t));
	dst.fd = -1;
	dst.virAddr = pDst->pBuf;
	dst.mmuFlag = 1;
	dst.rotation =  pDst->rotation;
	rga_set_rect(&dst.rect, 0, 0, pDst->width, pDst->height, pDst->hor_stride, pDst->ver_stride, pDst->fmt);
	if (c_RkRgaBlit(&src, &dst, NULL)) {
		printf("%s: rga fail\n", __func__);
		ret = -1;
	}
	else {
		ret = 0;
	}

	return ret;
}

#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 1280
Player::Player(int iChnNum) :
    mChnannelNumber(iChnNum),
    mPlayingChnId(0),
	bObjIsInited(0)
{
    // ===================== 1.初始化显示资源 =====================
    // 1.1-图像缓存初始化，以及结果数组初始化
    for(int i = 0; i < mChnannelNumber; i++){
        camImg[i] = Mat(FRAME_HEIGHT, FRAME_WIDTH, CV_8UC3);
    }
    // 1.2-读写锁初始化
    for(int i = 0; i < mChnannelNumber; i++){
    	pthread_rwlock_init(&camImglock[i], NULL);
    }
    
    // ======================== 2.初始化显示屏 ========================
    init();

    // ======================== 3.初始化解码器 ========================
	// 3.1-创建解码器
	create_decoder(mChnannelNumber);
	// 3.2-初始化解码通道资源
    for(int i = 0; i < mChnannelNumber; i++){
        mChannelId[i] = 0;
    }
    
	// 3.3-向解码器申请解码通道
    for(int i = 0; i < mChnannelNumber; i++){
    	if(0 == create_decMedia_channel(&mChannelId[i])){
    		//printf("============= [%d]time ==============\n", i);
    		printf("create channel OK, chn = %u\n", mChannelId[i]);
    		
    		// 3.4-往成功申请的通道绑定解码输出处理函数
    		if(0 == mChannelId[i]){
    	    	set_decMedia_channel_callback(mChannelId[i], VideoPlayerHandle, this);
            }else if (1 == mChannelId[i]){
    	    	set_decMedia_channel_callback(mChannelId[i], VideoPlayerHandle, this);
            }else if (2 == mChannelId[i]){
    	    	set_decMedia_channel_callback(mChannelId[i], VideoPlayerHandle, this);
            }else if (3 == mChannelId[i]){
    	    	set_decMedia_channel_callback(mChannelId[i], VideoPlayerHandle, this);
            }
    	}
    }
    
    // ======================= 4.初始化处理线程 =======================
	Player_para_t *pPlayer = (Player_para_t *)malloc(sizeof(Player_para_t));	 
	pPlayer->pSelf = this;
	// 4.1-初始化轮询线程
	
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

void Player::init()
{
    if(0 == disp_init(SCREEN_WIDTH, SCREEN_HEIGHT)){
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

void Player::displayAllChn()
{
}

void Player::displayCurChn()
{
    Mat image;
    
    // 图像转换并输出
    Image srcImage, dstImage;
    memset(&srcImage, 0, sizeof(srcImage));
    memset(&dstImage, 0, sizeof(dstImage));
    
    if(IsInited()){
        image = camImg[mPlayingChnId];
        
        srcImage.fmt = RK_FORMAT_RGB_888;
        srcImage.width = image.cols;
        srcImage.height = image.rows;
        srcImage.hor_stride = srcImage.width;
        srcImage.ver_stride = srcImage.height;
        srcImage.rotation = HAL_TRANSFORM_ROT_270;
        srcImage.pBuf = image.data;
        
        dstImage.fmt = RK_FORMAT_RGB_888;
        dstImage.width = SCREEN_WIDTH;
        dstImage.height = SCREEN_HEIGHT;
        dstImage.hor_stride = dstImage.width;
        dstImage.ver_stride = dstImage.height;
        dstImage.rotation = HAL_TRANSFORM_ROT_0;
        dstImage.pBuf = mpp_malloc(char, dstImage.width*dstImage.height*3);
        if(NULL == dstImage.pBuf){
            return;
        }
        
        pthread_rwlock_rdlock(&camImglock[mPlayingChnId]);
        srcImg_ConvertTo_dstImg(&dstImage, &srcImage);
        pthread_rwlock_unlock(&camImglock[mPlayingChnId]);
        
        disp_commit(dstImage.pBuf, dstImage.width*dstImage.height*3);
        
        mpp_free(dstImage.pBuf);
    }
}

void Player::makeCamImg(int32_t chn, void *ptr, int vFmt, int w, int h, int hS, int vS)
{
    Mat image;
    Image srcImage, dstImage;
    memset(&srcImage, 0, sizeof(srcImage));
    memset(&dstImage, 0, sizeof(dstImage));
    
    srcImage.fmt = (RgaSURF_FORMAT)vFmt;
    srcImage.width = w;
    srcImage.height = h;
    srcImage.hor_stride = hS;
    srcImage.ver_stride = vS;
    srcImage.rotation = HAL_TRANSFORM_ROT_0;
    srcImage.pBuf = ptr;
    
    dstImage.fmt = RK_FORMAT_RGB_888;
    dstImage.width = camImg[chn].cols;
    dstImage.height = camImg[chn].rows;
    dstImage.hor_stride = dstImage.width;
    dstImage.ver_stride = dstImage.height;
    dstImage.rotation = HAL_TRANSFORM_ROT_0;
    dstImage.pBuf = camImg[chn].data;
    
    pthread_rwlock_wrlock(&camImglock[chn]);
    srcImg_ConvertTo_dstImg(&dstImage, &srcImage);
    pthread_rwlock_unlock(&camImglock[chn]);
    
    return ;
}

int playerInit()
{
    int ret = -1;
    int chnNum = 0;
    Player *pPlayer = NULL;
    if(0 == ini_read_int(RTSP_CLIENT_PATH, "configInfo", "enableChnNum", &chnNum)) {
        pPlayer = new Player(chnNum);
        if(NULL == pPlayer){
            printf("Player Create faild !!!\n");
        }else{
            ret = 0;
        }
    }

    if(-1 == ret)
        return ret;
    
    while(1)
    {
        sleep(5);
    }
}

