#ifndef HEAD_H
#define HEAD_H

#include <func_zm.h>
#include "uthash.h"

#define IP "10.0.8.17"
#define workdirectory "/home/mingrem/cDisk_Store/" // 所有文件放置的目录
#define PORT 12345


// 网络编程相关函数
int tcpInit(int *pSockFd, char *ip, int port);
int recvFile(int sockFd);
int sendFile(int clientFd, const char *FileName);

int recvCycle(int fd, void *p, size_t len);
int sendCycle(int fd, void *p, size_t len);

// 小火车协议
typedef struct train_s
{
    int length;     // 包头，用于记录存数据的大小
    char buf[1024]; // 用于保存数据
} train_t;

// 定义客户端链表
typedef struct clientNode
{
    int netFd; // 保存對應的netFd
    struct clientNode *next;
} clientNode;

clientNode *clientAdd(clientNode *head, int clientFd);
clientNode *clientDel(clientNode *head, clientNode *delnode);
// clientNode *clientDel(clientNode *head, int clientFd);
clientNode *freeClient(clientNode *head); // 清空链表

// 定义mysql 相关借口
typedef struct
{
    MYSQL *mysql;
    MYSQL_RES *result;
    MYSQL_ROW row;
} MyDb;
#define ER_ROW_IS_REFERENCED_2 1451
#define ER_NO_REFERENCED_ROW_2 1452

int initDb(MyDb *mydb, char *host, char *user, char *pwd, char *db_name, int port);
int execSql(MyDb *mydb, char *sql);
int execOneSql(MyDb *mydb, const char *sql, char **res);
int execSqls(MyDb *mydb, const char *sql, char ***res, int row);
void execPath(MyDb *mydb, const char *sql, char *result, unsigned long *length);
int exitDb(MyDb *mydb);
// int selectOneSql

// 定义一个产生随机salt 的函数
int gennerateStr(char **res, int str_len);

typedef struct task_s
{
    int clientFd; // 客户端的fd
    char username[64];
    char file[256];
    int Dir;
} task_t; // 用于文件上传和下载时的 task 任务fd

void *puts_task(void *arg); // 定义对客户端发送过来的 puts 和 gets 命令相关的命令的任务处理函数
void *gets_task(void *arg);

// 定时器相关数据结构和接口
# define time_out 30000
typedef struct q_timer
{
    /* 1）30s超时，就创建一个index从0到30的环形队列（本质是个数组）

       2）环上每一个slot是一个 clientFd 的链表，任务集合

       3）同时还有一个Map<clientFd, index>，记录clientFd落在环上的哪个slot里

    规则：   
       1）启动一个timer，每隔1s，在上述环形队列中移动一格，0->1->2->3…->29->30->0…

       2）有一个Current Index指针来标识刚检测过的slot

    当有某用户uid有请求包到达时：

       1）从Map结构中，查找出这个clientFd存储在哪一个slot里

       2）从这个slot的链表中，删除这个 clientFd

       3）将clientFd重新加入到新的slot中，具体是哪一个slot呢 => Current Index指针所指向的上一个slot，因为这个slot，会被timer在30s之后扫描到

       4）更新Map，这个uid对应slot的index值

    哪些元素会被超时掉呢？
            Current Index每秒种移动一个slot，这个slot对应的链表中所有client都应该被集体超时！如果最近30s有请求包来到，一定被放到Current 

        Index的前一个slot了，Current Index所在的slot对应链表中所有元素，都是最近30s没有请求包来到的。所以，当没有超时时，Current Index

        扫到的每一个slot的Set中应该都没有元素。
    */
    clientNode *queue[time_out + 1]; // 0 ~ timer_out
    int current;           // 当前指针位置

} queue_timer;
int timerAdd(queue_timer *q, int epfd, int clientFd);  // 当有新的客户连接上后，加入到定时器中
int timerUpdate(queue_timer *q, int epfd);   // 每秒定时维护
int timerDel(queue_timer *q, int epfd, int clientFd);

// 定义一个 hash_map
typedef struct
{
    int netFd;
    int pos;
    UT_hash_handle hh;
} User;
extern User *users;
void mapAdd(int netFd, int pos);
User *mapFind(int netFd);
void mapDel(User *user);
void mapDelByPos(int pos);



// 定义日志系统相关数据机构和接口
typedef struct{
    MyDb mydb;
} Log;

int initLog(Log *log);
int addLog(Log *log, char *operation, char *username);
int exitLog(Log *log);

#endif
