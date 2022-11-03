//=====================  PRJ  =====================
#include "analyzer.h"
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

/* algorithm input width & height */
#define FRAME_WIDTH 1280
#define FRAME_HEIGHT 720

typedef struct{
	Analyzer *pSelf;
	
}Analyzer_para_t;

int analyzerHanndle(void *pObj, IPC_MSG_t *pMsg)
{
    Analyzer *pAdapter = (Analyzer *)pObj;
    if(MSGTYPE_BANAREA_DATA == pMsg->msgType){
        memcpy(&pAdapter->BanArea[0], pMsg->payload, pMsg->msgLen);
        printf("  ############  Rect[(%d, %d)--(%d, %d)]\n",pAdapter->BanArea[0].left, pAdapter->BanArea[0].top,
                                                        pAdapter->BanArea[0].right, pAdapter->BanArea[0].bottom);
    }
    
    return 0;
}

void *cruiseCtrl_thread(void *para)
{
    // 分析器对象
	Analyzer_para_t *pAnalyzerPara = (Analyzer_para_t *)para;
    Analyzer *pSelf = pAnalyzerPara->pSelf;

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
            //pSelf->displayCurChn();
            pSelf->displayAllChn();  //-- 实时性较差
        }

        // 计时操作
        cout++;
        cout %= 50000;
        // 时间单位：10ms
		usleep(10*1000);
	}

    if(pAnalyzerPara){
        free(pAnalyzerPara);
        pAnalyzerPara = NULL;
    }

	pthread_exit(NULL);
}


