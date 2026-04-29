#ifndef THREADPOOL_H
#define THREADPOOL_H

typedef struct task_node_t task_node_t;
typedef struct pthread_t;
typedef void (*task_func_t)(void *arg);

void *worker_thread(void *arg);
threadpool_t *threadpool_create(int count_threads, int queue_size);
int threadpool_task(threadpool_t *pool, task_func_t func, void *arg);
void threadpool_destroy(threadpool_t *pool);

typedef struct task_node_t
{
    task_func_t func;
    void *arg;
    struct task_node_t *next;
    /* data */
}task_node_t;


typedef struct threadpool_t
{
    //管资源
    pthread_t *threads;//线程ID数组
    int count_threads;//线程个数 

    //数据传输
    task_node_t *head; //头部
    task_node_t *tail;  //尾部
    int task_count;  //任务计数

    //同步
    pthread_mutex_t mutex; //锁
    pthread_cond_t cond_not_empty; //条件变量
    pthread_cond_t cond_not_full; //条件变量

    //初始化和退出
    int queue_size; //队列上限
    int shutdown; //销毁标志

    /* data */
}threadpool_t;


#endif