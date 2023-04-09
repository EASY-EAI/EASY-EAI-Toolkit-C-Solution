//=====================  C++  =====================
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
//=====================   C   =====================
#include "system.h"
#include "network.h"
#include "ipcData.h"
#include "config.h"
//=====================  SDK  =====================
#include "system_opt.h"
#include "ini_wrapper.h"
//#include "ipc.h"
//=====================  PRJ  =====================
#include "capturer/rtspCapturer.h"
#include "player/player.h"

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

#define PROGRAM_NAME_LEN 64 //主进程名
#define TASK_MAX_NUMBER 20  //子进程总数
struct st_SysTask
{
    char progName[PROGRAM_NAME_LEN];
	uint32_t dwTaskNum;
	struct st_Task_Info st_task[TASK_MAX_NUMBER];
};


#define PROCESS_PLAYER_NAME   "player"
#define PROCESS_RTSPCLIENT_NAME "rtspChannel"

static void ShowStatus( pid_t pid, int32_t status )
{
  int flag = 1;

  printf( "[show_status]: pid = %u => ", pid );

  if ( WIFEXITED( status ) ) {
    flag = 0;
    printf( "true if the child terminated normally, that is, "
    "by calling exit() or _exit(), or "
    "by returning from main().\n" );
	printf("子进程正常结束!\n");
  }
  if ( WEXITSTATUS( status ) ) {
    flag = 0;
    printf( "evaluates to the least significant eight bits of the "
    "return code of  the  child  which terminated, which may "
    "have been set as the"
    "argument to a call to exit() or _exit() or as the argument for a"
    "return  statement  in  the main program.  This macro can only be"
    " evaluated if WIFEXITED returned true. \n" );
	printf("子进程exit()返回的结束代码:[%d]\n",WEXITSTATUS( status ));
  }
  if ( WIFSIGNALED( status ) ) {
      flag = 0;
      printf( "\033[33m true if the child process terminated because of a signal"
	   	" which was not caught.\033[0m\n" );
	  printf("子进程是因为信号而结束!\n");
  }
  if ( WTERMSIG( status ) ) {
      flag = 0;
      printf( "  the  number of the signal that caused the child process"
	    "to terminate. This macro can only be  evaluated  if  WIFSIGNALED"
	    "returned non-zero.\n");
	  printf("子进程因信号而中止的信号代码:[%d] - [%d]\n", WIFSIGNALED( status ),WTERMSIG( status ));
  }
  if ( WIFSTOPPED( status ) ) {
      flag = 0;
      printf( "  true  if  the  child process which caused the return is"
	    "currently stopped; this is only possible if the  call  was  done"
	    "using   WUNTRACED  or  when  the  child  is  being  traced  (see"
	    "ptrace(2)).\n" );
	  printf("子进程处于暂停执行情况!\n");
	  
  }
  if ( WSTOPSIG( status ) ) {
    flag = 0;
    printf( " the number of the signal which caused the child to stop."
	  "This   macro  can  only  be  evaluated  if  WIFSTOPPED  returned"
    "non-zero.\n" );
	printf("引发子进程暂停的信号代码:[%d]\n", WIFSTOPPED( status ));
	
  }
  if ( flag ) {
    printf( "Unknown status = 0x%X\n", status );
  }
}

static int32_t CreateProcess(const char *pcPara, struct st_SysTask *st_TaskInfo)
{
	pid_t pid;
	uint8_t byNum = 0;
	uint8_t byFlg = 1;

    if(TASK_MAX_NUMBER <= st_TaskInfo->dwTaskNum){
        printf("[error]-The number of processes exceeds the limit, Max Task number is : %d\n", TASK_MAX_NUMBER);
        return -1;
    }

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
		
		osTask_msDelay(100);
		return pid; 
	}

    char cmd[PROGRAM_NAME_LEN+2] = {0};
    sprintf(cmd,"./%s",st_TaskInfo->progName);
	//执行程序文件
	if(execlp(cmd, st_TaskInfo->progName, pcPara, (char *)0) < 0)
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

    if(TASK_MAX_NUMBER <= st_TaskInfo->dwTaskNum){
        printf("[error]-The number of processes exceeds the limit, Max Task number is : %d\n", TASK_MAX_NUMBER);
        return -1;
    }

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
		
		osTask_msDelay(100);
		return pid; 
	}

    char cmd[PROGRAM_NAME_LEN+2] = {0};
    sprintf(cmd,"./%s", st_TaskInfo->progName);
	//执行程序文件
	if(execlp(cmd, st_TaskInfo->progName, pcPara, pChnId, (char *)0) < 0)
	{
		perror("execlp error");
		printf("execlp error: %d => %s", errno, strerror(errno));
		exit(1);
	}
    
    return 0;
}

