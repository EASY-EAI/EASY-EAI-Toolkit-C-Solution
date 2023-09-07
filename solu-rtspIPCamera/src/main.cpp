/**
*
* Copyright 2022 by Guangzhou Easy EAI Technologny Co.,Ltd.
* website: www.easy-eai.com
*
* Author: Jiehao.Zhong <zhongjiehao@easy-eai.com>
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* License file for more details.
* 
*/

//=====================  C++  =====================
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
//=====================   C   =====================
#include "system.h"
#include "config.h"
//=====================  SDK  =====================
#include "system_opt.h"
//=====================  PRJ  =====================
#include "rtspServer/rtspServer.h"
#include "enCoder/enCoder.h"

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


#define PROCESS_ENCODER_NAME   "enCoder"
#define PROCESS_RTSPSERVER_NAME "rtspServer"

void ShowStatus( pid_t pid, int32_t status )
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

int main(int sdwArgc, char **pcArg)
{
	struct st_SysTask st_TaskInfo;
	int32_t status;
	uint8_t byTaskNum = 0;
	pid_t waitpid;

    if(1 == sdwArgc){
        printf("\nerr: Missing parameter!\n");
        printf("================= [usage] ==================\n");
        printf("example:\n");
        printf("\t%s Main\n", pcArg[0]);
        printf("--------------------------------------------\n");
        return 0;
    }

	memset(&st_TaskInfo, 0, sizeof(st_TaskInfo));
    strcpy(st_TaskInfo.progName, pcArg[0]+2);   // +2偏移掉"./"

    /* 主进程进入此分支 */
	if(0 == strcmp(pcArg[1], "Main"))
	{
    	char processName[128]={0};
    	strcpy(processName, pcArg[0]+2);
        log_manager_init(LOGCONFIG_PATH, processName);
        
        // 2.1 创建RTSP服务器 x 1个
        CreateProcess(PROCESS_RTSPSERVER_NAME, &st_TaskInfo);
        
        // 2.2 创建编码器 x 1个
        CreateProcess(PROCESS_ENCODER_NAME, &st_TaskInfo);
    
    /* RTSP服务器子程进入此分支 */
    } else if(0 == strcmp(pcArg[1], PROCESS_RTSPSERVER_NAME)) {
        rtspServerInit(PROCESS_RTSPSERVER_NAME);
    /* 编码器子程进入此分支 */
    } else if(0 == strcmp(pcArg[1], PROCESS_ENCODER_NAME)) {
        enCoderInit(PROCESS_ENCODER_NAME);
    }

    /* 主进程进入守护状态，一旦子进程崩溃，则重新启动 */
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



