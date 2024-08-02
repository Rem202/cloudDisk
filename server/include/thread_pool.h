#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <func_zm.h>
typedef struct
{
    /* 任务处理函数 */
    void *(*func)(void *);
    void *args;
}thread_pool_task_t;

typedef struct
{
    /* data */
    pthread_mutex_t lock; 
    pthread_mutex_t thread_counter;
    pthread_cond_t queue_not_full;
    pthread_cond_t queue_not_empty;

    pthread_t *threads;
    pthread_t admin_tid;
    thread_pool_task_t *task_queue;

    int min_th_num;
    int max_th_num;
    int live_th_num;
    int busy_th_num;
    int wait_exit_th_num;

    int queue_front;
    int queue_rear;
    int queue_size;

    int queue_maxsize;
    int shutdown; // 线程池状态， true则为关闭
}thread_pool_t;

// 创建线程池
thread_pool_t *thread_pool_create(int min_th_num, int max_th_num, int queue_maxsize);
void * threadpool_thread(void *threadpool); // 工作线程的处理函数
int threadpool_add_task(thread_pool_t *pool, void *(*func)(void *args), void *args);
void *admin_thread(void *threadpool);
int is_thread_alive(pthread_t tid);
int threadpool_free(thread_pool_t *pool);
#define DEFAULT_TIME 5 // 每 5 s, 维护一下任务队列
#define DEFAULT_THREAD_NUM 5
#define MIN_WAIT_TASK_NUM 5
#endif
