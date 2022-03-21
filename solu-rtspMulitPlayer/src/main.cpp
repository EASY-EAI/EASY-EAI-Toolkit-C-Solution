//=====================  C++  =====================
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
//=====================   C   =====================
#include "system.h"
//=====================  SDK  =====================
#include "system_opt.h"
#include "ini_wrapper.h"
//=====================  PRJ  =====================
#include "rtspCapturer.h"
#include "player.h"

typedef void * (*OSATaskEntryFunc)(void *);
//任务对象描述结构体
typedef struct 
{
    pthread_t	pthTaskHandle;			//任务句柄	
    OSATaskEntryFunc pTaskEntryFunc;	//任务入口函数
    uint32_t dwTaskPri;					//任务优先级
    uint32_t dwStackSize;				//任务栈空间大小
    uint32_t dwDetached;				//任务是否分离标志
    void * pvParam;						//传递给入口函数的参数
    void * pvRval;						//任务退出状态
}OS_TASK_DSC_S;

#define TASK_INFO_NAME_LEN 256
#define TASK_RESPAWN_RETRY_MAX_TIMES 4 // 子进程崩溃后重启重试次数
struct st_Task_Info
{
	char byTaskName[TASK_INFO_NAME_LEN];
	char byTaskPara[TASK_INFO_NAME_LEN];
	pid_t dwTaskID;
	time_t dwExitBootTime;
	time_t dwExitRunTime;
	pid_t dwExitTaskID;
	uint32_t dwExitTimes;
	int32_t dwExitStatus;
};

struct st_SysTask
{
	uint32_t dwTaskNum;
	struct st_Task_Info st_task[20];
};

static void OSTaskDelay(uint32_t dwMillisec)
{
	struct timespec delayTime, elaspedTime;
  
	delayTime.tv_sec  = dwMillisec / 1000;
	delayTime.tv_nsec = (dwMillisec % 1000) * 1000000;

	nanosleep(&delayTime, &elaspedTime);
}

#define PROCESS_RTSPCLIENT_NAME "rtspChannel"


void ShowStatus( pid_t pid, int32_t status )
{
  int flag = 1;

  printf( "[show_status]: pid = %u => ", pid );

  if ( WIFEXITED( status ) )
  {
    flag = 0;
    printf( "true if the child terminated normally, that is, "
    "by calling exit() or _exit(), or "
    "by returning from main().\n" );
	printf("子进程正常结束!\n");

  }

  if ( WEXITSTATUS( status ) )
  {
    flag = 0;
    printf( "evaluates to the least significant eight bits of the "
    "return code of  the  child  which terminated, which may "
    "have been set as the"
    "argument to a call to exit() or _exit() or as the argument for a"
    "return  statement  in  the main program.  This macro can only be"
    " evaluated if WIFEXITED returned true. \n" );
	printf("子进程exit()返回的结束代码:[%d]\n",WEXITSTATUS( status ));
	
  }

  if ( WIFSIGNALED( status ) )
  {
      flag = 0;
      printf( "\033[33m true if the child process terminated because of a signal"
	   	" which was not caught.\033[0m\n" );
	  printf("子进程是因为信号而结束!\n");
	  
  }

  if ( WTERMSIG( status ) )
  {
      flag = 0;
      printf( "  the  number of the signal that caused the child process"
	    "to terminate. This macro can only be  evaluated  if  WIFSIGNALED"
	    "returned non-zero.\n");
	  printf("子进程因信号而中止的信号代码:[%d] - [%d]\n", WIFSIGNALED( status ),WTERMSIG( status ));
	  
  }

  if ( WIFSTOPPED( status ) )
  {
      flag = 0;
      printf( "  true  if  the  child process which caused the return is"
	    "currently stopped; this is only possible if the  call  was  done"
	    "using   WUNTRACED  or  when  the  child  is  being  traced  (see"
	    "ptrace(2)).\n" );
	  printf("子进程处于暂停执行情况!\n");
	  
  }

  if ( WSTOPSIG( status ) )
  {
    flag = 0;
    printf( " the number of the signal which caused the child to stop."
	  "This   macro  can  only  be  evaluated  if  WIFSTOPPED  returned"
    "non-zero.\n" );
	printf("引发子进程暂停的信号代码:[%d]\n", WIFSTOPPED( status ));
	
  }

  if ( flag )
  {
    printf( "Unknown status = 0x%X\n", status );
  }
}

