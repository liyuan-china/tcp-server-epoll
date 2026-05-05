#include <stdlib.h>
#include "frame.h"

/**
 * @brief 从环形缓冲区提取一帧
 * 
 * peek 预读头部，完整帧到达后才消费
 * 帧头错误时丢弃1字节尝试重对齐，成功时malloc载荷内存
 */
int frame_try_extract(ringbuf_t *rb, char **payload, uint16_t *payload_len){
    //1.参数保护
    if (payload == NULL || payload_len == NULL)
    {
        return -1;
    }
    *payload = NULL;
    *payload_len = 0;

    //2.检查头部完整性
    if (ringbuf_used(rb) < FRAME_HEADER_SIZE)
    {
        return 1;
    }

    //3.偷看头部
    unsigned char header[FRAME_HEADER_SIZE];
    ringbuf_peek(rb, (char *)header, FRAME_HEADER_SIZE);

    //4.检查帧头魔数
    if (header[0] != FRAME_MAGIC_0 || header[1] != FRAME_MAGIC_1)
    {
        ringbuf_skip(rb, 1);
        return -1;
    }

    // 5.读取载荷长度
    uint16_t data_len = ((uint16_t)header[2] << 8) | header[3];

    // 6.检查载荷完整性
    if (ringbuf_used(rb) < FRAME_HEADER_SIZE + data_len)
    {
        return 1;
    }

    //7. 提取完整帧
    ringbuf_skip(rb, FRAME_HEADER_SIZE);
    char *buf = (char *)malloc(data_len);
    if (buf == NULL)
    {
        return -1;
    }
    ringbuf_read(rb, buf, data_len);
    *payload = buf;
    *payload_len = data_len;

    return 0;
}