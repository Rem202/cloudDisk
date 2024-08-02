#include "thread_pool.h"
#include <func_zm.h>

thread_pool_t *thread_pool_create(int min_th_num, int max_th_num, int queue_maxsize){
    int ret;
    thread_pool_t *pool = NULL;
    do{
        pool = (thread_pool_t *)calloc(1, sizeof(thread_pool_t));  // 进程池开辟空间,并将所有元素全部置为0
        ERROR_CHECK(pool, "NULL", "calloc during thread_pool_create");
        // THREAD_ERROR_CHECK

        pool->min_th_num = min_th_num;
        pool->max_th_num = max_th_num;
        // pool->busy_thr_num = 0;
        pool->queue_maxsize = queue_maxsize;
        pool->shutdown = false; 

        // 根据最大线数， 给工作进程组开辟空间
        pool->threads = (pthread_t *)calloc(max_th_num, sizeof(pthread_t));
        ERROR_CHECK(pool->threads, NULL, "calloc during thread_pool_create 1");

        // 给工作队列开辟空间
        pool->task_queue = (thread_pool_task_t *)calloc(queue_maxsize, sizeof(thread_pool_task_t));
        ERROR_CHECK(pool->task_queue, NULL, "calloc during thread_pool_create 2");

        // 初始化互斥锁和条件变量
        pthread_mutex_init(&(pool->lock), NULL);
        pthread_mutex_init(&(pool->thread_counter), NULL);
        pthread_cond_init(&(pool->queue_not_empty), NULL);
        pthread_cond_init(&(pool->queue_not_full), NULL);

        // 启动 min_th_num 个工作线程
        for(int i = 0; i < min_th_num; ++i){
            pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool);
            // printf("thread %d start \n", (int)pool->threads[i]);
        }

        // 启动管理线程组
        pthread_create(&(pool->admin_tid), NULL, admin_thread, (void *)pool);
        printf("thread pool created! busy_num = %d, live_num = %d, exit_num = %d \n", pool->busy_th_num, pool->live_th_num, pool->wait_exit_th_num);
        return pool;
    }while (0);

    return NULL;
}


/*工作线程*/
void *threadpool_thread(void *threadpool)
{
  thread_pool_t *pool = (thread_pool_t *)threadpool;
  thread_pool_task_t task;

  while (true)
  {
    pthread_mutex_lock(&(pool->lock));

    /* 无任务则阻塞在 “任务队列为空” 上，有任务则跳出 */
    /*当任务队列为空并且线程池不关闭时阻塞，等待任务，有任务则跳出循环*/
    while ((pool->queue_size == 0) && (!pool->shutdown))
    { 
      //  printf("thread %d is waiting \n", (int)pthread_self());
       pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));  

       /* 判断是否需要清除线程,自杀功能 */
       if (pool->wait_exit_th_num > 0)
       {
          pool->wait_exit_th_num--;
          /* 判断线程池中的线程数是否大于最小线程数，是则结束当前线程 */
          if (pool->live_th_num > pool->min_th_num)
          {
            //  printf("thread 0x%x is exiting \n", (unsigned int)pthread_self());
             pool->live_th_num--;
             pthread_mutex_unlock(&(pool->lock));
             pthread_exit(NULL);//结束线程
          }
       }
    }

    /* 线程池开关状态 */
    if (pool->shutdown) //关闭线程池
    {
       pthread_mutex_unlock(&(pool->lock));
       printf("thread 0x%x is exiting \n", (unsigned int)pthread_self());
       pthread_exit(NULL); //线程自己结束自己
    }

    //否则该线程可以拿出任务
    task.func = pool->task_queue[pool->queue_front].func; //出队操作
    task.args = pool->task_queue[pool->queue_front].args;

    pool->queue_front = (pool->queue_front + 1) % pool->queue_maxsize;  //环型结构
    pool->queue_size--;

    //通知可以添加新任务
    pthread_cond_broadcast(&(pool->queue_not_full));

    //释放线程锁
    pthread_mutex_unlock(&(pool->lock));

    //执行刚才取出的任务
   //  printf("thread 0x%x start working \n", (unsigned int)pthread_self());
    pthread_mutex_lock(&(pool->thread_counter));            //锁住忙线程变量
    pool->busy_th_num++;
    pthread_mutex_unlock(&(pool->thread_counter));

    (*(task.func))(task.args);                           //执行任务

    //任务结束处理
   //  printf("thread 0x%x end working \n", (unsigned int)pthread_self());
    pthread_mutex_lock(&(pool->thread_counter));
    pool->busy_th_num--;
    pthread_mutex_unlock(&(pool->thread_counter));
  }
  pthread_exit(NULL);
}


