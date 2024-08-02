// 頭節點鏈表
#include "head.h"

clientNode* clientAdd(clientNode *head, int clientFd){
    //添加客戶端连接， 头插法
    clientNode *p = (clientNode *)calloc(1, sizeof(clientNode));
    ERROR_CHECK(p, NULL, "calloc");
    p->netFd = clientFd;
    p->next = head;
    return p;
}

clientNode* clientDel(clientNode *head, clientNode *delnode){
    if(NULL == delnode){
        return head;
    }
    if(NULL != delnode->next){
        delnode->netFd = delnode->next->netFd;
        clientNode * t = delnode->next;
        delnode->next = delnode->next->next;
        free(t);
    }
    else if(head == delnode && head->next == NULL){
        // 如果该节点头借点并且仅有头节点
        free(delnode);
        head = NULL;
    }
    else{
        // 删除尾节点
        clientNode *t= head;
        while(t->next!=delnode){
            t = t->next;
        }
        free(delnode);
        t->next = NULL;
    }
    return head;
}


clientNode *freeClient(clientNode *head)
{
    clientNode *t = head, *front = NULL;
    while(NULL != t)
    {
        front = t;
        t = t->next;
        free(front);
    }
    return NULL;
}
