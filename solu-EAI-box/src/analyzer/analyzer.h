#ifndef __ANALYZER_H__
#define __ANALYZER_H__
//=====================  C++  =====================
#include <iostream>
#include <fstream>
#include <mutex>
#include <opencv2/opencv.hpp>
//=====================   C   =====================
#include "system.h"
#include "ipcData.h"
#include "config.h"
//=====================  SDK  =====================
#include "geometry.h"
#include "person_detect.h"
#include "helmet_detect.h"

using namespace std;
using namespace cv;

#define MAX_CHN_NUM 16

typedef struct{
	helmet_detect_result_group_t helmet_group;
	person_detect_result_group_t person_group;
    int16_t helmet_enable;
    int16_t person_enable;
	int16_t helmet_number;
	int16_t person_number;
}Result_t;

class Analyzer
{
public:
	Analyzer(int iChnNum = 1);
	~Analyzer();

	void initDisplay();
    void deInitDisplay();
	int IsInited(){return bObjIsInited;}

    uint32_t maxChnNum(){return mChnannelNumber;}
    uint32_t playingChn(){return mPlayingChnId;}
    void setPlayingChn(uint32_t chnId){mPlayingChnId = chnId;}

    void displaySingleChn(int chn);
    
    void displayAllChn();
    void displayCurChn();

    // 解码器组装camImg
    void makeCamImg(int32_t chn, void *ptr, int vFmt, int w, int h, int hS, int vS);

    // 解码器输出数据 - YUV格式
    pthread_rwlock_t camImglock[MAX_CHN_NUM];
    Mat camImg[MAX_CHN_NUM];

    // 分析并画好框的数据 - RGB888
    pthread_rwlock_t analImglock[MAX_CHN_NUM];
    Mat analImg[MAX_CHN_NUM];

    s32Rect_t BanArea[MAX_CHN_NUM];
    Result_t Result[MAX_CHN_NUM];

    int32_t sendPersonDataToNet();
    int32_t sendHelmetDataToNet();
    
protected:
	
private:
    
	pthread_t mTid;
	pthread_t mAnalTid;
    
    int32_t mChnannelNumber;
    uint32_t mPlayingChnId;
    uint32_t mChannelId[MAX_CHN_NUM];

	int bObjIsInited;
	
};

extern int analyzerInit();


#endif