/*向线程池的任务队列中添加一个任务*/
int threadpool_add_task(thread_pool_t *pool, void *(*func)(void *args), void *args)
{
   pthread_mutex_lock(&(pool->lock));

   /*如果队列满了,调用wait阻塞*/
   while ((pool->queue_size == pool->queue_maxsize) && (!pool->shutdown))
      pthread_cond_wait(&(pool->queue_not_full), &(pool->lock));

   /*如果线程池处于关闭状态*/
   if (pool->shutdown)
   {
      pthread_mutex_unlock(&(pool->lock));
      return -1;
   }

   /*清空工作线程的回调函数的参数arg*/
   if (pool->task_queue[pool->queue_rear].args != NULL)
   {
      free(pool->task_queue[pool->queue_rear].args);
      pool->task_queue[pool->queue_rear].args = NULL;
   }

   /*添加任务到任务队列*/
   pool->task_queue[pool->queue_rear].func = func;
   pool->task_queue[pool->queue_rear].args = args;
   pool->queue_rear = (pool->queue_rear + 1) % pool->queue_maxsize;  /* 逻辑环  */
   pool->queue_size++;

   /*添加完任务后,队列就不为空了,唤醒线程池中的一个线程*/
   pthread_cond_signal(&(pool->queue_not_empty));
   pthread_mutex_unlock(&(pool->lock));
   return 0;
}



/*管理线程*/
void *admin_thread(void *threadpool)
{
   int i;
   thread_pool_t *pool = (thread_pool_t *)threadpool;
   while (!pool->shutdown)
   {
      // printf("admin -----------------\n");
      sleep(DEFAULT_TIME);                             /*隔一段时间再管理*/
      pthread_mutex_lock(&(pool->lock));                  /*加锁*/ 
      int queue_size = pool->queue_size;                  /*任务数*/
      int live_th_num = pool->live_th_num;                /*存活的线程数*/
      pthread_mutex_unlock(&(pool->lock));                /*解锁*/

      pthread_mutex_lock(&(pool->thread_counter));
      int busy_th_num = pool->busy_th_num;                 /*忙线程数*/  
      pthread_mutex_unlock(&(pool->thread_counter));

      // printf("admin busy live -%d--%d-\n", busy_th_num, live_th_num);
      /*创建新线程 实际任务数量大于 最小正在等待的任务数量，存活线程数小于最大线程数*/
      if (queue_size >= MIN_WAIT_TASK_NUM && live_th_num <= pool->max_th_num)
      {
         // printf("admin add-----------\n");
         pthread_mutex_lock(&(pool->lock));
         int add=0;

         /*一次增加 DEFAULT_THREAD_NUM default 5 个线程*/
         for (i=0; i<pool->max_th_num && add< DEFAULT_THREAD_NUM 
              && pool->live_th_num < pool->max_th_num; ++i)
         {
            if (pool->threads[i] == 0 || !is_thread_alive(pool->threads[i]))
           {
              pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool);
              add++;
              pool->live_th_num++;
            //   printf("new thread -----------------------\n");
           }
         }

         pthread_mutex_unlock(&(pool->lock));
      }

      /*销毁多余的线程 忙线程x2 都小于 存活线程，并且存活的大于最小线程数*/
      if ((busy_th_num*2) < live_th_num  &&  live_th_num > pool->min_th_num)
      {
         // printf("admin busy --%d--%d----\n", busy_thr_num, live_thr_num);
         /*一次销毁DEFAULT_THREAD_NUM个线程*/
         pthread_mutex_lock(&(pool->lock));
         pool->wait_exit_th_num = DEFAULT_THREAD_NUM;
         pthread_mutex_unlock(&(pool->lock));

         for (i=0; i<DEFAULT_THREAD_NUM; i++)
        {
           //通知正在处于空闲的线程，自杀
           pthread_cond_signal(&(pool->queue_not_empty));
         //   printf("admin cler --\n");
        }
      }

   }

   pthread_exit(NULL);

   // return NULL;
}


/*线程是否存活*/
int is_thread_alive(pthread_t tid)
{
   int kill_rc = pthread_kill(tid, 0);     //发送0号信号，测试是否存活
   if (kill_rc == ESRCH)  //线程不存在
   {
      return false;
   }
   return true;
}

/*释放线程池*/
int threadpool_free(thread_pool_t *pool)
{
   if (pool == NULL)
     return -1;
   if (pool->task_queue)
      free(pool->task_queue);
   if (pool->threads)
   {
      free(pool->threads);
      pthread_mutex_lock(&(pool->lock));               /*先锁住再销毁*/
      pthread_mutex_destroy(&(pool->lock));
      pthread_mutex_lock(&(pool->thread_counter));
      pthread_mutex_destroy(&(pool->thread_counter));
      pthread_cond_destroy(&(pool->queue_not_empty));
      pthread_cond_destroy(&(pool->queue_not_full));
   }
   free(pool);
   pool = NULL;
   return 0;
}

/*销毁线程池*/
int threadpool_destroy(thread_pool_t *pool)
{
   int i;
   if (pool == NULL)
   {
     return -1;
   }
   pool->shutdown = true;

   /*销毁管理者线程*/
   pthread_join(pool->admin_tid, NULL);

   // 通知所有线程去自杀(在自己领任务的过程中) 如果仅仅使用一次broadcast 
   // 如果有一个任务正在运行中，其不处在wait状态，这会造成下方阻塞，等主线程结束后系统自动回收这些线程
   for (i=0; i<pool->live_th_num; i++)
   {
      pthread_cond_broadcast(&(pool->queue_not_empty));
   }

   /*等待线程结束 先是pthread_exit 然后等待其结束*/
   for (i=0; i<pool->live_th_num; i++)
   {
     pthread_join(pool->threads[i], NULL);
   }

   threadpool_free(pool);
   return 0;
}