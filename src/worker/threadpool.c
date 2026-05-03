#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <worker/threadpool.h>

/**
 * @brief 任务队列的节点结构体
 */
typedef struct task_node_t
{
    task_func_t func;
    void *arg;
    struct task_node_t *next;
    /* data */
}task_node_t;

/**
 * pthread_create()第三个参数:void *(*__start_routine)(void *)
 * 工作线程的入口函数
 * 每个工作线程都会运行该函数，从任务队列中取出任务并执行
 * 直到线程池通知退出且队列为空时结束
 * @param arg 线程池指针(threadpool_t *)
 * @return 返回NULL
 */
void *worker_thread(void *arg){

    threadpool_t *pool = (threadpool_t *)arg;
    while (1)
    {
        pthread_mutex_lock(&pool->mutex);

        //等待任务或线程池销毁
        while (pool->task_count == 0 && !pool->shutdown)
        {
            pthread_cond_wait(&pool->cond_not_empty, &pool->mutex);
            /* code */
        }

        //退出条件：线程池销毁且无任务
        if (pool->shutdown && pool->task_count == 0)
        {
            pthread_mutex_unlock(&pool->mutex);
            break;
            /* code */
        }

        // 从队列头部取出一个任务
        task_node_t *task = pool->head;
        pool->head = task->next;
        pool->task_count--;
        if (pool->head == NULL){
            pool->tail = NULL;
        }
        // 通知生产者（若有等待队列非满的线程）
        pthread_cond_signal(&pool->cond_not_full);
        pthread_mutex_unlock(&pool->mutex);

        //执行任务（锁外）
        task->func(task->arg);

        free(task);
        /* code */
    }
    return NULL;
}

/**
 * 线程池创建
 * @param count_threads 工作线程数量
 * @param queue_size    任务队列最大长度(0表示无限制)
 * @return 成功返回线程池指针，失败返回NULL
 */
threadpool_t *threadpool_create(int count_threads, int queue_size){
    //动态分配线程池结构体本身的内存
    threadpool_t *pool = malloc(sizeof(threadpool_t));
    if (pool == NULL)
    {
        return NULL;
        /* code */
    }

    //参数值和初始状态写入结构体成员
    pool->count_threads = count_threads;
    pool->queue_size = queue_size;
    pool->head = NULL;
    pool->tail = NULL;
    pool->task_count = 0;
    pool->shutdown = 0;

    // 指针，存放线程ID数组的起始地址
    pool->threads = malloc(sizeof(pthread_t) * count_threads);
    if (pool->threads == NULL)
    {
        free(pool);
        return NULL;
        /* code */
    }

    /**
     * 初始化锁和条件变量
     * 1. mutex 失败 → 释放 threads + pool
     * 2. cond_not_empty 失败 → 销毁 mutex + 释放 threads + pool
     * 3. cond_not_full 失败 → 销毁 cond_not_empty + 销毁 mutex + 释放 threads + pool
     *  
     */
    if (pthread_mutex_init(&pool->mutex, NULL) != 0)
    {
        free(pool->threads);
        free(pool);
        return NULL;
        /* code */
    }
    
    if (pthread_cond_init(&pool->cond_not_empty, NULL) != 0)
    {
        pthread_mutex_destroy(&pool->mutex);
        free(pool->threads);
        free(pool);
        return NULL;
        /* code */
    }
    
    if (pthread_cond_init(&pool->cond_not_full, NULL) != 0)
    {
        pthread_mutex_destroy(&pool->mutex);
        pthread_cond_destroy(&pool->cond_not_empty);
        free(pool->threads);
        free(pool);
        return NULL;
        /* code */
    }
    
    // 创建N个线程，给每个线程传入work_thread函数和线程池指针
    for (int i = 0; i < count_threads; i++)
    {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0)
        {
            pool->shutdown = 1;
            pthread_cond_broadcast(&pool->cond_not_empty);

            for (int j = 0; j < i; j++)
            {
                pthread_join(pool->threads[j], NULL);
                /* code */
            }
            pthread_mutex_destroy(&pool->mutex);
            pthread_cond_destroy(&pool->cond_not_empty);
            pthread_cond_destroy(&pool->cond_not_full);
            free(pool->threads);
            free(pool);

            return NULL;
        }
        /* code */
    }

    return pool;
}

/**
 * 向任务池投递一个任务 主线程侧
 * @param pool 线程池指针
 * @param func 要执行的任务函数
 * @param arg 传入任务函数的参数
 * @return 成功返回0，线程池正在销毁或内存分配失败返回-1
 */
int threadpool_task(threadpool_t *pool, task_func_t func, void *arg){
    pthread_mutex_lock(&pool->mutex);

    while (pool->queue_size > 0 &&
           pool->task_count >= pool->queue_size &&
           !pool->shutdown)
    {
        pthread_cond_wait(&pool->cond_not_full, &pool->mutex);
    }

    if (pool->shutdown)
    {
        pthread_mutex_unlock(&pool->mutex);
        return -1;
    }
    
    //构造新任务节点，并插入队列
    task_node_t *new_node = (task_node_t *)malloc(sizeof(task_node_t));
    if (new_node == NULL)
    {
        pthread_mutex_unlock(&pool->mutex);
        return -1;
    }

    new_node->func = func;
    new_node->arg = arg;
    new_node->next = NULL;

    if (pool->tail == NULL)
    {
        pool->head = pool->tail = new_node;
    }else{
        pool->tail->next = new_node;
        pool->tail = new_node;
    }
    
    pool->task_count++;
    pthread_cond_signal(&pool->cond_not_empty);
    pthread_mutex_unlock(&pool->mutex);

    return 0;

}

/**
 * 销毁线程池，等待所有线程退出并释放所有资源
 * @param pool 线程池指针
 * @return 无返回值 (void)                   
 */
void threadpool_destroy(threadpool_t *pool){
    if (pool == NULL)
    {
        return;
    }
    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = 1;

    pthread_cond_broadcast(&pool->cond_not_empty);
    pthread_cond_broadcast(&pool->cond_not_full);
    pthread_mutex_unlock(&pool->mutex);

    for (int i = 0; i < pool->count_threads; i++)
    {
        pthread_join(pool->threads[i], NULL);
    }
    
    // 遍历并释放任务队列中残留任务节点,防止内存泄漏
    task_node_t *current = pool->head;
    while (current != NULL)
    {
        task_node_t *next = current->next;
        free(current);
        current = next;
        /* code */
    }
    
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond_not_empty);
    pthread_cond_destroy(&pool->cond_not_full);
    free(pool->threads);
    free(pool);

}
