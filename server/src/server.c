#include "thread_pool.h"
#include "head.h"
#define MAXEVENTS 1024 + 1 // netFd 最大值 设置为 1024

int epfd;
queue_timer q;
pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER; // 该锁用于维护定时器



void *ask(void *arg);
void display_map();
void getPath(MyDb *mydb, int fileId, char *path);

int main(int argc, char *argv[])
{
     // ./server
    // 启动多线程
    ARGS_CHECK(argc, 1);
    bzero(&q, sizeof(q));
    // 线程池初始化
    thread_pool_t *thp = thread_pool_create(10, 100, 100);

    // 数据库初始化
    MyDb mydb;
    initDb(&mydb, "111.230.115.250", "root", "chily7150", "zm", 3306);
    char **res = NULL; // 用于存放数据库中查询的内容
    char *one = NULL;

    // 网络初始化
    int sockFd;
    int Dir;
    tcpInit(&sockFd, IP, PORT);
    epfd = epoll_create(1);
    ERROR_CHECK(epfd, -1, "epoll_create");
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sockFd;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sockFd, &ev); // 将sockFd 加入监听
    ERROR_CHECK(ret, -1, epoll_ctl);

    // 日志系统
    Log log;
    initLog(&log);
    char operation[1024] = {0};
    char virWokrDir[256]; // 将当前的虚拟工作目录
    getPath(&mydb, 0, virWokrDir);

    // 使用一个线程来定时维护
    threadpool_add_task(thp, ask, NULL);

    // 先不进行认证
    int length = 0;
    char buf[1000] = {0};

    
    while (1)
    {
        struct epoll_event readyArr[MAXEVENTS]; // 设置对多监听的数量
        int readyNum = epoll_wait(epfd, readyArr, MAXEVENTS, -1);
        ERROR_CHECK(readyNum, -1, "epoll_wait");
        for (int i = 0; i < readyNum; ++i)
        {
            if (readyArr[i].data.fd == sockFd)
            {
                // 表明有新连接需要建立

                int newClientFd = accept(sockFd, NULL, NULL);
                ERROR_CHECK(newClientFd, -1, "accept");
                // 将客户端的 clientFd 发送给客户端
                // sendCycle(newClientFd, &newClientFd, 4);
                puts("=========================================================");

            accept_client:
                bzero(buf, sizeof(buf));
                recvCycle(newClientFd, &length, 4);
                ret = recvCycle(newClientFd, buf, length);

                printf("no connected client: %d order: %s \n", newClientFd, buf);
                // 暂时不对请求作出返回 对order进行拆分
                char order1[100] = {0};
                char option1[100] = {0};
                char option2[100] = {0};

                char *word = NULL;
                word = strtok(buf, "_  \n\t");
                strcpy(order1, word);

                if ((word = strtok(NULL, "_  \n\t")) != NULL)
                {
                    strcpy(option1, word);
                }

                if ((word = strtok(NULL, "_  \n\t")) != NULL)
                {
                    strcpy(option2, word);
                }

                if (strcmp(order1, "login") == 0)
                {
                    // 登陆
                    char sql[] = "select salt from UserTable where User = '";
                    strcat(sql, option1);
                    strcat(sql, "'");
                    int row = execOneSql(&mydb, sql, &one);
                    if (row == -1)
                    {
                        // 则表示没有该用户
                        length = 0;
                        sendCycle(newClientFd, &length, sizeof(length));
                        goto accept_client;
                    }
                    else
                    {
                        length = strlen(one);

                        sendCycle(newClientFd, &length, sizeof(length));
                        // 将 salt 发送给客户端
                        sendCycle(newClientFd, one, length);
                        // 接受返回的 cipher 密文
                        recvCycle(newClientFd, &length, 4);
                        bzero(buf, sizeof(buf));

                        recvCycle(newClientFd, buf, length);
                        char sql[] = "select Cipher from UserTable where User = '";
                        strcat(sql, option1);
                        strcat(sql, "'");
                        int row = execOneSql(&mydb, sql, &one);
                        length = strcmp(one, buf); // 将比较结果发送过去，如果 == 0 则表示验证成功
                        free(one);
                        one = NULL;
                        sendCycle(newClientFd, &length, 4);
                        if (length == 0) // 验证成功则将该用户加入到已连接的用户队列
                        {
                            puts("密码正确");
                            pthread_mutex_lock(&mutex_queue);
                            timerAdd(&q, epfd, newClientFd);
                            pthread_mutex_unlock(&mutex_queue);
                            display_map();
                        }
                        else
                        {
                            goto accept_client;
                        }
                    }
                }
                else if (strcmp(order1, "regis") == 0)
                {
                    // 注册命令
                    char sql[] = "select salt from UserTable where User = '";
                    strcat(sql, option1);
                    strcat(sql, "'");
                    int row = execSql(&mydb, sql);
                    sendCycle(newClientFd, &row, sizeof(row));
                    if (row == 0)
                    {
                        char *salt = (char *)calloc(8, sizeof(char));
                        // 生成salt
                        gennerateStr(&salt, 8);
                        // 生成 cipher
                        char *cipher = crypt(option2, salt);
                        char sql[256];
                        sprintf(sql, "INSERT into UserTable VALUES('%s', '%s', '%s', '%s')",option1, option2, salt, cipher);
                        puts(sql);
                        row = execSql(&mydb, sql);
                        sendCycle(newClientFd, &row, sizeof(row));
                        goto accept_client;
                    }
                }
                else if (strcmp(order1, "puts") == 0)
                {
                    // 当用户认证过后， 在命令行输入 puts .. 命令后 会建立一个新的 clientFd 用于上传数据

                    int Dir;
                    recvCycle(newClientFd, &Dir, 4);
                    printf("Dir = %d \n", Dir);

                    // int parentFd;
                    // recvCycle(newClientFd, &parentFd, 4);
                    // printf("client:%d 建立新连接: %d 来进行puts\n", parentFd, newClientFd);

                    task_t t;
                    t.clientFd = newClientFd;
                    t.Dir = Dir;
                    strcpy(t.username, option1);
                    strcpy(t.file, option2);

                    // 构造好任务后，添加进入任务队列, puts 和 gets 是用的一個新的 sockFd 單獨進行上傳和下載,不用加入定时器，刷新父连接的时间
                    // pthread_mutex_lock(&mutex_queue);
                    // timerAdd(&q, epfd, parentFd);
                    // pthread_mutex_unlock(&mutex_queue);

                    threadpool_add_task(thp, puts_task, &t);
                    bzero(operation, sizeof(operation));
                    sprintf(operation, "order: puts, file:%s/%s\n", virWokrDir, option2);
                    addLog(&log, operation, option1);
                }
                else if (strcmp(order1, "gets") == 0)
                {
                    int Dir;
                    recvCycle(newClientFd, &Dir, 4);
                    printf("Dir = %d \n", Dir);

                    // int parentFd;
                    // recvCycle(newClientFd, &parentFd, 4);
                    // printf("client:%d 建立新连接: %d 来进行gets\n", parentFd, newClientFd);

                    task_t t;
                    t.clientFd = newClientFd;
                    t.Dir = Dir;
                    strcpy(t.username, option1);
                    strcpy(t.file, option2);

                    // pthread_mutex_lock(&mutex_queue);
                    // timerAdd(&q, epfd, parentFd);
                    // pthread_mutex_unlock(&mutex_queue);

                    threadpool_add_task(thp, gets_task, &t);
                    bzero(operation, sizeof(operation));
                    sprintf(operation, "order: gets, file:%s/%s\n", virWokrDir, option2);
                    addLog(&log, operation, option1);
                }
            }
        }

        // 接受已連接客戶端發送過來的命令i
        for (int i = 0; i < readyNum; ++i)
        {
            User *current_user, *tmp;
            HASH_ITER(hh, users, current_user, tmp)
            {
                if (readyArr[i].data.fd == current_user->netFd)
                {
                    puts("=========================================================");
                    puts("recv order from client:");
                    printf("clientFd: %d \n", current_user->netFd);
                    length = 0;
                    bzero(buf, sizeof(buf));
                    recvCycle(readyArr[i].data.fd, &length, sizeof(length));
                    ret = recvCycle(readyArr[i].data.fd, buf, length);
                    if (ret == 0)
                    {
                        pthread_mutex_lock(&mutex_queue);
                        timerDel(&q, epfd, readyArr[i].data.fd);
                        pthread_mutex_unlock(&mutex_queue);
                        display_map();
                        continue;
                    }
                    else
                    {
                        printf("client: %d order: %s ,", readyArr[i].data.fd, buf);
                        // 暂时不对请求作出返回 对order进行拆分
                        char order[100] = {0};

                        char *word = NULL;
                        word = strtok(buf, " \n\t");
                        strcpy(order, word);

                        // 这里开始对已连接的客户端的命令进行解析

                        if (strcmp(order, "ls") == 0)
                        {
                            char sql[256] = {0};
                            // 接受用户名
                            bzero(buf, sizeof(buf));
                            recvCycle(readyArr[i].data.fd, &length, sizeof(length));
                            // 接受名字
                            recvCycle(readyArr[i].data.fd, buf, length);

                            printf("username: %s ,", buf);

                            char username[256] = {0};
                            strcpy(username, buf);

                            // 接受目录号
                            recvCycle(readyArr[i].data.fd, &Dir, sizeof(Dir));
                            printf("Dir: %d \n", Dir);
                            sprintf(sql, "SELECT FileName, FileType FROM FileTable WHERE `User` = '%s' and Dir = %d", buf, Dir);
                            int row = execSqls(&mydb, sql, &res, 0);
                            // 将结果传回给客户端
                            // printf("row = %d, res = %s\n", row, res[0]);
                            length = 0;
                            for (int file = 0; file < row; ++file)
                            {
                                length += strlen(res[file]);
                            }
                            sendCycle(readyArr[i].data.fd, &length, sizeof(length));
                            sendCycle(readyArr[i].data.fd, &row, sizeof(row));

                            bzero(operation, sizeof(operation));
                            sprintf(operation, "order: ls, dir:%s\n, res:", virWokrDir);

                            for (int file = 0; file < row; ++file)
                            {
                                sendCycle(readyArr[i].data.fd, res[file], strlen(res[file]));
                                strcat(operation, res[file]);
                                strcat(operation, "\n");
                            }

                            // 释放 res资源
                            for (int f = 0; f < row; ++f)
                            {
                                free(res[f]);
                            }
                            free(res);
                            res = NULL;
                            ret = addLog(&log, operation, username); // ls 不加入日志
                        }
                        else if (strcmp(order, "mkdir") == 0)
                        {
                            // 接受用户名
                            bzero(buf, sizeof(buf));
                            recvCycle(readyArr[i].data.fd, &length, sizeof(length));
                            // 接受名字
                            recvCycle(readyArr[i].data.fd, buf, length);
                            char username[256] = {0};
                            strcpy(username, buf);
                            printf("username: %s ,", username);
                            // 首先接受一个目录名
                            recvCycle(readyArr[i].data.fd, &length, 4);
                            bzero(buf, sizeof(buf));
                            recvCycle(readyArr[i].data.fd, buf, length);
                            printf("目录名: %s ,", buf);
                            // 接受目录号
                            recvCycle(readyArr[i].data.fd, &Dir, 4);
                            printf("Dir: %d \n", Dir);
                            // sendCycle(t->netFd, &Dir, 4);

                            char sql[256] = {0};
                            sprintf(sql, "select * from FileTable where user = '%s' and Dir = %d and FileType = 'd' and FileName = '%s'", username, Dir, buf);
                            // puts(sql);
                            int ret = execSql(&mydb, sql);
                            // 给客户端发送结果
                            sendCycle(readyArr[i].data.fd, &ret, 4);
                            if (ret == 0)
                            {
                                bzero(sql, sizeof(sql));
                                sprintf(sql, "INSERT into FileTable(Dir,FileName,FileSize,FileType,MD5,`User`) values (%d, '%s', 0, 'd', 0, '%s')", Dir, buf, username);
                                // 创建目录
                                ret = execSql(&mydb, sql);
                                bzero(operation, sizeof(operation));
                                sprintf(operation, "order: mkdir, Dir:%s/%s\n", virWokrDir, buf);
                                addLog(&log, operation, username);
                            }
                        }
                        else if (strcmp(order, "rmdir") == 0)
                        {
                            bzero(buf, sizeof(buf));
                            recvCycle(readyArr[i].data.fd, &length, sizeof(length));
                            // 接受名字
                            recvCycle(readyArr[i].data.fd, buf, length);
                            char username[256] = {0};
                            strcpy(username, buf);
                            printf("username: %s ,", username);
                            // 首先接受一个目录名
                            recvCycle(readyArr[i].data.fd, &length, 4);
                            bzero(buf, sizeof(buf));
                            recvCycle(readyArr[i].data.fd, buf, length);
                            printf("目录名: %s ,", buf);
                            // 接受目录号
                            recvCycle(readyArr[i].data.fd, &Dir, 4);
                            printf("Dir: %d \n", Dir);

                            char sql[256] = {0};
                            sprintf(sql, "delete  from FileTable where user = '%s' and Dir = %d and FileType = 'd' and FileName = '%s'", username, Dir, buf);
                            int ret = execSql(&mydb, sql);
                            sendCycle(readyArr[i].data.fd, &ret, 4);
                            if (1 == ret)
                            {
                                bzero(operation, sizeof(operation));
                                sprintf(operation, "order: rmdir, Dir:%s/%s\n", virWokrDir, buf);
                                addLog(&log, operation, username);
                            }
                        }
                        else if (strcmp(order, "cd") == 0)
                        {
                            bzero(buf, sizeof(buf));
                            recvCycle(readyArr[i].data.fd, &length, sizeof(length));
                            // 接受名字
                            recvCycle(readyArr[i].data.fd, buf, length);
                            char username[256] = {0};
                            strcpy(username, buf);
                            printf("username: %s ,", username);
                            // 首先接受一个目录名
                            recvCycle(readyArr[i].data.fd, &length, 4);
                            bzero(buf, sizeof(buf));
                            recvCycle(readyArr[i].data.fd, buf, length);
                            printf("目录名: %s ,", buf);

                            recvCycle(readyArr[i].data.fd, &Dir, 4);
                            printf("Dir: %d \n", Dir);

                            char sql[256] = {0};
                            if (strcmp(buf, "..") == 0 || strcmp(buf, "../") == 0)
                            {
                                sprintf(sql, "SELECT Dir FROM FileTable WHERE User = '%s' and FileId = %d", username, Dir);
                            }
                            else
                            {
                                sprintf(sql, "SELECT FileId FROM FileTable WHERE User = '%s' and Dir = %d and FileName = '%s'", username, Dir, buf);
                            }
                            int ret = execOneSql(&mydb, sql, &one);
                            if (strcmp(one, "") == 0)
                            {
                                ret = -2; // 该目录不存在                            }
                            }
                            else
                            {
                                ret = atoi(one);
                                printf("fileid: %s \n", one);
                                free(one);
                                one = NULL;

                                bzero(operation, sizeof(operation));
                                sprintf(operation, "order: cd, Dir:%s/%s\n", virWokrDir, buf);
                                addLog(&log, operation, username);
                                // cd 命令也不加入到日志系统
                            }
                            sendCycle(readyArr[i].data.fd, &ret, 4);
                            if (-2 != ret)
                            {
                                bzero(virWokrDir, sizeof(virWokrDir));
                                getPath(&mydb, ret, virWokrDir);
                                length = strlen(virWokrDir);
                                sendCycle(readyArr[i].data.fd, &length, 4);
                                sendCycle(readyArr[i].data.fd, virWokrDir, length);
                            }
                        }
                        else if (strcmp(order, "rm") == 0)
                        {
                            // 接收名字
                            bzero(buf, sizeof(buf));
                            recvCycle(readyArr[i].data.fd, &length, sizeof(length));
                            recvCycle(readyArr[i].data.fd, buf, length);
                            char username[256] = {0};
                            strcpy(username, buf);
                            printf("username: %s ,", username);

                            // 接收一个文件名
                            recvCycle(readyArr[i].data.fd, &length, 4);
                            bzero(buf, sizeof(buf));
                            recvCycle(readyArr[i].data.fd, buf, length);
                            printf("文件名: %s ,", buf);

                            // 接受目录号
                            recvCycle(readyArr[i].data.fd, &Dir, 4);
                            printf("Dir: %d \n", Dir);

                            char sql[256] = {0};
                            sprintf(sql, "delete from FileTable where User = '%s' and Dir = %d and FileName = '%s'", username, Dir, buf);
                            int ret = execSql(&mydb, sql);
                            sendCycle(readyArr[i].data.fd, &ret, 4);

                            if (1 == ret)
                            {
                                bzero(operation, sizeof(operation));
                                sprintf(operation, "order: rm, file:%s/%s\n", virWokrDir, buf);
                                addLog(&log, operation, username);
                            }
                        }
                        pthread_mutex_lock(&mutex_queue);
                        timerAdd(&q, epfd, readyArr[i].data.fd);
                        pthread_mutex_unlock(&mutex_queue);
                    }
                }
            }
        }
    }
    threadpool_destroy(thp);
    return 0;
}

