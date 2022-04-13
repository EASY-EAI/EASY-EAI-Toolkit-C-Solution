#ifndef __DATABASE_H__
#define __DATABASE_H__

#include <stdbool.h>
#include <stdint.h>

/*
 * 本解决方案仅使用，一个数据库，一张表
 *
 * 一个数据库可以有多张表，结构如下：
 *
 *     xxxx.db           ---> 数据库文件
 *      ┣━ xxx1_table    ---> 表1
 *      ┣━ xxx1_table    ---> 表2
 *      ┣━ ……
 *      ┗━ xxxn_table    ---> 表n
 */
#define DATABASE_PATH "./face_data.db"
#define FACE_TABLE    "face_table"


/* 设备可容纳的总人数 */
#define MAX_USER_NUM 10000
/* 算法输出的特征值大小 */
#define FEATURE_SIZE 2048
/* ID长度 */
#define ID_LEN 128
/* ID对应的姓名最大长度 */
#define NAME_LEN 256
/*
 * 该结构体用于同步数据库信息到内存上，
 * 避免特征比对时频繁操作数据库，以提升比对性能。
 * 注意：此结构体不能作任何改动(比如增加成员变量等)
 */
typedef struct {
    char feature[FEATURE_SIZE];
    char id[ID_LEN];
}faceData_t;


/* 
 * 数据库基本操作：
 *     1.初始化
 *     2.退出
 *     3.重置数据库(不删除数据)
 */
extern int  database_init(void);
extern void database_exit(void);
extern void database_reset(void);

// 读出数据库记录总条数数量
extern int database_record_count(void);
// 凭Id，删除数据库一条记录
extern int database_delete_record(char *id);
// 凭Id，向数据库插入一条记录(若此id已存在于数据库，则会被覆盖)
extern int database_add_record(char * id, char *name, void *data, size_t d_size);
// 把整个数据库同步到内存中来，存放于结构体 “faceData_t” 中
extern int database_getData_to_memory(void *pData);
// 删除数据库所有记录
extern int database_delete_all_record(void);


// 判断此名字是否存在于数据库
extern bool database_name_is_exist(const char *name);
// 判断此 Id 是否存在于数据库
extern bool database_id_is_exist(const char *id, char *name, size_t size);

#endif

