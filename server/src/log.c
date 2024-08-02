#include "head.h"
int initLog(Log *log)
{
    log->mydb.mysql = mysql_init(NULL);
    ERROR_CHECK(log->mydb.mysql, NULL, "mysql_init");
    log->mydb.mysql = mysql_real_connect(log->mydb.mysql, IP, "root", "chily7150", "zm", 3306,NULL, 0);
    ERROR_CHECK(log->mydb.mysql, NULL, "mysql_real_connect");
    return 0;
}

int addLog(Log *log, char *operation, char *username)
{
    // 插入用户记录
    time_t now = time(NULL);
    char *time_s = ctime(&now);

    char sql[256];
    sprintf(sql, "INSERT into Log(`User`, Operation, time) values('%s', '%s', '%s')", username, operation, time_s);
    mysql_query(log->mydb.mysql, sql);
    log->mydb.result = mysql_store_result(log->mydb.mysql);
    if(NULL == log->mydb.result)
    {
        if(mysql_field_count(log->mydb.mysql) == 0)
        {
            unsigned long long num_rows = mysql_affected_rows(log->mydb.mysql);
            return num_rows;
        }
        printf("Get result error: %s\n", mysql_error(log->mydb.mysql));
    }
    return -1;
}

int exitLog(Log *log)
{
    // 退出日志系统， 释放对应资源
    mysql_close(log->mydb.mysql);
    return 0;
}