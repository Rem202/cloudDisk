// 定义对客户端发送过来的 puts 和 gets 命令相关的命令的任务处理函数
#include "head.h"
void *puts_task(void *arg)
{
    MyDb mydb;
    initDb(&mydb, IP, "root", "chily7150", "zm", 3306);
    char *one = NULL;
    // 初始化数据库
    int length;
    char buf[1024];
    off_t size; // 文件大小
    task_t *pt = (task_t *)arg;
    // recv md5 
    recvCycle(pt->clientFd, &length, 4);
    char md5[33];
    recvCycle(pt->clientFd, md5, length);
    // 查询是否已经存在该文件
    char sql[256];
    sprintf(sql, "SELECT * from FileTable WHERE Dir = %d and user = '%s' and FileType = 'f' and FileName = '%s'", pt->Dir, pt->username, pt->file);
    int ret = execSql(&mydb, sql);
    // 将查询结果发送给 客户端
    sendCycle(pt->clientFd, &ret, 4);
    if(ret == 0)
    {
        // 文件并不存在，查找虚拟目录下是否存在该文件
        bzero(sql, sizeof(sql));
        sprintf(sql, "SELECT * from FileTable WHERE MD5 = '%s'", md5);
        ret = execSql(&mydb, sql);
        sendCycle(pt->clientFd, &ret, 4);

        if(ret == 0)
        {
            // 创建文件并收到文件, 由于使用的是虚拟目录，所有文件放在一个地方，因此创建文件时，使用文件的 md5 名字来创建文件
            // 保存一下当前的工作路径
            char cwd[256];
            getcwd(cwd, sizeof(cwd));
            // 切换到文件存储系统路径
            chdir(workdirectory);
            int file_fd = open(md5, O_RDWR|O_CREAT, 0666);
            ERROR_CHECK(file_fd, -1, "open");
            chdir(cwd); // 切换回原工作目录

            // 首先接受文件名称
            bzero(buf, sizeof(buf));
            recvCycle(pt->clientFd, &length, 4);
            recvCycle(pt->clientFd, buf, length);

            // 然后接受文件的大小
            recvCycle(pt->clientFd, &length, 4);
            recvCycle(pt->clientFd, &size, length);
            // printf("file_name: %ld\n", size);

            ftruncate(file_fd, size);
            // 开始接受文件的内容
            bzero(buf, sizeof(buf));
            char *pmap = (char *)mmap(NULL, size, PROT_WRITE, MAP_SHARED, file_fd, 0);
            ERROR_CHECK(pmap, (char *)-1, "mmap");
            off_t download = 0, last_size  = 0;
            off_t slice = size / 100;

            while (1)
            {
                // 客户端是使用小火车协议一段一段的发送过来
                recvCycle(pt->clientFd, &length, 4);
                if(0 == length)
                {
                    printf("\r100.00%%\n");
                    break;
                }
                else
                {
                    ret = recvCycle(pt->clientFd, pmap + download, length);
                    if(0 == ret)
                    {
                        break;
                    }
                    download += length;
                    if(download - last_size >= slice)
                    {
                        printf("\r%5.2f%%", (float)download / size * 100);
                        fflush(stdout);
                        last_size = download;
                    }
                }

            }
            ret = munmap(pmap, size);
            ERROR_CHECK(ret, -1, "munmap");
            close(file_fd);
        }
        // 插入文件到虚拟目录中去
        bzero(sql, sizeof(sql));
        sprintf(sql, "INSERT into FileTable(Dir, FileName, FileSize, FileType, MD5, `User`) VALUES(%d, '%s', %ld, 'f', '%s', '%s')", pt->Dir, pt->file, size, md5, pt->username);
        puts(sql);
        ret = execSql(&mydb, sql);
        if(0 == ret){
            puts("upload success!");
        }
    }
    close(pt->clientFd);
    exitDb(&mydb);
}


void *gets_task(void *arg)
{
    MyDb mydb;
    initDb(&mydb, IP, "root", "chily7150", "zm", 3306);
    char *one = NULL;
    // 初始化数据库
    int length;
    task_t *pt = (task_t *)arg;
    // 

    char sql[256];
    sprintf(sql, "select MD5 from FileTable where User = '%s' and Dir = %d and FileName = '%s' and FileType = 'f'", pt->username, pt->Dir, pt->file);
    puts(sql);
    int ret = execOneSql(&mydb, sql, &one);
    printf("ret = %ld\n", ret);
    sendCycle(pt->clientFd, &ret, 4);

    if(1 == ret)
    {
        // printf("md5:%s \n", one);
        // 打开对应的文件
        char cwd[64];
        getcwd(cwd, sizeof(cwd));
        chdir(workdirectory);
        // 首先 获取客户端文件 现有大小
        off_t beg_pos;
        recvCycle(pt->clientFd, &beg_pos, sizeof(beg_pos));

        struct stat file_stat;
        stat(one, &file_stat);
        // printf("file size = %ld\n", file_stat.st_size);
        off_t file_size = file_stat.st_size;
        

        sendCycle(pt->clientFd, &file_size, sizeof(file_size));

        int fileId = open(one, O_RDONLY);
        char *pmmap = (char *)mmap(NULL, file_stat.st_size, PROT_READ, MAP_SHARED, fileId, 0);
        ERROR_CHECK(pmmap, (char *)-1, "mmap");

        off_t offset = beg_pos, last_size = beg_pos;
        off_t slice = file_size / 100;
        train_t train;
        // printf("beg_pos: %ld, file_size = %ld \n", beg_pos, file_size);
        while(1)
        {
            if(file_size > offset + (off_t)sizeof(train.buf))
            {
                // 一个小火车装不完的情况
                train.length = sizeof(train.buf);
                memcmp(train.buf, pmmap + offset, train.length);
                ret = sendCycle(pt->clientFd, &train, train.length + 4);
                if(-1 == ret)
                {
                    close(fileId);
                    return -1;
                }
                offset += train.length;
                if(offset - last_size >= slice)
                {
                    printf("\r%5.2f%%", (float)offset / file_size * 100);
                    fflush(stdout);
                    last_size = offset;
                }
            }
            else
            {
                // 最后一车
                bzero(&train, sizeof(train));
                train.length = file_size - offset;
                memcpy(train.buf, pmmap + offset, train.length);
                // printf("last train, length: %d, train.buf: %s \n", train.length, train.buf);
                ret = sendCycle(pt->clientFd, &train, train.length + 4);
                if(-1 == ret)
                {
                    close(fileId);
                    return -1;
                }
                break;
            }
        }
        printf("\r100.0000000%%\n");
        ret = munmap(pmmap, file_size);
        ERROR_CHECK(ret, -1, "munmap");
        // 发送传送结束标志

        train.length = 0;
        sendCycle(pt->clientFd, &train, 4);
        chdir(cwd);
        close(fileId);
    }
    else{
        puts("no this file");
    }
    sendCycle(pt->clientFd, &ret, 4);
    close(pt->clientFd);
    exitDb(&mydb);
    return 0;
}

