#include <func_zm.h>
#include "head.h"


int sendFile(int clientFd, const char *FileName){
    // 用于给客户端发送文件
    int ret;
    train_t train;
    // 打开本地文件
    int file_fd = open(FileName, O_RDWR);
    ERROR_CHECK(file_fd, -1, "open");

    // 传送文件名称
    train.length = strlen(FileName);
    strcpy(train.buf, FileName);
    sendCycle(clientFd, &train, sizeof(train.length) + train.length);

    // 傳送文件大小
    struct stat statbuf;
    ret = stat(FileName, &statbuf);
    ERROR_CHECK(ret, -1, "stat");
    train.length = sizeof(off_t);
    memcpy(train.buf, &statbuf.st_size, train.length);
    sendCycle(clientFd, &train, sizeof(train.length) + train.length);

    // 傳送文件本身
    char *pmap = (char *)mmap(NULL, statbuf.st_size, PROT_READ, MAP_SHARED, file_fd, 0);
    ERROR_CHECK(pmap, (char *)-1, "mmap");
    off_t total = 0, last_size = 0;  // 总接受数据， 上一次接受 1% 的数据的大小
    off_t slice = statbuf.st_size / 100;
    while(total < statbuf.st_size){
        if(statbuf.st_size - total < sizeof(train.buf)){
            // 当传送内容不足一个小火车的容量时 即最后一车
            train.length = statbuf.st_size - total;
        }else{
            train.length = sizeof(train.buf);
        }
        memcpy(train.buf, pmap+total, train.length);
        total += train.length;
        if(total - last_size >= slice){
            last_size = total;
            printf("\x1b[33m\r%5.2f%%\x1b[0m", (float)total/statbuf.st_size * 100);
            fflush(stdout);
        }
        sendCycle(clientFd, &train, train.length + sizeof(train.length));
    }
    printf("\x1b[33m\r100.00%%\x1b[0m\n"); 
    ret = munmap(pmap, statbuf.st_size);
    ERROR_CHECK(ret, -1, "munmap");
    // 发送一个空的火车，表示传送结束
    train.length = 0;
    sendCycle(clientFd, &train, sizeof(train.length));
    close(file_fd);
    return 0;
}

// 由于send 和 recv 每一次不一定能将所有的数据发送完毕，
int recvCycle(int fd, void *p, size_t len)
{
    signal(SIGPIPE, handle_sigpipe);
    char *pStart = (char *)p;
    size_t size = 0;
    while (size < len)
    {
        int ret = recv(fd, pStart + size, len - size, 0);
        if (0 == ret)
        {
            return 0;
        }
        if (-1 == ret)
            return -1;
        size += ret;
    }
    return size;
}

int sendCycle(int fd, void *p, size_t len)
{
    signal(SIGPIPE, handle_sigpipe);
    char *pStart = (char *)p;
    size_t size = 0;
    while (size < len)
    {
        int ret = send(fd, pStart + size, len - size, 0);
        ERROR_CHECK(ret, -1, "send");
        size += ret;
    }
    return size;
}