static Scalar colorArray[10]={
    Scalar(255, 0, 0, 255),
    Scalar(0, 255, 0, 255),
    Scalar(0,0,139,255),
    Scalar(0,100,0,255),
    Scalar(139,139,0,255),
    Scalar(209,206,0,255),
    Scalar(0,127,255,255),
    Scalar(139,61,72,255),
    Scalar(0,255,0,255),
    Scalar(255,0,0,255),
};
static int plot_one_box(Mat src, int x1, int y1, int x2, int y2, char *label, char colour)
{
    int tl = round(0.002 * (src.rows + src.cols) / 2) + 1;
    rectangle(src, cv::Point(x1, y1), cv::Point(x2, y2), colorArray[(unsigned char)colour], 3);

    int tf = max(tl -1, 1);

    int base_line = 0;
    cv::Size t_size = getTextSize(label, FONT_HERSHEY_SIMPLEX, (float)tl/3, tf, &base_line);
    int x3 = x1 + t_size.width;
    int y3 = y1 - t_size.height - 3;

    rectangle(src, cv::Point(x1, y1), cv::Point(x3, y3), colorArray[(unsigned char)colour], -1);
    putText(src, label, cv::Point(x1, y1 - 2), FONT_HERSHEY_SIMPLEX, (float)tl/3, cv::Scalar(255, 255, 255, 255), tf, 8);
    return 0;
}
void *analysis_thread(void *para)
{
    // 播放器对象
	Analyzer_para_t *pAnalyzerPara = (Analyzer_para_t *)para;
    Analyzer *pSelf = pAnalyzerPara->pSelf;

	// 安全帽检测模型初始化
	rknn_context helmetCtx;
	helmet_detect_init(&helmetCtx, "helmet_detect.model");

	// 人员检测模型初始化
	rknn_context personCtx;
	person_detect_init(&personCtx, "person_detect.model");

    int ret = -1;
    int chnId = 0;
	Mat image;
    s32Rect_t boxRect = {0};
    double iofRatio = 0.0; // 禁区入侵比例
	while(1) {
        if(NULL == pSelf){
            msleep(5);
            break;
        }
        chnId %= pSelf->maxChnNum();    
        if(pSelf->camImg[chnId].empty()) {
            chnId++;
			usleep(5000);
            continue;
        }

        // 从当前通道取出图像
        pthread_rwlock_rdlock(&pSelf->camImglock[chnId]);
        pSelf->camImg[chnId].copyTo(image);
        pthread_rwlock_unlock(&pSelf->camImglock[chnId]);

        // 安全帽算法分析
        if(pSelf->Result[chnId].helmet_enable){
	        ret = helmet_detect_run(helmetCtx, image, &pSelf->Result[chnId].helmet_group);
            pSelf->Result[chnId].helmet_number = pSelf->Result[chnId].helmet_group.count;
        }
        
		// 人员算法分析
        if(pSelf->Result[chnId].person_enable){
		    ret = person_detect_run(personCtx, image, &pSelf->Result[chnId].person_group);
		    pSelf->Result[chnId].person_number = pSelf->Result[chnId].person_group.count;
        }

        /* 既没有发现安全帽，也没有发现人员 */
        if((ret < 0 ) || ((pSelf->Result[chnId].helmet_number <= 0)&&(pSelf->Result[chnId].person_number <= 0))){
            chnId++;
			usleep(1000);
			continue;
		}
        
		//printf("\n>>>>>>>>>>>>>>>[Chn_%02d]>>>>>>>>>>>>>>>>\n",chnId);
		//printf("person number : %d\n", pSelf->Result[chnId].person_number);
		//printf("helmet number : %d\n", pSelf->Result[chnId].helmet_number);
		// 在图像中标记 [安全帽]
		if(pSelf->Result[chnId].helmet_number > 0){
            //pSelf->sendPersonDataToNet();
        	for (int i = 0; i < pSelf->Result[chnId].helmet_number; i++) {
        		helmet_detect_result_t *det_result = &(pSelf->Result[chnId].helmet_group.results[i]);
        		if( det_result->prop < 0.3 ){
        			continue;
        		}
        		/*
        		printf("%s @ (%d %d %d %d) %f\n",
        			   det_result->name,
        			   det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom,
        			   det_result->prop);
        		*/
        		char label_text[50];
        		memset(label_text, 0 , sizeof(label_text));
        		sprintf(label_text, "%s %0.2f",det_result->name, det_result->prop); 
        		plot_one_box(image, det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom, label_text, i%10);

                boxRect.left = det_result->box.left;
                boxRect.top = det_result->box.top;
                boxRect.right = det_result->box.right;
                boxRect.bottom = det_result->box.bottom;
                iofRatio = calc_intersect_of_min_rect(boxRect, pSelf->BanArea[chnId]);
                if(iofRatio > 0.1){
                    printf("intersect of min rect ratio is %lf %%\n", iofRatio*100);
                }
        	}
        }
		// 在图像中标记 [人员]
        if(pSelf->Result[chnId].person_number > 0){
            //pSelf->sendPersonDataToNet();
    		for (int i = 0; i < pSelf->Result[chnId].person_number; i++) {
    			person_detect_result_t *det_result = &(pSelf->Result[chnId].person_group.results[i]);
    			if( det_result->prop < 0.3 ){
    				continue;
    			}
    			/*
    			printf("%s @ (%d %d %d %d) %f\n",
    				   det_result->name,
    				   det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom,
    				   det_result->prop);
    			*/    			
    			char label_text[50];
    			memset(label_text, 0 , sizeof(label_text));
    			sprintf(label_text, "%s %0.2f",det_result->name, det_result->prop); 
    			plot_one_box(image, det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom, label_text, i%10);
                
                boxRect.left = det_result->box.left;
                boxRect.top = det_result->box.top;
                boxRect.right = det_result->box.right;
                boxRect.bottom = det_result->box.bottom;
                iofRatio = calc_intersect_of_min_rect(boxRect, pSelf->BanArea[chnId]);
                if(iofRatio > 0.1){
                    //printf("intersect of min rect ratio is %lf %%\n", iofRatio*100);
                }
    		}
        }
        // 缓存好上云图片
        pthread_rwlock_wrlock(&pSelf->analImglock[chnId]);
        image.copyTo(pSelf->analImg[chnId]);
        pthread_rwlock_unlock(&pSelf->analImglock[chnId]);

        // 切换到下一个通道进行分析
        chnId++;
        usleep(30000);
	}

	person_detect_release(personCtx);
	helmet_detect_release(helmetCtx);

    if(pAnalyzerPara){
        free(pAnalyzerPara);
        pAnalyzerPara = NULL;
    }

	pthread_exit(NULL);
}

