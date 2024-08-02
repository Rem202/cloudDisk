// 用于定时维护用户数组
// 使用环形队列法， 每一个队列

#include "head.h"


int timerAdd(queue_timer *q, int epfd, int clientFd)
{
    // 首先，查询该clientFd 在 环形队列的哪一个位置
    User *user = mapFind(clientFd);
    if(NULL != user) // time_out s 内，该clientFd发送过消息
    {
        int pos = user->pos;
        clientNode *t = q->queue[pos];
        while(NULL != t && clientFd != t->netFd)
        {
            t = t->next;
        }
        q->queue[pos] = clientDel(q->queue[pos], t); // 删除掉该clientFd
        mapDel(user);                // 从map中删除掉
    }
    else
    {
        // 新用户第一次连接, 需要将其加入epoll监听
        // 
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = clientFd;
        int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, clientFd, &ev);
        ERROR_CHECK(ret, -1, "epoll_ctl_timerAdd");
    }
    // 当新用户过来时，直接插入到 环形队列中 current 之前的那个位置 所对应的链表中.并且更行Map

    int insertPos = (q->current - 1 < 0 ? (q->current + time_out): (q->current -1));
    q->queue[insertPos] = clientAdd(q->queue[insertPos], clientFd);
    printf("client: %ld insert to pos: %d \n", clientFd, insertPos);
    mapAdd(clientFd, insertPos);
    return 0;
}

int timerDel(queue_timer *q, int epfd, int clientFd)
{
    // 用于删除
    // 1 查找 clientFd 所在环形队列的 index
    User *user = mapFind(clientFd);
    if(NULL != user)
    {
        clientNode *head = q->queue[user->pos];
        while(NULL != head && head->netFd != clientFd)
        {
            head = head->next;
        }
        if(NULL == head)
        {
            // 发生错误，未找到
            return -1;
        }
        // 将其取消 epoll 的监听
        int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, clientFd, NULL); 
        ERROR_CHECK(ret, -1, "epoll_ctl_timerDel");
        close(clientFd);
        printf("client:%d exit\n", clientFd);
        q->queue[user->pos] = clientDel(q->queue[user->pos], head); // 刪除了對應的節點
        // 从 map 中也刪除對應的元素
        mapDel(user);
        return 0;
    }
    return -1;
}


int timerUpdate(queue_timer *q, int epfd)
{
    // 用于定时维护更新，将超时的clientFd 给删除，并不在监听
    time_t now = time(NULL);
    clientNode *head = q->queue[q->current];
    clientNode *front = NULL;
    while(NULL != head)
    {
        // 首先将其从epfd的监听中删除
        puts("queue_timer");
        printf("clientFd:%d \n", head->netFd);
        int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, head->netFd, NULL); 
        ERROR_CHECK(ret, -1, "epoll_ctl_timerUpdate");
        close(head->netFd); 
        // 从current 链表中删除
        printf("current:%d, cient: %ld timeout \n", q->current ,head->netFd);
        front = head;
        head = head->next;
        free(front);
    } 
    q->queue[q->current] = NULL;
    // 在 map 中删除 current 位置中的元素
    mapDelByPos(q->current);
    q->current = (q->current + 1) % (time_out + 1);
    return 0;
}