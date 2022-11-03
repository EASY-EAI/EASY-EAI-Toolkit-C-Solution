#ifndef __ENCODER_H__
#define __ENCODER_H__
//=====================   C   =====================
#include "system.h"
#include "config.h"
//=====================  SDK  =====================
#include "frame_queue.h"
#include "endeCode_api.h"

#if 0
class EnCoder
{
public:
	EnCoder();
	~EnCoder();

	void init();
	int32_t IsInited(){return bObjIsInited;}

    int32_t sendPublicInfoToHttpAdaper();
    int32_t sendAlarmSrvDataToHttpAdaper();
    int32_t send3rdPlatformDataToHttpAdaper();
    int32_t sendAlgoSupportDataToHttpAdaper();
    int32_t sendBanAreaToAnalyzer();

protected:
	
private:
    int32_t mListMaxNum;
    std::string strModelList;
    
	pthread_t mTid;
	int bObjIsInited;
};
#endif

extern int enCoderInit(const char *moduleName);

#endif
