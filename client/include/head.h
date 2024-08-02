#include <func_zm.h>
#include "uthash.h"

// 网络编程相关函数
#define IP "10.0.8.17"
#define PORT 12345
#define mysql_port 3306

int recvFile(int sockFd, const char *file_name, off_t load_size);
int sendFile(int clientFd, const char *FileName);

int recvCycle(int fd, void *p, size_t len);
int sendCycle(int fd, void *p, size_t len);

// 小火车协议
typedef struct train_s{
    int length;   // 包头，用于记录存数据的大小
    char buf[1000]; // 用于保存数据
} train_t;

// 生成 salt 函数


// 生成 md5 的函数
int getFileMd5(const char *file_name, char *md5_value);

// 定义上传任务和下载任务的任务包
typedef struct task_s
{
    // int parentFd; // 发送 puts 和 gets 任務需要刷新本地的客戶端连接，因此需要将本次连接之前建立的clientFd发送过去
    char username[64];
    int Dir;
    char order[10];
    char option[256];
} task_t;

void *upload(void *arg);
void *download(void *arg);


void handle_sigpipe(int sig);
