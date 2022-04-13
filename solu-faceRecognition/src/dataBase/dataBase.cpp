#include "system.h"
#include <sqlite3.h>

#include "dataBase.h"

static sqlite3 *g_db = NULL;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ==========================================================================
 * 初始化数据库
 *     1.打开数据库文件
 */
int database_init(void)
{
    char *err;
    char cmd[256];

    if (sqlite3_open(DATABASE_PATH, &g_db) != SQLITE_OK) {
        printf("%s open database %s failed!\n", __func__, DATABASE_PATH);
        return -1;
    }
    snprintf(cmd, sizeof(cmd),
             "CREATE TABLE IF NOT EXISTS %s (data blob, name varchar(%d), id varchar(%d))",
             FACE_TABLE, NAME_LEN, ID_LEN);

    if (sqlite3_exec(g_db, cmd, 0, 0, &err) != SQLITE_OK) {
        sqlite3_close(g_db);
        g_db = NULL;
        printf("%s create table %s failed!\n", __func__, FACE_TABLE);
        return -1;
    }
	printf("database_init OK\n");

    return 0;
}

/* ==========================================================================
 * 退出数据库
 *     1.关闭数据库文件
 */
void database_exit(void)
{
    sqlite3_close(g_db);
    g_db = NULL;    
}


/* ==========================================================================
 * 重置数据库
 *     1.关闭数据库文件
 *     2.打开数据库文件
 */
void database_reset(void)
{
    pthread_mutex_lock(&g_mutex);
    database_exit();
    unlink(DATABASE_PATH);
    database_init();
    pthread_mutex_unlock(&g_mutex);
}

/* ==========================================================================
 * 读出记录数量
 *     【返回】：记录条目数量(若为-1,则为调用异常)
 */
int database_record_count(void)
{
    int ret = 0;
    char cmd[256];
    sqlite3_stmt *stat = NULL;

    if(NULL == g_db) {
        printf("database is unInit !!!\n");
        return -1;
    }

    pthread_mutex_lock(&g_mutex);
    snprintf(cmd, sizeof(cmd), "SELECT COUNT(*) FROM %s;", FACE_TABLE);

    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }
    if (sqlite3_step(stat) == SQLITE_ROW)
        ret = sqlite3_column_int(stat, 0);
    sqlite3_finalize(stat);
    pthread_mutex_unlock(&g_mutex);

    return ret;
}

/* ==========================================================================
 * 凭“id”删除与“id”所匹配的记录
 *     【参数】id：目标id
 *     【参数】sync_flag：是否立即同步到文件系统(1-立即同步；0-系统绝对)
 *      如果不是立即同步，有可能导致本次操作在上电重启后丢失
 */
static void database_delete(const char *id, bool sync_flag)
{
    char cmd[256];

    pthread_mutex_lock(&g_mutex);
    snprintf(cmd, sizeof(cmd), "DELETE FROM %s WHERE id = '%s';", FACE_TABLE, id);
    printf("%s\n", cmd);

    sqlite3_exec(g_db, "begin transaction", NULL, NULL, NULL);
    sqlite3_exec(g_db, cmd, NULL, NULL, NULL);
    sqlite3_exec(g_db, "commit transaction", NULL, NULL, NULL);
    if (sync_flag)
        sync();
    pthread_mutex_unlock(&g_mutex);
}
int database_delete_record(char *id)
{
    if(NULL == g_db) {
        printf("database is unInit !!!\n");
        return -1;
    }
    
    database_delete((const char *)id, 1);

    return 0;
}

/* ==========================================================================
 * 插入一条新记录
 *     【参数】*id         : 目标id
 *     【参数】id_size : “id”字符串的长度
 *     【参数】*name       : 目标名字
 *     【参数】n_size      : “名字”字符串的长度
 *     【参数】*data       : 目标数据
 *     【参数】d_size      : “数据”长度(注意:不一定是字符串)
 *     【参数】sync_flag : 是否立即同步到文件系统(1-立即同步；0-系统绝对)
 *      如果不是立即同步，有可能导致本次操作在上电重启后丢失
 */
