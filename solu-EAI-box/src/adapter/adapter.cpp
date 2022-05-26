//=====================  C++  =====================
#include <iostream>
#include <string>
#include <queue>
//=====================   C   =====================
#include "system.h"
#include "ipcData.h"
#include "config.h"
//=====================  SDK  =====================
#include "system_opt.h"
//=====================  PRJ  =====================
#include "adapter.h"

using namespace std;

typedef struct{
	Adapter *pSelf;
	
}Adapter_para_t;

int adapterHanndle(void *pObj, IPC_MSG_t *pMsg)
{
    Adapter *pAdapter = (Adapter *)pObj;
    if(MSGTYPE_PERSON_DATA == pMsg->msgType){
        int16_t personNum;
        memcpy(&personNum, pMsg->payload, pMsg->msgLen);
        pAdapter->printfData(personNum);
    }else if(MSGTYPE_HELMET_DATA == pMsg->msgType){
        int16_t helmetNum;
        memcpy(&helmetNum, pMsg->payload, pMsg->msgLen);
        pAdapter->printfData(helmetNum);
    }
    
    return 0;
}


Adapter::Adapter() :
	bObjIsInited(0)
{
}

Adapter::~Adapter()
{
	printf("destroy thread succ...\n");
}

void Adapter::init()
{
    // ==================== 1.初始化进程间通信资源 ====================
    IPC_client_create();
    IPC_client_init(IPC_SERVER_PORT, ADAPTER_CLI_ID);
    IPC_client_set_callback(this, adapterHanndle);

	bObjIsInited = 1;
}

void Adapter::printfData(int16_t personNum)
{
    printf("adapter person number is :%d\n", personNum);
}

int32_t Adapter::sendBanAreaToAnalyzer()
{
    int32_t ret = -1;
	s32Rect_t bandArea = {950, 300, 1275, 715};
    ret = IPC_client_sendData(ANALYZER_CLI_ID, MSGTYPE_BANAREA_DATA, &bandArea, sizeof(bandArea));
    return ret;
}

int adapterInit()
{
    int ret = -1;
    Adapter *pAdapter = NULL;
    pAdapter = new Adapter();
    if(NULL == pAdapter){
        printf("pAdapter Create faild !!!\n");
    }else{
        ret = 0;
    }

    if(-1 == ret)
        return ret;
    
    pAdapter->init();
    
    sleep(2);
    pAdapter->sendBanAreaToAnalyzer();
    while(1)
    {
        sleep(5);
    }
}


