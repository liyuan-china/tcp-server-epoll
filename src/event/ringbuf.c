#include <stdlib.h>
#include <string.h>
#include "ringbuf.h"

static int min_int(int a, int b)
{
    return a < b ? a : b;
}

/**
 * @brief 初始化环形缓冲区
 * 
 * 分配capacity 字节内存，并将读写指针和计数归零
 * 
 * @return          成功返回0，失败返回-1
 */
int ringbuf_init(ringbuf_t *rb, int capacity){
    if (rb == NULL || capacity <= 0)
    {
        return -1;
    }
    
    rb->buf = malloc(capacity);
    if (rb->buf == NULL)
    {
        return -1;
    }

    rb->head = 0;
    rb->tail = 0;
    rb->capacity = capacity;
    rb->count = 0;

    return 0;
}


/**
 * @brief 销毁环形缓冲区
 * 
 * 释放buf内存，所有字段归零
 * 可安全传入NULL
 */
void ringbuf_destroy(ringbuf_t *rb){
    if (rb == NULL)
    {
        return;
    }
    free(rb->buf);
    rb->buf = NULL;
    rb->head = rb->tail = 0;
    rb->capacity = rb->count = 0;
}


/**
 * @brief 向环形缓冲区写入数据
 * 
 * 分两段拷贝：从head到数组末尾，剩余绕回开头
 * 空间不足返回-1，不覆盖旧数据 
 */
int ringbuf_write(ringbuf_t *rb, const char *data, int len){
    if(rb == NULL || data == NULL || len <= 0){
        return -1;

    }
    if (len > ringbuf_free_space(rb)) {
        return -1;
    }

    size_t first = min_int(len, rb->capacity - rb->head);
    memcpy(rb->buf + rb->head, data, first);
    
    size_t second = len - first;
    if (second > 0)
    {
        memcpy(rb->buf, data +first, second);
    }

    rb->head = (rb->head + len) % rb->capacity;
    rb->count += len;
    return len;
}

/**
 * @brief 查看缓冲区数据（不消费）
 * 
 */
int ringbuf_peek(ringbuf_t *rb, char *out, int len){
    if (rb == NULL || out == NULL || len <= 0) {
        return 0;
    }
    if (len > ringbuf_used(rb))
    {
        return rb->count; //限制为实际可用数据量
    }

    size_t first = min_int(len, rb->capacity - rb->tail);
    memcpy(out, rb->buf + rb->tail, first);

    size_t second = len - first;
    if (second > 0)
    {
        memcpy(out + first, rb->buf, second);
    }

    return len;
}

/**
 * @brief 从环形缓冲区读取数据（消费）
 * 
 * 分两段拷贝：从tail到数组末尾，剩余绕回开头
 * 实际读取量 = min(len, count), 读后移动tail并减少count
 */
int ringbuf_read(ringbuf_t *rb, char *out, int len){
    if (rb == NULL || out == NULL || len <= 0) {
        return 0;
    }
    if (len > ringbuf_used(rb))
    {
        len = rb->count;
    }

    size_t first = min_int(len, rb->capacity - rb->tail);
    memcpy(out, rb->buf + rb->tail, first);
    
    size_t second = len - first;
    if (second > 0)
    {
        memcpy(out + first, rb->buf, second);
    }

    rb->tail = (rb->tail + len) % rb->capacity;
    rb->count -= len;
    return len;
}

/**
 * @brief 丢弃缓冲区开头的若干字节
 * 
 * 仅移动tail并减少count，不拷贝数据
 * 用于帧头错位逐字节丢弃或跳过已处理的帧头
 */
int ringbuf_skip(ringbuf_t *rb, int len){
    if (rb == NULL || len <= 0) {
        return 0;
    }
    if (len > ringbuf_used(rb))
    {
        return rb->count;
    }

    rb->tail = (rb->tail + len) % rb->capacity;
    rb->count -= len;
    return len;
}

/**
 * @brief 清空缓冲区，保留已分配内存
 * 
 * head/tail/count归零，buf和capacity不变
 * 用于帧头连续错误时快速清空脏数据，避免反复malloc/free
 */
void ringbuf_reset(ringbuf_t *rb){
    if (rb == NULL)
    {
        return;
    }
    rb->head = rb->tail = 0;
    rb->count = 0;
}





