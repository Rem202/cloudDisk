#include "../include/head.h"
#include <func_zm.h>


// int recvFile(int sockFd, const char *file_name, off_t load_size);

void *upload(void *arg)
{
    // puts("upload!");
    task_t *t = (task_t *)arg;
    
    // 子线程重新连接开启新的描述符
    int sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockFd == -1)
    {
        perror("sockFd");
        pthread_exit(NULL);
    }
    struct sockaddr_in sockinfo;
    bzero(&sockinfo, sizeof(sockinfo));
    sockinfo.sin_addr.s_addr = inet_addr(IP);
    sockinfo.sin_port = htons(PORT);
    sockinfo.sin_family = AF_INET;

    int ret = connect(sockFd, (struct sockaddr *)&sockinfo, sizeof(sockinfo));
    if(ret == -1)
    {
        perror("connect");
        pthread_exit(NULL);
    }

    // 将指令 和 用户名拼接起 传送到服务器
    train_t train;
    sprintf(train.buf, "%s %s %s", t->order, t->username, t->option);
    train.length = strlen(train.buf);
    sendCycle(sockFd, &train, train.length + 4);

    // 发送目录号
    sendCycle(sockFd, &t->Dir, 4);

    // 先将parentFd 发送过去
    // sendCycle(sockFd, &t->parentFd, 4);

    char md5[33];
    
    ret = getFileMd5(t->option, md5);

    if(ret == -1)
    {
        perror("md5");
        pthread_exit(NULL);
    }

    // 传送 md5 过去
    bzero(&train, sizeof(train));
    strcpy(train.buf, md5);
    train.length = strlen(train.buf);
    sendCycle(sockFd, &train, 4 + train.length);

    // 接收查询结果
    recvCycle(sockFd, &ret, 4);
    if(ret == 1)
    {
        puts("file already exist!");
    }
    else
    {
        recvCycle(sockFd, &ret, 4);
        if(ret == 1)
        {
            puts(" \x1b[33m100%秒传成功！\x1b[0m");
        }
        else
        {
            // 
            sendFile(sockFd, t->option);
        }
    }
    close(sockFd);
    pthread_exit(NULL);
}

struct {
    int fd;
    off_t loadsize;
} loadPackt;  // 定义一个临时变量

void hanlder(int signum)
{
    if(loadPackt.loadsize !=0)
    {
        puts("\n\033[31m Stop download!  \033[0m");
        ftruncate(loadPackt.fd, loadPackt.loadsize); //将文件截断成对应大小
        printf("  已下载: \033[31m %d \033[0mByte\n", loadPackt.loadsize);
        exit(-1);
    }
}

void *download(void *arg)
{
    task_t *t = (task_t *)arg;

    int sockFd = socket(AF_INET, SOCK_STREAM, 0);
     if(sockFd == -1)
    {
        perror("sockFd");
        pthread_exit(NULL);
    }
    struct sockaddr_in sockinfo;
    sockinfo.sin_addr.s_addr = inet_addr(IP);
    sockinfo.sin_port = htons(PORT);
    sockinfo.sin_family = AF_INET;

    int ret = connect(sockFd, (struct sockaddr *)&sockinfo, sizeof(sockinfo));
    if(ret == -1)
    {
        perror("connect");
        pthread_exit(NULL);
    }

    // 将指令 和 用户名拼接起 传送到服务器
    // printf("username: %s, order: %s, file_name: %s \n", t->username, t->order, t->option);
    train_t train;
    sprintf(train.buf, "%s %s %s", t->order, t->username, t->option);
    train.length = strlen(train.buf);
    sendCycle(sockFd, &train, train.length + 4);

    // 发送 Dir
    sendCycle(sockFd, &t->Dir, 4);

    // 发送parentFd
    // sendCycle(sockFd, &t->parentFd, 4);
    // 接收查询结果，查看服务器是否含有该文件
    recvCycle(sockFd, &ret, 4);
    // printf("ret = %ld\n", ret);
    if(1 == ret)
    {
        // 判断是否已经下载过该文件，从上次中断过后继续下载
        // 读取本地的文件，查看其大小，并发送给服务端
        struct stat file_stat;
        stat(t->option, &file_stat);
        sendCycle(sockFd, &file_stat.st_size, sizeof(file_stat.st_size));
        // 读取远程文件总大小
        recvFile(sockFd, t->option, file_stat.st_size);
        
    }
    else
    {
        // 服务器上不存在该文件
        puts("\033[31m Server don't have this file! \033[0m");
    }
    close(sockFd);

}


