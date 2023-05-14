#ifndef __USART_H
#define __USART_H
#include "stdio.h"
#include "sys.h"

#define USART_REC_LEN 300
#define EN_USART1_RX 1 //使能（1）/禁止（0）串口1接收

extern u8 USART_RX_BUF[USART_REC_LEN];
extern u8 ReceiveLen;
extern u8 EN_Receive;

void uart_init(u32 bound);
#endif