static int32_t CreateProcess(const char *pcPara, struct st_SysTask *st_TaskInfo)
{
	pid_t pid;
	uint8_t byNum = 0;
	uint8_t byFlg = 1;

	pid = fork();						//创建子进程
	if(pid == -1)						//创建失败
	{
		printf("fork error, error: %d => %s", errno, strerror(errno));
		return -1;
	}
	else if(pid != 0)					//如果是父进程,则返回
	{
		for(byNum = 0; byNum < st_TaskInfo->dwTaskNum; byNum++)
		{
			if(0 == strcmp(pcPara, st_TaskInfo->st_task[byNum].byTaskName))
			{
				st_TaskInfo->st_task[byNum].dwTaskID = pid;
				st_TaskInfo->st_task[byNum].dwExitBootTime = time(NULL);
				byFlg = 0;
			}
		}
		
		if(byFlg)
		{
			strcpy(st_TaskInfo->st_task[st_TaskInfo->dwTaskNum].byTaskName, pcPara);
			strcpy(st_TaskInfo->st_task[st_TaskInfo->dwTaskNum].byTaskPara, pcPara);
			st_TaskInfo->st_task[st_TaskInfo->dwTaskNum].dwTaskID = pid;
			st_TaskInfo->dwTaskNum++;
			st_TaskInfo->st_task[st_TaskInfo->dwTaskNum].dwExitBootTime = time(NULL);
		}
		
		OSTaskDelay(100);
		return pid; 
	}

	//执行程序文件
	if(execlp("./solu-rtspMulitPlayer","solu-rtspMulitPlayer",pcPara,(char *)0) < 0)
	{
		perror("execlp error");
		printf("execlp error: %d => %s", errno, strerror(errno));
		exit(1);
	}
    
    return 0;
}

static int32_t CreateSignalProcess(const char *pcPara, struct st_SysTask *st_TaskInfo, char *pChnId)
{
	pid_t pid;
	uint8_t byNum = 0;
	uint8_t byFlg = 1;

	pid = fork();						//创建子进程
	if(pid == -1)						//创建失败
	{
		printf("fork error, error: %d => %s", errno, strerror(errno));
		return -1;
	}
	else if(pid != 0)					//如果是父进程,则返回
	{
		for(byNum = 0; byNum < st_TaskInfo->dwTaskNum; byNum++)
		{
			if(0 == strcmp(pcPara, st_TaskInfo->st_task[byNum].byTaskName))
			{
				st_TaskInfo->st_task[byNum].dwTaskID = pid;
				st_TaskInfo->st_task[byNum].dwExitBootTime = time(NULL);
				byFlg = 0;
			}
		}
		
		if(byFlg)
		{
			strcpy(st_TaskInfo->st_task[st_TaskInfo->dwTaskNum].byTaskName, pcPara);
			strcpy(st_TaskInfo->st_task[st_TaskInfo->dwTaskNum].byTaskPara, pcPara);
			st_TaskInfo->st_task[st_TaskInfo->dwTaskNum].dwTaskID = pid;
			st_TaskInfo->dwTaskNum++;
			st_TaskInfo->st_task[st_TaskInfo->dwTaskNum].dwExitBootTime = time(NULL);
		}
		
		OSTaskDelay(100);
		return pid; 
	}

	//执行程序文件
	if(execlp("./solu-rtspMulitPlayer","solu-rtspMulitPlayer",pcPara, pChnId,(char *)0) < 0)
	{
		perror("execlp error");
		printf("execlp error: %d => %s", errno, strerror(errno));
		exit(1);
	}
    
    return 0;
}

static int rtspSignalInit(int argc, char** argv)
{
	if(argc < 2){
		::exit(0);
	}

    char channelName[64] = {0};
    sprintf(channelName, "%s_%s", argv[1], argv[2]);
    
    RtspCapturer *pRtspCapturer = new RtspCapturer(channelName);    
    pRtspCapturer->init(atoi(argv[2]));

    return 0;
}