static int database_insert(const char *id, size_t id_size, const char *name, size_t n_size, void *data, size_t d_size, bool sync_flag)
{
    char cmd[256];
    sqlite3_stmt *stat = NULL;

    if (id_size > ID_LEN) {
        printf("%s id_size error\n", __func__);
        return -1;
    }
    if (n_size > NAME_LEN) {
        printf("%s n_size error\n", __func__);
        return -1;
    }
    
    pthread_mutex_lock(&g_mutex);
    snprintf(cmd, sizeof(cmd), "REPLACE INTO %s VALUES(?, '%s', '%s');", FACE_TABLE, name, id);

    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    sqlite3_exec(g_db, "begin transaction", NULL, NULL, NULL);
    sqlite3_bind_blob(stat, 1, data, d_size, NULL);
    sqlite3_step(stat);
    sqlite3_finalize(stat);
    sqlite3_exec(g_db, "commit transaction", NULL, NULL, NULL);
    if (sync_flag)
        sync();
    pthread_mutex_unlock(&g_mutex);

    return 0;
}
int database_add_record(char * id, char *name, void *data, size_t d_size)
{
    if(NULL == g_db) {
        printf("database is unInit !!!\n");
        return -1;
    }
    if(strlen(id) > ID_LEN){
        printf("id length is too long !!!\n");
        return -1;
    }
    if(strlen(name) > NAME_LEN){
        printf("name length is too long !!!\n");
        return -1;
    }

    // 若Id已存在，先删除，先不同步
    database_delete((const char *)id, 0);

    // 插入记录，并同步
    return database_insert((const char *)id, strlen(id), (const char *)name, strlen(name), data, d_size, 1);
}

/* ==========================================================================
 * 提取所有记录
 *     【参数】*dst             : 用于缓存的记录的内存地址
 *     【参数】cnt              : 记录条数总数量
 *     【参数】feature_size     : 特征值数据所占用的内存大小(Byte)
 *     【参数】feature_off      : 特征值数据相对于 *dst 内存位置的偏移(为0)
 *     【参数】id_size          : id字符串所占用的内存大小(Byte)
 *     【参数】id_off           : id字符串相对于 *dst 内存位置的偏移(一个feature_size的长度)
 */
static int database_get_data(void *dst, const int cnt, size_t feature_size, size_t feature_off, size_t id_size, size_t id_off)
{
    int ret = 0;
    char cmd[256];
    sqlite3_stmt *stat = NULL;
    int index = 0;
    const void *data;
    size_t size;
    const size_t sum_size = feature_size + id_size;

    if( cnt ==0)
    {
        return -1;
    }

    pthread_mutex_lock(&g_mutex);
    snprintf(cmd, sizeof(cmd), "SELECT * FROM %s;", FACE_TABLE);

    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }
    while (1) {
        ret = sqlite3_step(stat);
        if (ret != SQLITE_ROW)
            break;
        data = sqlite3_column_blob(stat, 0);
        size = sqlite3_column_bytes(stat, 0);
        if (size <= feature_size)
            memcpy((char*)dst + index * sum_size + feature_off, data, size);
        //id = sqlite3_column_int(stat, 2);

        //memcpy((char*)dst + index * sum_size + id_off, &id, id_size);

        const unsigned char *n = sqlite3_column_text(stat, 2);
        size_t s = sqlite3_column_bytes(stat, 2);
        if (s <= 128)
            memcpy((char*)dst + index * sum_size + id_off, (const char *)n, s);
            //strncpy(name, (const char *)n, size - 1);

        if (++index >= cnt)
            break;
    }
    sqlite3_finalize(stat);
    pthread_mutex_unlock(&g_mutex);

    return index;
}
/* ==========================================================================
 * 提取所有记录
 *     【参数】*pData : 用于缓存的记录的内存地址
 *     【返回】       : 记录条目数量，即人数(若为-1,则为调用异常)
 */
int database_getData_to_memory(void *pData)
{
	int ret = -1;
    if(NULL == pData) {
        return ret;
    }
    if(NULL == g_db) {
        printf("database is unInit !!!\n");
        return ret;
    }
    // 读出数据库人数
    int peopleNum = database_record_count();
	if(0 == peopleNum){
		return 0;
	}
    memset(pData, 0, peopleNum * sizeof(faceData_t));

    // 提取到内存中
	faceData_t face = {0};
    ret = database_get_data(pData, peopleNum, sizeof(face.feature), 0, sizeof(face.id), sizeof(face.feature));
	if(ret < 0)
		return ret;
	
	return peopleNum;
}

/* ==========================================================================
 * 删除所有记录
 */