int recvFile(int sockFd, const char *file_name, off_t load_size) {
    bzero(&loadPackt, sizeof(loadPackt));
    signal(SIGINT, hanlder);  // 确保信号处理函数名称正确
    int ret, length;
    char buf[1000] = {0};

    // 接收文件名称
    int fileId = open(file_name, O_RDWR | O_CREAT, 0666);
    ERROR_CHECK(fileId, -1, "open");
    loadPackt.fd = fileId;
    off_t size;

    // 接收文件大小
    recvCycle(sockFd, &size, sizeof(size));

    ftruncate(fileId, size);

    // 接收文件内容
    char *pmap = (char *)mmap(NULL, size, PROT_WRITE, MAP_SHARED, fileId, 0);
    ERROR_CHECK(pmap, (char *)-1, "mmap");


    

    off_t download = load_size, last_size = load_size;
    off_t slice = size / 100;
    while (1) {
        recvCycle(sockFd, &length, 4);
        // printf("recv %d byte data \n", length);
        if (0 == length) {
            printf("\x1b[33m\r100.00%%\x1b[0m\n"); // 发送文件最后会发送一个0作为结束标志。
            fflush(stdout);
            bzero(&loadPackt, sizeof(loadPackt));
            break;
        } else {
            ret = recvCycle(sockFd, pmap + download, length);
            if (0 == ret) {
                break; // 服务器已经断开
            }
            download += length;
            loadPackt.loadsize = download;
            if (download - last_size >= slice || download == size) {
                printf("\x1b[33m\r%5.2f%%\x1b[0m", (float)download / size * 100);
                fflush(stdout);
                last_size = download;
            }
        }
    }
    ret = munmap(pmap, size);
    ERROR_CHECK(ret, -1, "munmap");
    close(fileId);
    return 0;
}


// int recvFile(int sockFd, const char *file_name, off_t load_size)
// {
//     bzero(&loadPackt, sizeof(loadPackt));
//     signal(SIGINT, hanlder);
//     int ret, length;
//     char buf[1000] = {0};

//     // 接收文件名称
//     int fileId = open(file_name, O_RDWR|O_CREAT, 0666);
//     ERROR_CHECK(fileId, -1, "open");
//     loadPackt.fd = fileId;
//     off_t size;


//     // 接收文件大小
//     recvCycle(sockFd, &size, sizeof(size));
//     // printf("file size = %ld \n", size);

//     ftruncate(fileId, size);

//     // 接收文件内容
//     char *pmap = (char *)mmap(NULL, size, PROT_WRITE, MAP_SHARED, fileId, 0);
//     ERROR_CHECK(pmap, (char *)-1,"mmap");

//     off_t download = load_size, last_size = load_size;
//     off_t slice = size / 100;
//     while(1)
//     {
//         recvCycle(sockFd, &length, 4);
//         // printf("recv %d byte data \n", length);
//         if(0 == length)
//         {
//             printf("\x1b[33m\r100.00%%\x1b[0m\n"); // 发送文件最后会发送一个0作为结束标志。
//             break;
//         }
//         else
//         {
//             ret = recvCycle(sockFd, pmap + download, length);
//             if(0 == ret)
//             {
//                 break;  // 服务器已经断开
//             }
//             download += length;
//             loadPackt.loadsize = download;
//             if(download - last_size >= slice)
//             {

//                 printf("\x1b[33m\r%5.2f%%\x1b[0m", (float)download / size * 100);
//                 fflush(stdout);
//                 last_size = download;
//             }
//             sleep(1);
//         }
//     }
//     ret = munmap(pmap, size);
//     ERROR_CHECK(ret, -1, "munmap");
//     close(fileId);
//     return 0;
// }