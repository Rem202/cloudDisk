#include "head.h"
#include <func_zm.h>
// #include "thread_pool.h"


int main(int argc, char *argv[])
{
    // client ip port
    int ret;
    int datalen = 0;
    char buf[1000] = {0};
    train_t train;
    bool flag;
    int Dir = 0;
    char post[100] = "netDisk@";
    char cwd[1024] = "/";
    char username[1024] = {0};
    char password[1024] = {0};
    char order[1024] = {0};
    char option1[1024] = {0};
    char option2[1024] = {0};
    // 首先 需要连接服务器
    int sockFd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_CHECK(sockFd, -1, "socket");
    struct sockaddr_in sockinfo;
    bzero(&sockinfo, sizeof(sockinfo));
    sockinfo.sin_addr.s_addr = inet_addr(IP);
    sockinfo.sin_port = htons(PORT);
    sockinfo.sin_family = AF_INET;

    ret = connect(sockFd, (struct sockaddr *)&sockinfo, sizeof(sockinfo));
    ERROR_CHECK(ret, -1, "connnect");
    puts("connect to the server successfully!");
    // int clientFd;
    // recvCycle(sockFd, &clientFd, 4);

    bool loop_1 = true, loop_2 = false;

    while (1)
    {
        if (loop_1 && !loop_2)
        { // 进行登陆 注册 等操作
            while (1)
            {

                printf(
                    "选择登陆或者注册: \n"
                    "\t1:登陆\n"
                    "\t2:注册\n"
                    "\t3:退出\n");
                printf("netDisk>>");
                int choice;
                int res = scanf("%d", &choice);
                if (res != 1)
                {
                    // 输入不是一个整数
                    puts("invalid command");
                    // 清空输入缓冲区
                    while (getchar() != '\n');
                    continue;
                }
                switch (choice)
                {
                case 1:
                    // 进行登陆的操作， 首先需要根据用户名来从用户表中取出salt
                    printf("netDisk>>请输入账户:");
                    scanf("%s", username);
                    printf("netDisk>>密码:");
                    scanf("%s", password);
                    bzero(order, sizeof(order));
                    strcat(order, "login_");
                    strcat(order, username);

                    bzero(&train, sizeof(train));

                    strcpy(train.buf, order);
                    train.length = strlen(train.buf);
                    sendCycle(sockFd, (void *)&train, train.length + sizeof(train.length));

                    bzero(&train, sizeof(train));
                    recvCycle(sockFd, &train.length, sizeof(train.length));
                    if (train.length == 0)
                    {
                        printf("%s doesn't exist！\n", username);
                    }
                    else
                    {
                        recvCycle(sockFd, train.buf, train.length);

                        // 将接受到的 salt 使用crypt 函数 和 password 进行加密后再发送给客户端
                        char *cipher = crypt(password, train.buf);
                        bzero(&train, sizeof(train));
                        train.length = strlen(cipher);
                        strcpy(train.buf, cipher);
                        sendCycle(sockFd, &train, train.length + 4);

                        // 接受一个length 如果 = 1 则表示密码正确
                        recvCycle(sockFd, &train.length, 4);
                        if (train.length == 0)
                        {
                            // puts("netDisk>>密码正确");
                            loop_1 = false;
                            loop_2 = true;
                        }
                        else
                        {
                            puts("netDisk>>密码错误!");
                        }
                    }
                    break;
                case 2:
                    /* 注册用户 */
                    printf("netDisk>>请输入账户:");
                    scanf("%s", username);
                    printf("netDisk>>密码:");
                    scanf("%s", password);
                    bzero(order, sizeof(order));
                    strcat(order, "regis_");
                    strcat(order, username);
                    strcat(order, "_");
                    strcat(order, password);
                    bzero(&train, sizeof(train));
                    strcpy(train.buf, order);
                    train.length = strlen(train.buf);
                    sendCycle(sockFd, (void *)&train, train.length + sizeof(train.length));

                    bzero(&train, sizeof(train));
                    // 判断是否已经存在该用户
                    recvCycle(sockFd, &train.length, sizeof(train.length));
                    if (train.length == 1)
                    {
                        printf("user: %s has already exist!", username);
                        break;
                    }
                    else
                    {
                        // 接受注册结果
                        recvCycle(sockFd, &train.length, sizeof(train.length));
                        if (train.length == 1)
                        {
                            puts("register success!");
                        }
                        else
                        {
                            puts("register failed");
                        }
                    }

                    break;
                case 3:
                    /* 直接退出 */
                    loop_1 = false;
                    loop_2 = false;
                    break;

                default:
                    puts("");
                    puts("invalid command");
                    break;
                }
                if (!loop_1)
                    break;
            }
        }
        else if (!loop_1 && loop_2)
        {
            puts("connected to the server !");
            // 连接服务器成功， 先不进行认证，直接发送消息
            strcat(post, username);
            while (1)
            {
                // 输入对应的命令

                printf("\033[92m%s\033[0m:\033[34m%s\033[0m$ ", post, cwd);
                fflush(stdout);
                bzero(&train, sizeof(train));
                train.length = read(STDIN_FILENO, train.buf, sizeof(train.buf));
                if (train.length == 0)
                {
                    break;
                }
                else if (train.length == 1)
                {
                    continue; // 用户按enter 健
                }
                ERROR_CHECK(train.length, -1, "read");
                // 对命令进行解析
                char *word = NULL;
                char *delimiters = " \n\t\0";
                word = strtok(train.buf, delimiters);
                // puts(word);
                bzero(order, sizeof(order));
                strcpy(order, word);
                if ((word = strtok(NULL, delimiters)) != NULL)
                {
                    bzero(option1, sizeof(option1));
                    strcpy(option1, word);
                }
                if (strcmp(order, "ls") == 0)
                {
                    // 向服务器发送命令
                    bzero(&train, sizeof(train));
                    strcpy(train.buf, order);
                    train.length = strlen(train.buf);
                    sendCycle(sockFd, &train, train.length + sizeof(train.length));

                    // 再发送一下 用户名
                    bzero(&train, sizeof(train));
                    strcpy(train.buf, username);
                    train.length = strlen(train.buf);
                    sendCycle(sockFd, &train, train.length + sizeof(train.length));

                    // 发送目录号
                    sendCycle(sockFd, &Dir, sizeof(Dir));

                    // 接受返回的信息
                    bzero(&train, sizeof(train));
                    recvCycle(sockFd, &train.length, sizeof(train.length));
                    int row;
                    recvCycle(sockFd, &row, sizeof(row));
                    recvCycle(sockFd, train.buf, train.length);
                    char *word = strtok(train.buf, delimiters);
                    while (word)
                    {
                        char file_name[256] = {0};
                        strcpy(file_name, word);
                        word = strtok(NULL, delimiters);
                        if (strcmp(word, "d") == 0)
                        {
                            printf("\033[34m%s\033[0m ", file_name);
                        }
                        else
                        {
                            printf("%s ", file_name);
                        }
                        word = strtok(NULL, delimiters);
                    }
                    puts("");
                }
                else if (strcmp(order, "mkdir") == 0)
                {
                    // 向服务器发送命令
                    bzero(&train, sizeof(train));
                    strcpy(train.buf, order);
                    train.length = strlen(train.buf);
                    sendCycle(sockFd, &train, train.length + sizeof(train.length));
                    // 将用户名发送过去
                    bzero(&train, sizeof(train));
                    strcpy(train.buf, username);
                    train.length = strlen(train.buf);
                    sendCycle(sockFd, &train, train.length + sizeof(train.length));
                    // 发送需要创建的目录名称
                    bzero(&train, sizeof(train));
                    strcpy(train.buf, option1);
                    train.length = strlen(train.buf);
                    sendCycle(sockFd, &train, train.length + sizeof(train.length));

                    // 发送目录号
                    sendCycle(sockFd, &Dir, 4);
                    recvCycle(sockFd, &train.length, 4);
                    if (train.length == 1)
                    {
                        printf("\033[31mError: %s has already exist !\033[0m\n", option1);
                    }
                }
                else if (strcmp(order, "rmdir") == 0)
                {
                    // 向服务器发送命令
                    bzero(&train, sizeof(train));
                    strcpy(train.buf, order);
                    train.length = strlen(train.buf);
                    sendCycle(sockFd, &train, train.length + sizeof(train.length));
                    // 删除目录
                    // 首先发送目录名称
                    bzero(&train, sizeof(train));
                    strcpy(train.buf, username);
                    train.length = strlen(train.buf);
                    sendCycle(sockFd, &train, train.length + sizeof(train.length));

                    bzero(&train, sizeof(train));
                    strcpy(train.buf, option1);
                    train.length = strlen(train.buf);
                    sendCycle(sockFd, &train, train.length + sizeof(train.length));

                    // 发送目录号
                    sendCycle(sockFd, &Dir, 4);

                    recvCycle(sockFd, &train.length, 4);
                    if (train.length == 1)
                    {
                        // puts("删除成功");
                    }
                    else if (train.length == 0)
                    {
                        printf("\033[31mError: directory %s doesn't exist !\033[0m\n", option1);
                    }
                    else if (train.length == -2)
                    {
                        printf("\033[31mError: %s is not empty!!\033[0m\n", option1);
                    }
                }
                else if (strcmp(order, "cd") == 0)
                {
                    if (strcmp(option1, ".") == 0 || strcmp(option1, "./") == 0 || (Dir == 0 && strcmp(option1, "..") == 0) || (Dir == 0 && strcmp(option1, "../") == 0))
                    {
                        // do nothing
                        continue;
                    }
                    else
                    {
                        // 向服务器发送命令
                        bzero(&train, sizeof(train));
                        strcpy(train.buf, order);
                        train.length = strlen(train.buf);
                        sendCycle(sockFd, &train, train.length + sizeof(train.length));
                        // 发送用户名
                        bzero(&train, sizeof(train));
                        strcpy(train.buf, username);
                        train.length = strlen(train.buf);
                        sendCycle(sockFd, &train, train.length + sizeof(train.length));
                        // 发送需要进入目录名称
                        bzero(&train, sizeof(train));
                        strcpy(train.buf, option1);
                        train.length = strlen(train.buf);
                        sendCycle(sockFd, &train, train.length + sizeof(train.length));

                        // 发送目录号
                        sendCycle(sockFd, &Dir, 4);
                        recvCycle(sockFd, &train.length, 4);
                        if (train.length == -2)
                        {
                            printf("\033[31mError: %s not exist !\033[0m\n", option1);
                        }
                        else
                        {
                            Dir = train.length;
                            bzero(&train, sizeof(train));
                            recvCycle(sockFd, &train.length, 4);
                            recvCycle(sockFd, train.buf, train.length);
                            bzero(cwd, sizeof(cwd));
                            strcpy(cwd, train.buf);
                        }
                    }
                }
                else if (strcmp(order, "rm") == 0)
                {
                    // 发送命令
                    bzero(&train, sizeof(train));
                    strcpy(train.buf, order);
                    train.length = strlen(train.buf);
                    sendCycle(sockFd, &train, train.length + sizeof(train.length));
                    // 发送用户名
                    bzero(&train, sizeof(train));
                    strcpy(train.buf, username);
                    train.length = strlen(train.buf);
                    sendCycle(sockFd, &train, train.length + sizeof(train.length));
                    // 发送文件名称
                    bzero(&train, sizeof(train));
                    strcpy(train.buf, option1);
                    train.length = strlen(train.buf);
                    sendCycle(sockFd, &train, train.length + sizeof(train.length));

                    // 发送目录号
                    sendCycle(sockFd, &Dir, 4);

                    // 接收执行结果
                    recvCycle(sockFd, &train.length, 4);
                    if (train.length == 0)
                    {
                        printf("\033[31mError: file %s doesn't exist !\033[0m\n", option1);
                    }
                }
                else if (strcmp(order, "puts") == 0)
                {
                    // printf("order: %s, file_name: %s \n", order, option1);
                    task_t t;
                    strcpy(t.order, order);
                    strcpy(t.username, username);
                    strcpy(t.option, option1);
                    t.Dir = Dir;

                    //
                    pthread_t pth1;
                    pthread_create(&pth1, NULL, upload, &t);
                    pthread_join(pth1, NULL);
                    // puts("");
                }
                else if (strcmp(order, "gets") == 0)
                {
                    printf("order: %s, file_name: %s \n", order, option1);
                    task_t t;
                    // t.parentFd = clientFd;
                    strcpy(t.order, order);
                    strcpy(t.username, username);
                    strcpy(t.option, option1);
                    t.Dir = Dir;

                    pthread_t pth1;
                    pthread_create(&pth1, NULL, download, &t);
                    pthread_join(pth1, NULL);

                }
                else if (strcmp(order, "quit") == 0)
                {
                    loop_1 = false;
                    loop_2 = false;
                    break;
                }
                else
                {
                    puts("\033[31m invalid command \033[0m");
                }
            }
        }
        else if (!loop_1 && !loop_2)
        {
            break; // 退出主进程
        }
    }

    return 0;
}

void handle_sigpipe(int sig) {
    puts("\033[31mError: Time out, Bye!\033[0m");
    exit(EXIT_FAILURE); // 终止进程
}