int database_delete_all_record(void)
{
    // 关闭数据库文件
    database_exit();
    if(NULL != g_db){
        printf("database close error\n");
        return -1;
    }

    // 删除整个数据库
    char sys_cmd[256] = {0};
    sprintf(sys_cmd,"rm %s", DATABASE_PATH);
    system(sys_cmd);

    // 重新打开数据库
    database_init();
    if(NULL == g_db){
        printf("database open error\n");
        return -1;
    }
    return 0;
}

/* ==========================================================================
 * 查询这个“名字”是否在数据库中
 *
 *     【返回】true : “名字”在数据库中
 *     【返回】false: “名字”不在数据库中
 */
bool database_name_is_exist(const char *name)
{
    bool exist = false;
    int ret = 0;
    char cmd[256];
    sqlite3_stmt *stat = NULL;

    pthread_mutex_lock(&g_mutex);
    snprintf(cmd, sizeof(cmd), "SELECT * FROM %s WHERE name = '%s' LIMIT 1;", FACE_TABLE, name);

    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return false;
    }
    ret = sqlite3_step(stat);
    if (ret == SQLITE_ROW) //表示查询的数据，以多行的形式显示出来
        exist = true;
    else
        exist = false;
    sqlite3_finalize(stat);
    pthread_mutex_unlock(&g_mutex);

    return exist;
}

/* ==========================================================================
 * 查询这个“id”是否在数据库中，并且把“id”对应的“名字”输出到“name”所指的内存中
 *
 *     【返回】true : “名字”在数据库中
 *     【返回】false: “名字”不在数据库中
 */
bool database_id_is_exist(const char *id, char *name, size_t size)
{
    bool exist = false;
    int ret = 0;
    char cmd[256];
    sqlite3_stmt *stat = NULL;

    memset(name, 0, size);
    snprintf(cmd, sizeof(cmd), "SELECT * FROM %s WHERE id = '%s' LIMIT 1;", FACE_TABLE, id);

    pthread_mutex_lock(&g_mutex);
    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return false;
    }
    ret = sqlite3_step(stat);
    if (ret == SQLITE_ROW) {
        const unsigned char *n = sqlite3_column_text(stat, 1);
        size_t s = sqlite3_column_bytes(stat, 1);
        if (s < size)
            strncpy(name, (const char *)n, s);
        else
            strncpy(name, (const char *)n, size - 1);
        exist = true;
    } else {
        exist = false;
    }
    sqlite3_finalize(stat);
    pthread_mutex_unlock(&g_mutex);

    return exist;
}


/* ==========================================================================
 * 获取未被使用ID？
 *
 *     返回: 未知意义
 */
int database_get_user_name_id(void)
{
    int ret = 0;
    char cmd[256];
    sqlite3_stmt *stat = NULL;
    int id = 0;
    int max_id = -1;
    int *save_id = NULL;
    int ret_id = 0;

    pthread_mutex_lock(&g_mutex);

    /* 提取数据库的表内的行数量 */
    snprintf(cmd, sizeof(cmd), "SELECT * FROM %s where id=(select max(id) from %s);", FACE_TABLE, FACE_TABLE);
    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    while (1) {
        ret = sqlite3_step(stat);
        if (ret != SQLITE_ROW)
            break;
        max_id = sqlite3_column_int(stat, 2);
    }
    sqlite3_finalize(stat);

    if (max_id < 0) {
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }
    save_id = (int*)calloc(max_id + 1, sizeof(int));
    if (!save_id) {
        printf("%s: memory alloc fail!\n", __func__);
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }

    snprintf(cmd, sizeof(cmd), "SELECT * FROM %s;", FACE_TABLE);
    if (sqlite3_prepare(g_db, cmd, -1, &stat, 0) != SQLITE_OK) {
        ret_id = -1;
        goto exit;
    }
    while (1) {
        ret = sqlite3_step(stat);
        if (ret != SQLITE_ROW)
            break;
        id = sqlite3_column_int(stat, 2);
        save_id[id] = 1;
    }
    sqlite3_finalize(stat);

    for (int i = 0; i < max_id + 1; i++) {
        if (!save_id[i]) {
            ret_id = i;
            goto exit;
        }
    }
    ret_id = max_id + 1;

exit:
    if (save_id)
        free(save_id);
    pthread_mutex_unlock(&g_mutex);
    return ret_id;
}

