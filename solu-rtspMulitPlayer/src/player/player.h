#ifndef __PLAYER_H__
#define __PLAYER_H__
//=====================  C++  =====================
#include <iostream>
#include <fstream>
#include <mutex>
#include <opencv2/opencv.hpp>
//=====================   C   =====================
#include "system.h"
#include "ipcData.h"
#include "config.h"

using namespace std;
using namespace cv;

#define MAX_CHN_NUM 16


class Player
{
public:
	Player(int iChnNum = 1);
	~Player();

	void init();
    void deInit();
	int IsInited(){return bObjIsInited;}

    uint32_t maxChnNum(){return mChnannelNumber;}
    uint32_t playingChn(){return mPlayingChnId;}
    void setPlayingChn(uint32_t chnId){mPlayingChnId = chnId;}
    
    void displayAllChn();
    void displayCurChn();

    // 解码器组装camImg
    void makeCamImg(int32_t chn, void *ptr, int vFmt, int w, int h, int hS, int vS);

    // 解码器输出数据 - RGB格式
    pthread_rwlock_t camImglock[MAX_CHN_NUM];
    Mat camImg[MAX_CHN_NUM];
protected:
	
private:
    
	pthread_t mTid;
    
    int32_t mChnannelNumber;
    uint32_t mPlayingChnId;
    uint32_t mVideoChannelId[MAX_CHN_NUM];
    uint32_t mAudioChannelId[MAX_CHN_NUM];

	int bObjIsInited;
	
};

extern int playerInit();


#endif
