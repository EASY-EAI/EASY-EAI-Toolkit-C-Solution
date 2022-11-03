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

#include "system.h"

#include "rtspServer.h"

int rtspServerInit(const char *moduleName)
{
    create_rtsp_Server(554);

    // 正常情况下不会走到这里
    PRINT_ERROR("Fatal Error! RtspServer loop exited!");

    return -1;
}

