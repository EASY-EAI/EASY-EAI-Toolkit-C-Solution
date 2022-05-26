#ifndef __IPCDATA_H__
#define __IPCDATA_H__


/* 端口分配：
 *     命令：netstat -tunl     (查看端口占用情况)
 * 端口说明：
 *     [30000]：IPC服务器端口
 */
#define IPC_SERVER_PORT   30000
#define ANALYZER_CLI_ID (IPC_SERVER_PORT+1)
#define ADAPTER_CLI_ID  (IPC_SERVER_PORT+2)

enum MsgType{
    MSGTYPE_NULL = 0,
    MSGTYPE_PERSON_DATA,
    MSGTYPE_HELMET_DATA,
    MSGTYPE_BANAREA_DATA,
    MSGTYPE_NUM
};



#endif //__IPCDATA_H__

