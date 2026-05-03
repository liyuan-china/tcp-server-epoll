#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

typedef struct task_node_t task_node_t;
//typedef struct pthread_p;
typedef void (*task_func_t)(void *arg);


/**
 * @brief 线程池控制结构体
 * 
 * 包含工作线程数组、任务队列】同步对象及运行状态。
 * 所有对任务队列的访问必须在持有mutex的前提下进行
 */
typedef struct threadpool_t
{
    // 管资源
    pthread_t *threads; // 线程ID数组
    int count_threads;  // 线程个数

    // 数据传输
    task_node_t *head; // 头部
    task_node_t *tail; // 尾部
    int task_count;    // 任务计数

    // 同步
    pthread_mutex_t mutex;         // 锁
    pthread_cond_t cond_not_empty; // 条件变量
    pthread_cond_t cond_not_full;  // 条件变量

    // 初始化和退出
    int queue_size; // 队列上限
    int shutdown;   // 销毁标志

    /* data */
}threadpool_t;


void *worker_thread(void *arg);
threadpool_t *threadpool_create(int count_threads, int queue_size);
int threadpool_task(threadpool_t *pool, task_func_t func, void *arg);
void threadpool_destroy(threadpool_t *pool);


#endif