void getPath(MyDb *mydb, int fileId, char *path)
{
    char sql[256];
    char name[256];
    unsigned long length;

    if (fileId == 0)
    {
        // 根目录
        strcpy(path, "/");
        return;
    }

    // 查询当前文件或目录的名称
    snprintf(sql, sizeof(sql), "SELECT FileName FROM FileTable WHERE FileId = %d", fileId);
    execPath(mydb, sql, name, &length);
    printf("fileId = %d, name = %s\n", fileId, name);

    // 查询父目录的 FileId
    snprintf(sql, sizeof(sql), "SELECT Dir FROM FileTable WHERE FileId = %d", fileId);
    int parentDirId;

    if (mysql_query(mydb->mysql, sql))
    {
        fprintf(stderr, "SELECT error: %s\n", mysql_error(mydb->mysql));
        exit(EXIT_FAILURE);
    }

    mydb->result = mysql_store_result(mydb->mysql);
    if (mydb->result == NULL)
    {
        fprintf(stderr, "mysql_store_result() failed\n");
        exit(EXIT_FAILURE);
    }

    if ((mydb->row = mysql_fetch_row(mydb->result)) != NULL)
    {
        parentDirId = atoi(mydb->row[0]);
    }

    mysql_free_result(mydb->result);

    // 递归获取父目录的路径
    getPath(mydb, parentDirId, path);

    // 拼接当前目录名称
    if (strlen(path) > 1)
    {
        strcat(path, "/");
    }
    strcat(path, name);
}

void *ask(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&mutex_queue);
        timerUpdate(&q, epfd);
        pthread_mutex_unlock(&mutex_queue);
        sleep(1);
    }
    pthread_exit(NULL);
}

void display_map()
{
    User *current_user, *tmp;
    puts("====================================== client list ====================================");
    HASH_ITER(hh, users, current_user, tmp)
    {
        printf("%d ->", current_user->netFd);
    }
    puts("");
}