#include "system.h"
#include <linux/input.h>

#include "system_opt.h"

#include "keyEvent.h"

KeyHandle g_handle = NULL;
void *keyEvent_thread_entry(void *para)
{
    FILE *fp;
    struct input_event ie;
    fp = fopen(KEY_EVENT_PATH, "r");  
    if (fp == NULL) {
        return NULL;
    }
	
    while (1) {
        fread((void *)&ie, sizeof(ie), 1, fp);
        if (ferror(fp)) {
            perror("fread");
			return NULL;
        }
		
//        printf("[timeval:sec:%ld,usec:%ld,type:%d,code:%d,value:%d]\n",ie.time.tv_sec, ie.time.tv_usec,  ie.type, ie.code, ie.value);
		if(g_handle){
			if((ie.type == 1) && (ie.code == 1) && (ie.value == 1)){
				g_handle(KEY_PRESS);
			}else if((ie.type == 1) && (ie.code == 1) && (ie.value == 0)){
				g_handle(KEY_RELEASE);
#if 0
			}else if(1 == ie.code){
				g_handle(KEY_CLICK);
			}else if(1 == ie.code){
				g_handle(KEY_DOUBLECLICK);
#endif
			}
		}
		usleep(50*1000);
    }
	
    return NULL;	
}

int keyEvent_init(void)
{
	// 创建一个线程“监听”和“分发”按键事件
	pthread_t mTid;
	if(0 != CreateNormalThread(keyEvent_thread_entry, NULL, &mTid)){
		return -1;
	}
	return 0;
}

int set_event_handle(KeyHandle handle)
{
	if(NULL == handle)
		return -1;
	
	g_handle = handle;
	
	return 0;
}