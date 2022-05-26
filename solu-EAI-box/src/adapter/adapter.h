#ifndef __ADAPTER_H__
#define __ADAPTER_H__
//=====================   C   =====================
#include "system.h"
#include "ipcData.h"
#include "config.h"
//=====================  SDK  =====================
#include "geometry.h"


class Adapter
{
public:
	Adapter();
	~Adapter();

	void init();
	int32_t IsInited(){return bObjIsInited;}

    int32_t sendBanAreaToAnalyzer();

    void printfData(int16_t personNum);
protected:
	
private:
    
	pthread_t mTid;
	int bObjIsInited;
};

extern int adapterInit();

#endif
