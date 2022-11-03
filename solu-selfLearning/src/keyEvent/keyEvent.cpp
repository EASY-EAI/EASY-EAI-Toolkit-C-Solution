#include "system.h"
#include <linux/input.h>
#include "keyEvent.h"

#include "system_opt.h"

KeyHandle g_handle = NULL;

//按键检测线程回调
void *keyEvent_thread_entry(void *para)
{
    FILE *fp;
    struct input_event ie;
    fp = fopen(KEY_EVENT_PATH,"r");
    if(NULL == fp)
    {
        return NULL;
    }
    while (1)
    {
        /* code */
        fread((void *)&ie,sizeof(ie),1,fp);
        if(ferror(fp))
        {
            perror("fread");
            return NULL;
        }
        printf("time(sec):%ld   (usec):%ld  type:%d  code:%d   value:%d\n",ie.time.tv_sec,  ie.time.tv_usec,  ie.type  ,ie.code  ,ie.value);
        if(NULL != g_handle)
        {
            if((ie.type == EV_KEY) && (ie.code == 1) && (ie.value == 1) )
            {
                g_handle(KEY_PRESS);
            }
            else if((ie.type == EV_KEY) && (ie.code == 1) && (ie.value == 0))
            {
                g_handle(KEY_RELEASE);
            }
        }
        usleep(50*1000);
    }
}
//初始化按键
int  Init_KeyEven()
{
    pthread_t tid;
    if(0 != CreateNormalThread(keyEvent_thread_entry,NULL,&tid))
    {
        return -1;
    }
    return 0;
}


//回调
int set_even_handle(KeyHandle handle)
{
    if(NULL == handle)
    {
        return -1;
    }
    g_handle = handle;
    return 0;
}




