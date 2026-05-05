#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>
#include "ringbuf.h"

#define FRAME_MAGIC_0      0xAB
#define FRAME_MAGIC_1      0xCD
#define FRAME_HEADER_SIZE  4

/**
 * @brief 从环形缓冲区提取一帧完整数据
 * 
 * @param rb          环形缓冲区指针        
 * @param payload     载荷数据地址
 * @param payload_len 载荷字节数
 * 
 * @retval 0   成功提取一帧
 * @retval 1   数据不足，等待更多字节
 * @retval -1  帧头错误，已跳过1字节重对齐
 */
int frame_try_extract(ringbuf_t *rb, char **payload, uint16_t *payload_len);

#endif