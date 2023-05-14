#ifndef __USART3_H
#define __USART3_H

#include "stm32f4xx_conf.h"
#include "sys.h"

#define USART3_REC_LEN 200 //定义最大接收字节数 200

extern u8 USART3_RX_BUF[USART3_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符
extern u16 USART3_RX_STA;               //接收状态标记

void uart3_init(u32 bound);
void printf3(unsigned char *Data);
#endif
