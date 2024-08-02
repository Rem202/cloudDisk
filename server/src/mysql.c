#include "head.h"
#include <mysql/mysql.h>

int initDb(MyDb *mydb,char *host, char *user, char *pwd, char *db_name, int port){
    mydb->mysql = mysql_init(NULL);
    ERROR_CHECK(mydb->mysql, NULL, "mysql_init");
    mydb->mysql = mysql_real_connect(mydb->mysql, host, user, pwd, db_name, port, NULL, 0);
    ERROR_CHECK(mydb->mysql, NULL, "mysql_real_connect");
    return 0;
}
// int execSql(MyDb *mydb, char *sql){
//     // 执行 sql 命令，如果是查询语句则打印查询结果， 如果是 其他语句泽返回受影响的行数， 错误返回 -1
//     mysql_query(mydb->mysql, sql);
//     mydb->result = mysql_store_result(mydb->mysql);
//     if(mydb->result){
//         // 获取结果集中的字段数目，即列数
//         int num_fields = mysql_num_fields(mydb->result);
//         unsigned long long num_rows = mysql_num_rows(mydb->result);
//         for(unsigned long long i = 0; i < num_rows; ++i){
//             mydb->row = mysql_fetch_row(mydb->result);
//             if(!mydb->row){
//                 break;
//             }
//             for(int j = 0; j < num_fields; ++j){
//                 printf("%s\t", mydb->row[j]);
//             }
//             puts("");
//         }
//         return num_rows;
//     }
//     else{
//         // 代表执行的是 insert，, udpate, delete 类的查询语句
//         if(mysql_field_count(mydb->mysql) == 0){
//             unsigned long long num_rows = mysql_affected_rows(mydb->mysql);
//             return num_rows;
//         }
//         else{
//             printf("Get result error: %s\n", mysql_error(mydb->mysql));
//             return -1;
//         }
//     }
//     return 0;
// }

int execSql(MyDb *mydb, char *sql) {
    // 执行 SQL 命令
    if (mysql_query(mydb->mysql, sql)) {
        // 如果执行SQL命令失败
        unsigned int err_code = mysql_errno(mydb->mysql);
        if (err_code == ER_ROW_IS_REFERENCED_2 || err_code == ER_NO_REFERENCED_ROW_2) {
            // 外键约束错误
            printf("Foreign key constraint error: %s\n", mysql_error(mydb->mysql));
            return -2;
        } else {
            // 其他错误
            printf("SQL execution error: %s\n", mysql_error(mydb->mysql));
            return -1;
        }
    }

    mydb->result = mysql_store_result(mydb->mysql);
    if (mydb->result) {
        // 获取结果集中的字段数目，即列数
        int num_fields = mysql_num_fields(mydb->result);
        unsigned long long num_rows = mysql_num_rows(mydb->result);
        for (unsigned long long i = 0; i < num_rows; ++i) {
            mydb->row = mysql_fetch_row(mydb->result);
            if (!mydb->row) {
                break;
            }
            // for (int j = 0; j < num_fields; ++j) {
            //     printf("%s\t", mydb->row[j] ? mydb->row[j] : "NULL");
            // }
            // puts("");
        }
        mysql_free_result(mydb->result); // 释放结果集
        return num_rows;
    } else {
        // 代表执行的是 insert，update，delete 类的查询语句
        if (mysql_field_count(mydb->mysql) == 0) {
            unsigned long long num_rows = mysql_affected_rows(mydb->mysql);
            return num_rows;
        } else {
            printf("Get result error: %s\n", mysql_error(mydb->mysql));
            return -1;
        }
    }

    return 0;
}

int execOneSql(MyDb *mydb, const char *sql,char **res){
    // 查找一个属性列
    if(*res !=NULL)
    {
        free(*res);
        *res = NULL;
    }
    mysql_query(mydb->mysql, sql);
    mydb->result = mysql_store_result(mydb->mysql);

    unsigned long long num_rows = mysql_num_rows(mydb->result);
    if(mydb->result){
        // 获取结果集中的字段数目，即列数
        *res = (char **)calloc(1000, sizeof(char));
        ERROR_CHECK(*res, NULL, "calloc");
        mydb->row = mysql_fetch_row(mydb->result);
        if(!mydb->row){
            return -1; // 如果没有该属性列
        }
        strcpy((*res), mydb->row[0]);
        return num_rows;
    }
    return -1;
}

int execSqls(MyDb *mydb, const char *sql,char ***res, int row){
    // 返回行数
    if(NULL != (*res)){
        for(int i = 0; i < row; ++i){
            free((*res)[i]);
        }
        free(*res);
    }
    *res = NULL;
    mysql_query(mydb->mysql, sql);
    mydb->result = mysql_store_result(mydb->mysql);
    if(mydb->result){
        // 获取结果集中的字段数目，即列数
        int num_fields = mysql_num_fields(mydb->result);
        unsigned long long num_rows = mysql_num_rows(mydb->result);
        *res = (char **)calloc(num_rows, sizeof(char *));
        ERROR_CHECK(*res, NULL, "calloc");
        for(int i = 0; i < num_rows; ++i){
            (*res)[i] = (char *)calloc(1024, sizeof(char));
            ERROR_CHECK((*res)[i], NULL, "calloc");
        }
        for(unsigned long long i = 0; i < num_rows; ++i){
            mydb->row = mysql_fetch_row(mydb->result);
            if(!mydb->row){
                break;
            }
            for(int j = 0; j < num_fields; ++j){
                strcat((*res)[i], mydb->row[j]);
                strcat((*res)[i], " ");
            }
        }
        return num_rows;
    }
    return NULL;
}

void execPath(MyDb *mydb, const char *sql, char *result, unsigned long *length) {


    if (mysql_query(mydb->mysql, sql)) {
        fprintf(stderr, "SELECT error: %s\n", mysql_error(mydb->mysql));
        exit(EXIT_FAILURE);
    }

    mydb->result = mysql_store_result(mydb->mysql);
    if (mydb->result == NULL) {
        fprintf(stderr, "mysql_store_result() failed\n");
        exit(EXIT_FAILURE);
    }

    if ((mydb->row = mysql_fetch_row(mydb->result)) != NULL) {
        strcpy(result, mydb->row[0]);
        *length = strlen(result);
    }

    mysql_free_result(mydb->result);
}

int exitDb(MyDb *mydb){
    mysql_close(mydb->mysql);
    return 0;
}