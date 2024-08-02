#include "head.h"
// 哈希表指针
User  *users = NULL;
// map 作为一个共享资源，需要对其加锁

pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER;

// 添加元素到哈希表
void mapAdd(int netFd, int pos) {
    pthread_mutex_lock(&user_mutex);
    User *user = (User *)malloc(sizeof(User));
    user->netFd = netFd;
    user->pos = pos;
    HASH_ADD_INT(users, netFd, user);
    pthread_mutex_unlock(&user_mutex);
}

// 查找元素
User *mapFind(int netFd) {
    pthread_mutex_lock(&user_mutex);
    User *user;
    HASH_FIND_INT(users, &netFd, user);
    pthread_mutex_unlock(&user_mutex);
    return user;  // 如果没有找到，user 将是 NULL
}

// 删除元素
void mapDel(User *user) {
    pthread_mutex_lock(&user_mutex);
    HASH_DEL(users, user);
    free(user);
    pthread_mutex_unlock(&user_mutex);
}

// 删除所有具有指定 pos 值的元素
void mapDelByPos(int pos) {
    pthread_mutex_lock(&user_mutex);
    User *current_user, *tmp;
    HASH_ITER(hh, users, current_user, tmp) {
        if (current_user->pos == pos) {
            HASH_DEL(users, current_user);
            free(current_user);
        }
    }
    pthread_mutex_unlock(&user_mutex);
}




