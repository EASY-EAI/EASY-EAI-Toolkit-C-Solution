#ifndef __PLAYER_H__
#define __PLAYER_H__

#define MAX_CHN_NUM 16

using namespace std;

class Player
{
public:
	Player(int iChnNum = 1);
	~Player();

	void init(int width, int height);
    void deInit();
	int IsInited(){return bObjIsInited;}

    uint32_t maxChnNum(){return mChnannelNumber;}
    uint32_t playingChn(){return mPlayingChnId;}
    void setPlayingChn(uint32_t chnId){mPlayingChnId = chnId;}

    void displayCommit(void *ptr, int fd, int fmt, int w, int h, int rotation);



protected:
	
private:
	pthread_t mTid;
    
    int32_t mChnannelNumber;
    uint32_t mPlayingChnId;
    uint32_t mChannelId[MAX_CHN_NUM];

	int bObjIsInited;
	
};



#endif