int main(int sdwArgc, char **pcArg)
{
	struct st_SysTask st_TaskInfo;
	int32_t status;
	uint8_t byTaskNum = 0;
	pid_t waitpid;

	memset(&st_TaskInfo, 0, sizeof(st_TaskInfo));	

    /* 主进程进入此分支 */
	if(0 == strcmp(pcArg[1], "Main"))
	{
	    /* 1.根据配置文件，设置本地IP地址 */
	    char ipv4[64]={0};
	    char netMask[64]={0};
	    char gateWay[64]={0};
        ini_read_string(RTSP_CLIENT_PATH, "configInfo", "ipAddress", ipv4, sizeof(ipv4));
        ini_read_string(RTSP_CLIENT_PATH, "configInfo", "netMask", netMask, sizeof(netMask));
        ini_read_string(RTSP_CLIENT_PATH, "configInfo", "gateWay", gateWay, sizeof(gateWay));
        set_net_ipv4(ipv4, netMask, gateWay);
        
        /* 2.根据配置文件，创建播放器x1，创建取流器xN */
        int chnNum = 0;
        if(0 == ini_read_int(RTSP_CLIENT_PATH, "configInfo", "enableChnNum", &chnNum))
        {
            // 2.1 创建播放器 x 1个，
        	Player *pPlayer = NULL;
        	pPlayer = new Player(chnNum);
            if(NULL == pPlayer){
                printf("Player Create faild !!!\n");
                return -1;
            }

            // 2.2 创建Rtsp取流器 x n个，每一个都是一条独立进程
    	    char chnId[MAX_CHN_NUM] = {0};
            for(int i = 0; i < chnNum; i++){
                bzero(&chnId, sizeof(chnId));
                sprintf(chnId, "%d", i);
                CreateSignalProcess(PROCESS_RTSPCLIENT_NAME, &st_TaskInfo, chnId);
            }
        }
    
    /* 取流器子程进入此分支 */
    } else if(0 == strcmp(pcArg[1], PROCESS_RTSPCLIENT_NAME)) {
    	// 从RTSP流解码
        rtspSignalInit(sdwArgc, pcArg);
    }

    /* 主进程进入守护状态，一旦取流器子进程崩溃，则重新启动 */
	while(1)
	{
	    printf("*****************************************\n");
		printf("Waiting for the child process terminated!\n");
	    printf("*****************************************\n");
	    waitpid = wait(&status);
		if(-1 == waitpid)
		{
			printf("errno = %d,%s",errno,strerror(errno));
		}
		
	    printf("*****************************************\n");
	    printf("The ID of the child process terminated :%d\n",waitpid);
		printf("*****************************************\n");
		
		ShowStatus( waitpid, status );

		for(byTaskNum = 0; byTaskNum < st_TaskInfo.dwTaskNum; byTaskNum++)
		{
			if(st_TaskInfo.st_task[byTaskNum].dwTaskID == waitpid)
			{
				st_TaskInfo.st_task[byTaskNum].dwExitStatus = status;
				st_TaskInfo.st_task[byTaskNum].dwExitTaskID = waitpid;
				st_TaskInfo.st_task[byTaskNum].dwExitTimes++;
				st_TaskInfo.st_task[byTaskNum].dwExitRunTime = time(NULL) - st_TaskInfo.st_task[byTaskNum].dwExitBootTime;
                
				printf("ExitTaskName:[%s], ExitStatus:[%d], dwExitTaskID:[%d], dwExitTimes:[%u], dwExitRunTime:[%ld]\n",
				st_TaskInfo.st_task[byTaskNum].byTaskName,st_TaskInfo.st_task[byTaskNum].dwExitStatus, 
				st_TaskInfo.st_task[byTaskNum].dwExitTaskID,st_TaskInfo.st_task[byTaskNum].dwExitTimes, 
				st_TaskInfo.st_task[byTaskNum].dwExitRunTime);

				if(st_TaskInfo.st_task[byTaskNum].dwExitRunTime < 10)
				{
					OSTaskDelay(1000);
				}

				bool bSuccess = false;
				for (int i = 0; i < TASK_RESPAWN_RETRY_MAX_TIMES; i++)
				{
					int32_t sdwRet = 0;
					sdwRet = CreateProcess(st_TaskInfo.st_task[byTaskNum].byTaskPara, &st_TaskInfo);
					
					if (sdwRet == -1)
					{
						printf("CreateProcess failed, retry index: %d", i);
						OSTaskDelay(500);
					}
					else
					{
						printf("task \"%s\" respawned", st_TaskInfo.st_task[byTaskNum].byTaskName);
						bSuccess = true;
						break;
					}
				}
				if (!bSuccess)
				{
					printf("Failed to respawn child process \"%s\"",st_TaskInfo.st_task[byTaskNum].byTaskName);
				}
				break;
			}
		}
	}

	return 0;
}