int main(int sdwArgc, char **pcArg)
{
	struct st_SysTask st_TaskInfo;
	int32_t status;
	uint8_t byTaskNum = 0;
	pid_t waitpid;

	memset(&st_TaskInfo, 0, sizeof(st_TaskInfo));
    strcpy(st_TaskInfo.progName, pcArg[0]+2);   // +2偏移掉"./"

    /* 主进程进入此分支 */
	if(0 == strcmp(pcArg[1], "Main"))
	{
	    /* 1.根据配置文件，设置本地IP地址 */
	    char tagIpv4[64]={0};
	    char tagNetMask[64]={0};
	    char tagGateWay[64]={0};
        ini_read_string(RTSP_CLIENT_PATH, "configInfo", "ipAddress", tagIpv4, sizeof(tagIpv4));
        ini_read_string(RTSP_CLIENT_PATH, "configInfo", "netMask", tagNetMask, sizeof(tagNetMask));
        //ini_read_string(RTSP_CLIENT_PATH, "configInfo", "gateWay", tagGateWay, sizeof(tagGateWay));
        
	    char curIpv4[64]={0};
	    char curNetMask[64]={0};
	    //char curGateWay[64]={0};
        get_local_Ip("eth0", curIpv4, sizeof(curIpv4));
        get_local_NetMask("eth0", curNetMask, sizeof(curNetMask));
        //get_local_GateWay("eth0", curGateWay, sizeof(curGateWay));
        if((0 != strcmp(tagIpv4, curIpv4))||(0 != strcmp(tagNetMask, curNetMask)) /*||(0 != strcmp(tagGateWay, curGateWay))*/){
            // 重设IP地址
            printf("need reSet network parameter!\n");
            set_ipv4_static("eth0", tagIpv4, tagNetMask, tagGateWay);
            restart_network_device();
            usleep(2500*1000);
        }
        
        
        /* 2.根据配置文件，创建播放器x1，创建取流器xN */
        int chnNum = 0;
        if(0 == ini_read_int(RTSP_CLIENT_PATH, "configInfo", "enableChnNum", &chnNum)){
                        
            // 2.1 创建播放器 x 1个
            CreateProcess(PROCESS_PLAYER_NAME, &st_TaskInfo);

            // 2.2 创建Rtsp取流器 x n个，每一个都是一条独立进程
    	    char chnId[MAX_CHN_NUM] = {0};
            for(int i = 0; i < chnNum; i++){
                bzero(&chnId, sizeof(chnId));
                sprintf(chnId, "%d", i);
                CreateSignalProcess(PROCESS_RTSPCLIENT_NAME, &st_TaskInfo, chnId);
            }
        }else{
			return -1;
        }
    
    /* 播放器子程进入此分支 */
    } else if(0 == strcmp(pcArg[1], PROCESS_PLAYER_NAME)) {
        playerInit();
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
					osTask_msDelay(1000);
				}

				bool bSuccess = false;
				for (int i = 0; i < TASK_RESPAWN_RETRY_MAX_TIMES; i++)
				{
					int32_t sdwRet = 0;
					sdwRet = CreateProcess(st_TaskInfo.st_task[byTaskNum].byTaskPara, &st_TaskInfo);
					
					if (sdwRet == -1)
					{
						printf("CreateProcess failed, retry index: %d", i);
						osTask_msDelay(500);
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

