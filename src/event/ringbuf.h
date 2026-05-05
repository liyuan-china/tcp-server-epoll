#ifndef RINGBUF_H
#define RINGBUF_H

#include <pthread.h>

/**
 *  @brief 环形缓冲区结构体
 * 
 * 采用 head + tail + count 三变量方案，避免空满歧义
 */
typedef struct ringbuf_t
{
    char *buf;
    int head;
    int tail;
    int capacity;
    int count;

} ringbuf_t;


/**
 * @brief 初始化环形缓冲区
 * 
 * 为环形缓冲区分配指定大小的内存，并将读写指针及计数归零
 * 
 * @param rb       指向待初始化的缓冲区结构体
 * @param capacity 缓冲区最大字节数
 * @return         成功返回 0
 *                 失败返回 1
 */
int ringbuf_init(ringbuf_t *rb, int capacity);

/**
 * @brief 销毁环形缓冲区，释放已分配的内存
 * 
 * 调用后缓冲区不可再用，除非重新ringbuf_init
 * 
 * @param rb 指向已初始化的环形缓冲区
 */
void ringbuf_destroy(ringbuf_t *rb);

/**
 * @brief 写入数据到环形缓冲区
 * 
 * @param rb     指向缓冲区
 * @param data   源数据指针
 * @param len    期望写入字节数
 * @return       实际写入字节数(成功时等于len)，失败返回-1
 */
int ringbuf_write(ringbuf_t *rb, const char *data, int len);

/**
 * @brief 查看缓冲区数据（不消费）
 * 
 * 与ringbuf_read读取逻辑相同，但数据保留在缓冲区中。
 * 
 * @param rb     环形缓冲区指针
 * @param out    输出缓冲区地址
 * @param len    期望查看字节数
 * @return       实际可查看字节数
 */
int ringbuf_peek(ringbuf_t *rb, char *out, int len);

/**
 * @brief 从环形缓冲区中读取数据（消费）
 * 
 * 读取后数据从缓冲区删除。
 * 
 * @param rb     环形缓冲区指针
 * @param out    输出缓冲区地址
 * @param len    期望读取字节数
 * @return       实际读取字节数
 */
int ringbuf_read(ringbuf_t *rb, char *out, int len);

/**
 * @brief 丢弃缓冲区开头的若干字节（消费）
 * 
 * 不拷贝数据，仅移动读指针
 * 
 * @param rb 环形缓冲区指针
 * @param len 期望丢弃字节数
 * @return    实际丢弃字节数
 */
int ringbuf_skip(ringbuf_t *rb, int len);

/**
 * @brief 返回缓冲区中已存储的字节数
 */
static inline int ringbuf_used(ringbuf_t *rb){
    return rb->count;
}

/**
 * @brief 返回缓冲区剩余可用空间（字节数）
 */
static inline int ringbuf_free_space(ringbuf_t *rb){
    return rb->capacity - rb->count;
}

/**
 * @brief 清空缓冲区，保留已分配内存
 * 
 * 读写指针和计数归零，内存不释放，可立即复用
 * 
 * @param rb  环形缓冲区指针
 */
void ringbuf_reset(ringbuf_t *rb);

#endif