static int32_t VideoAnalyzerHandle(void *pAnalyzer,  VideoFrameData *pData)
{
	static uint64_t preTime[MAX_CHN_NUM] = {0};
    uint64_t curTime = 0;
    
    if(NULL == pAnalyzer){
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
    
    Analyzer *pSelf = (Analyzer *)pAnalyzer;
    if(NULL == pSelf){
        return -1;
    }
    if((0 == pData->err_info) && (0 == pData->discard)){
        RgaSURF_FORMAT vFmt = RK_FORMAT_YCbCr_420_SP;
        pSelf->makeCamImg(pData->channel, pData->pBuf, vFmt,pData->width, pData->height, pData->hor_stride, pData->ver_stride);

        // 里面用opencv来画框写字，性能达不到要求，后续改成用rga画则再次开放此接口
        // pSelf->displaySingleChn(pData->channel); //高实时性
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
Analyzer::Analyzer(int iChnNum) :
    mChnannelNumber(iChnNum),
    mPlayingChnId(0),
	bObjIsInited(0)
{
    // ==================== 1.初始化进程间通信资源 ====================
    IPC_client_create();
    IPC_client_init(IPC_SERVER_PORT, ANALYZER_CLI_ID);
    IPC_client_set_callback(this, analyzerHanndle);

    // ===================== 2.初始化分析算法资源 =====================
    // 1.1-图像缓存初始化，以及结果数组初始化
    for(int i = 0; i < mChnannelNumber; i++){
        camImg[i]  = Mat(FRAME_HEIGHT, FRAME_WIDTH, CV_8UC3);
        analImg[i] = Mat(FRAME_HEIGHT, FRAME_WIDTH, CV_8UC3);
	    Result[i].helmet_enable = 1;
	    Result[i].person_enable = 1;
	    Result[i].helmet_number = 0;
	    Result[i].person_number = 0;
    }
    // 1.2-读写锁初始化
    for(int i = 0; i < mChnannelNumber; i++){
    	pthread_rwlock_init(&camImglock[i], NULL);
    	pthread_rwlock_init(&analImglock[i], NULL);
    }
    // 1.3-初始化禁区
    for(int i = 0; i < mChnannelNumber; i++){
        BanArea[i].left = 0;
        BanArea[i].top = 0;
        BanArea[i].right = 0;
        BanArea[i].bottom = 0;
    }
    
    // ======================== 3.初始化显示屏 ========================
    initDisplay();

    // ======================== 4.初始化解码器 ========================
	// 4.1-创建解码器
	create_decoder(mChnannelNumber);
	// 4.2-初始化解码通道资源
    for(int i = 0; i < mChnannelNumber; i++){
        mChannelId[i] = 0;
    }
	// 4.3-向解码器申请解码通道
    for(int i = 0; i < mChnannelNumber; i++){
    	if(0 == create_decMedia_channel(&mChannelId[i])){
    		//printf("============= [%d]time ==============\n", i);
    		printf("create channel OK, chn = %u\n", mChannelId[i]);
    		// 4.4-往成功申请的通道绑定解码输出处理函数
    		if(0 == mChannelId[i]){
    	    	set_decMedia_channel_callback(mChannelId[i], VideoAnalyzerHandle, this);
            }else if (1 == mChannelId[i]){
    	    	set_decMedia_channel_callback(mChannelId[i], VideoAnalyzerHandle, this);
            }else if (2 == mChannelId[i]){
    	    	set_decMedia_channel_callback(mChannelId[i], VideoAnalyzerHandle, this);
            }else if (3 == mChannelId[i]){
    	    	set_decMedia_channel_callback(mChannelId[i], VideoAnalyzerHandle, this);
            }
    	}
    }
    
    // ======================= 5.初始化处理线程 =======================
	Analyzer_para_t *pAnalyzer = (Analyzer_para_t *)malloc(sizeof(Analyzer_para_t));	 
	pAnalyzer->pSelf = this;
	// 5.1-初始化轮询线程
	if(0 != CreateNormalThread(cruiseCtrl_thread, pAnalyzer, &mTid)){
		free(pAnalyzer);
        return ;
	}
#if 1    
	// 5.2-初始化分析线程
	if(0 != CreateNormalThread(analysis_thread, pAnalyzer, &mAnalTid)){
		free(pAnalyzer);
        return ;
	}
#endif
}

Analyzer::~Analyzer()
{
    for(int i = 0; mChnannelNumber < 1; i++){
	    close_decMedia_channel(mChannelId[i]);
    }
    deInitDisplay();
}

void Analyzer::initDisplay()
{
    // 显示屏初始化
	disp_screen_t screen = {0};

	screen.screen_width = SCREEN_WIDTH;
	screen.screen_height = SCREEN_HEIGHT;
	screen.wins[0].enable = 1;
	screen.wins[0].in_fmt = IMAGE_TYPE_RGB888;
	screen.wins[0].in_w = FRAME_WIDTH;
	screen.wins[0].in_h = FRAME_HEIGHT;
	screen.wins[0].HorStride = FRAME_WIDTH;
	screen.wins[0].VirStride = FRAME_HEIGHT;
	screen.wins[0].crop_x = 0;
	screen.wins[0].crop_y = 0;
	screen.wins[0].crop_w = FRAME_WIDTH;
	screen.wins[0].crop_h = FRAME_HEIGHT;
	screen.wins[0].rotation = 270;
	screen.wins[0].win_x = 0;
	screen.wins[0].win_y = 0;
	screen.wins[0].win_w = SCREEN_WIDTH/2;
	screen.wins[0].win_h = SCREEN_HEIGHT/2;

	screen.wins[1].enable = 1;
	screen.wins[1].in_fmt = IMAGE_TYPE_RGB888;
	screen.wins[1].in_w = FRAME_WIDTH;
	screen.wins[1].in_h = FRAME_HEIGHT;
	screen.wins[1].HorStride = FRAME_WIDTH;
	screen.wins[1].VirStride = FRAME_HEIGHT;
	screen.wins[1].crop_x = 0;
	screen.wins[1].crop_y = 0;
	screen.wins[1].crop_w = FRAME_WIDTH;
	screen.wins[1].crop_h = FRAME_HEIGHT;
	screen.wins[1].rotation = 270;
	screen.wins[1].win_x = SCREEN_WIDTH/2;
	screen.wins[1].win_y = 0;
	screen.wins[1].win_w = SCREEN_WIDTH/2;
	screen.wins[1].win_h = SCREEN_HEIGHT/2;

	screen.wins[2].enable = 1;
	screen.wins[2].in_fmt = IMAGE_TYPE_RGB888;
	screen.wins[2].in_w = FRAME_WIDTH;
	screen.wins[2].in_h = FRAME_HEIGHT;
	screen.wins[2].HorStride = FRAME_WIDTH;
	screen.wins[2].VirStride = FRAME_HEIGHT;
	screen.wins[2].crop_x = 0;
	screen.wins[2].crop_y = 0;
	screen.wins[2].crop_w = FRAME_WIDTH;
	screen.wins[2].crop_h = FRAME_HEIGHT;
	screen.wins[2].rotation = 270;
	screen.wins[2].win_x = 0;
	screen.wins[2].win_y = SCREEN_HEIGHT/2;
	screen.wins[2].win_w = SCREEN_WIDTH/2;
	screen.wins[2].win_h = SCREEN_HEIGHT/2;
    
	screen.wins[3].enable = 1;
	screen.wins[3].in_fmt = IMAGE_TYPE_RGB888;
	screen.wins[3].in_w = FRAME_WIDTH;
	screen.wins[3].in_h = FRAME_HEIGHT;
	screen.wins[3].HorStride = FRAME_WIDTH;
	screen.wins[3].VirStride = FRAME_HEIGHT;
	screen.wins[3].crop_x = 0;
	screen.wins[3].crop_y = 0;
	screen.wins[3].crop_w = FRAME_WIDTH;
	screen.wins[3].crop_h = FRAME_HEIGHT;
	screen.wins[3].rotation = 270;
	screen.wins[3].win_x = SCREEN_WIDTH/2;
	screen.wins[3].win_y = SCREEN_HEIGHT/2;
	screen.wins[3].win_w = SCREEN_WIDTH/2;
	screen.wins[3].win_h = SCREEN_HEIGHT/2;
    
    if(0 == disp_init_pro(&screen)){
        bObjIsInited = 1;
    }else{
        bObjIsInited = 0;
    }
}

void Analyzer::deInitDisplay()
{
    if(IsInited()){
        // 显示屏去初始化
        disp_exit_pro();
    }
}

void Analyzer::displaySingleChn(int chn)
{
    // 标记图像
    bool bIsNeedToMark = false;
    Mat image;

    if(IsInited()) {
#if 0   //仅显示rtsp流图像
        pthread_rwlock_rdlock(&camImglock[chn]);
        disp_commit_pro(camImg[chn].data, chn, camImg[chn].cols*camImg[chn].rows*3);
        pthread_rwlock_unlock(&camImglock[chn]);
#else   //显示rtsp流图像，并在上面进行标记

        // 取图片
        if((0 < Result[chn].helmet_number) || (0 < Result[chn].person_number) ||
            !((0 == BanArea[chn].left)&&(0 == BanArea[chn].top)&&(0 == BanArea[chn].right)&&(0 == BanArea[chn].bottom))
         ){
            bIsNeedToMark = true;
            pthread_rwlock_rdlock(&camImglock[chn]);
            camImg[chn].copyTo(image);
            pthread_rwlock_unlock(&camImglock[chn]);
        }else{
            bIsNeedToMark = false;
            image = camImg[chn];
        }

        // 标记禁区
        if(!((0 == BanArea[chn].left)&&(0 == BanArea[chn].top)&&(0 == BanArea[chn].right)&&(0 == BanArea[chn].bottom))){
            const char *textLab = "banArea";
            plot_one_box(image, BanArea[chn].left, BanArea[chn].top, BanArea[chn].right, BanArea[chn].bottom, (char *)textLab, 0);
        }
        
        if(bIsNeedToMark){
            // 在图像中标记 [安全帽]
            if(Result[chn].helmet_number > 0){
                for (int i = 0; i < Result[chn].helmet_number; i++) {
                    helmet_detect_result_t *det_result = &(Result[chn].helmet_group.results[i]);
                    if( det_result->prop < 0.3 ){
                        continue;
                    }
                    /*
                    printf("%s @ (%d %d %d %d) %f\n",
                           det_result->name,
                           det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom,
                           det_result->prop);
                    */
                    char label_text[50];
                    memset(label_text, 0 , sizeof(label_text));
                    sprintf(label_text, "%s %0.2f",det_result->name, det_result->prop);
                    plot_one_box(image, det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom, label_text, i%10);
                }
            }
            // 在图像中标记 [人员]
            if(Result[chn].person_number > 0){
                for (int i = 0; i < Result[chn].person_number; i++) {
                    person_detect_result_t *det_result = &(Result[chn].person_group.results[i]);           
                    if( det_result->prop < 0.3 ){
                        continue;
                    }
                    /*
                    printf("%s @ (%d %d %d %d) %f\n",
                           det_result->name,
                           det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom,
                           det_result->prop);
                    */
                    char label_text[50];
                    memset(label_text, 0 , sizeof(label_text));
                    sprintf(label_text, "%s %0.2f",det_result->name, det_result->prop); 
                    plot_one_box(image, det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom, label_text, i%10);
                }
            }
            
            disp_commit_pro(image.data, chn, image.cols*image.rows*3);
        }else{
            
            pthread_rwlock_rdlock(&camImglock[chn]);
            disp_commit_pro(image.data, chn, image.cols*image.rows*3);
            pthread_rwlock_unlock(&camImglock[chn]);
        }
#endif
    }
}

void Analyzer::displayAllChn()
{
    // 标记图像
    bool bIsNeedToMark = false;
    Mat image[4];
    
    if(IsInited()) {
        for(int chn = 0; chn < mChnannelNumber; chn++) {
#if 0   //仅显示rtsp流图像
            pthread_rwlock_rdlock(&camImglock[chn]);
            disp_commit_pro(camImg[chn].data, chn, camImg[chn].cols*camImg[chn].rows*3);
            pthread_rwlock_unlock(&camImglock[chn]);
#else   //显示rtsp流图像，并在上面进行标记

            // 取图片
            if((0 < Result[chn].helmet_number) || (0 < Result[chn].person_number) ||
                !((0 == BanArea[chn].left)&&(0 == BanArea[chn].top)&&(0 == BanArea[chn].right)&&(0 == BanArea[chn].bottom))
             ){
                bIsNeedToMark = true;
                pthread_rwlock_rdlock(&camImglock[chn]);
                camImg[chn].copyTo(image[chn]);
                pthread_rwlock_unlock(&camImglock[chn]);
            }else{
                bIsNeedToMark = false;
                image[chn] = camImg[chn];
            }

            // 标记禁区
            if(!((0 == BanArea[chn].left)&&(0 == BanArea[chn].top)&&(0 == BanArea[chn].right)&&(0 == BanArea[chn].bottom))){
                const char *textLab = "banArea";
                plot_one_box(image[chn], BanArea[chn].left, BanArea[chn].top, BanArea[chn].right, BanArea[chn].bottom, (char *)textLab, 0);
            }
            
            if(bIsNeedToMark){
        		// 在图像中标记 [安全帽]
        		if(Result[chn].helmet_number > 0){
                	for (int i = 0; i < Result[chn].helmet_number; i++) {
                		helmet_detect_result_t *det_result = &(Result[chn].helmet_group.results[i]);
                		if( det_result->prop < 0.3 ){
                			continue;
                		}
                		/*
                		printf("%s @ (%d %d %d %d) %f\n",
                			   det_result->name,
                			   det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom,
                			   det_result->prop);
                		*/
                		char label_text[50];
                		memset(label_text, 0 , sizeof(label_text));
                		sprintf(label_text, "%s %0.2f",det_result->name, det_result->prop);
                		plot_one_box(image[chn], det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom, label_text, i%10);
                	}
                }
                // 在图像中标记 [人员]
                if(Result[chn].person_number > 0){
                    for (int i = 0; i < Result[chn].person_number; i++) {
                        person_detect_result_t *det_result = &(Result[chn].person_group.results[i]);           
                        if( det_result->prop < 0.3 ){
                            continue;
                        }
                        /*
                        printf("%s @ (%d %d %d %d) %f\n",
                               det_result->name,
                               det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom,
                               det_result->prop);
                        */
                        char label_text[50];
                        memset(label_text, 0 , sizeof(label_text));
                        sprintf(label_text, "%s %0.2f",det_result->name, det_result->prop); 
                        plot_one_box(image[chn], det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom, label_text, i%10);
                    }
                }
                
                disp_commit_pro(image[chn].data, chn, image[chn].cols*image[chn].rows*3);
            }else{
                
                pthread_rwlock_rdlock(&camImglock[chn]);
                disp_commit_pro(image[chn].data, chn, image[chn].cols*image[chn].rows*3);
                pthread_rwlock_unlock(&camImglock[chn]);
            }
#endif
        }
    }

}

void Analyzer::displayCurChn()
{
    // 标记图像
    bool bIsNeedToMark = false;
    Mat image;

    if((0 < Result[mPlayingChnId].helmet_number) || (0 < Result[mPlayingChnId].person_number) ||
        !((0 == BanArea[mPlayingChnId].left)&&(0 == BanArea[mPlayingChnId].top)&&(0 == BanArea[mPlayingChnId].right)&&(0 == BanArea[mPlayingChnId].bottom))
     ){
        bIsNeedToMark = true;
        pthread_rwlock_rdlock(&camImglock[mPlayingChnId]);
        camImg[mPlayingChnId].copyTo(image);
        pthread_rwlock_unlock(&camImglock[mPlayingChnId]);
    }else{
        image = camImg[mPlayingChnId];
    }
    
    if(!((0 == BanArea[mPlayingChnId].left)&&(0 == BanArea[mPlayingChnId].top)&&(0 == BanArea[mPlayingChnId].right)&&(0 == BanArea[mPlayingChnId].bottom))){
        const char *textLab = "banArea";
        plot_one_box(image, BanArea[mPlayingChnId].left, BanArea[mPlayingChnId].top, BanArea[mPlayingChnId].right, BanArea[mPlayingChnId].bottom, (char *)textLab, 0);
    }
    
    if(bIsNeedToMark){        
		// 在图像中标记 [安全帽]
		if(Result[mPlayingChnId].helmet_number > 0){
        	for (int i = 0; i < Result[mPlayingChnId].helmet_number; i++) {
        		helmet_detect_result_t *det_result = &(Result[mPlayingChnId].helmet_group.results[i]);
        		if( det_result->prop < 0.3 ){
        			continue;
        		}
        		/*
        		printf("%s @ (%d %d %d %d) %f\n",
        			   det_result->name,
        			   det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom,
        			   det_result->prop);
        		*/
        		char label_text[50];
        		memset(label_text, 0 , sizeof(label_text));
        		sprintf(label_text, "%s %0.2f",det_result->name, det_result->prop);
        		plot_one_box(image, det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom, label_text, i%10);
        	}
        }
        // 在图像中标记 [人员]
        if(Result[mPlayingChnId].person_number > 0){
            for (int i = 0; i < Result[mPlayingChnId].person_number; i++) {
                person_detect_result_t *det_result = &(Result[mPlayingChnId].person_group.results[i]);           
                if( det_result->prop < 0.3 ){
                    continue;
                }
                /*
                printf("%s @ (%d %d %d %d) %f\n",
                       det_result->name,
                       det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom,
                       det_result->prop);
                */
                char label_text[50];
                memset(label_text, 0 , sizeof(label_text));
                sprintf(label_text, "%s %0.2f",det_result->name, det_result->prop); 
                plot_one_box(image, det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom, label_text, i%10);
            }
        }
    }

    // 图像转换并输出
    Image srcImage, dstImage;
    memset(&srcImage, 0, sizeof(srcImage));
    memset(&dstImage, 0, sizeof(dstImage));
    
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
    if(IsInited()){
        if(bIsNeedToMark){
            srcImg_ConvertTo_dstImg(&dstImage, &srcImage);
        }else{
            pthread_rwlock_rdlock(&camImglock[mPlayingChnId]);
            srcImg_ConvertTo_dstImg(&dstImage, &srcImage);
            pthread_rwlock_unlock(&camImglock[mPlayingChnId]);
        }
        disp_commit(dstImage.pBuf, dstImage.width*dstImage.height*3);
    }
    mpp_free(dstImage.pBuf);
    
    return ;
}

void Analyzer::makeCamImg(int32_t chn, void *ptr, int vFmt, int w, int h, int hS, int vS)
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
    
    dstImage.fmt = RK_FORMAT_BGR_888;
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

int32_t Analyzer::sendPersonDataToNet()
{
    int32_t ret = -1;
    int16_t person_number = Result[mPlayingChnId].person_number;
    ret = IPC_client_sendData(ADAPTER_CLI_ID, MSGTYPE_PERSON_DATA, &person_number, sizeof(person_number));
    return ret;
}
int32_t Analyzer::sendHelmetDataToNet()
{
    int32_t ret = -1;
    int16_t helmet_number = Result[mPlayingChnId].helmet_number;
    ret = IPC_client_sendData(ADAPTER_CLI_ID, MSGTYPE_HELMET_DATA, &helmet_number, sizeof(helmet_number));
    return ret;
}



int analyzerInit()
{
    int ret = -1;
    int chnNum = 0;
    Analyzer *pAnalyzer = NULL;
    if(0 == ini_read_int(RTSP_CLIENT_PATH, "configInfo", "enableChnNum", &chnNum)) {
        pAnalyzer = new Analyzer(chnNum);
        if(NULL == pAnalyzer){
            printf("Analyzer Create faild !!!\n");
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

