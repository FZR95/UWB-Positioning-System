#include "sys.h"
#include "string.h"
#include "usart3.h"

u8 USART3_RX_BUF[USART3_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART3_RX_STA = 0; //接收状态标记
//! PB10 - Tx; PB11 - Rx
void uart3_init(u32 bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);  //使能GPIOA时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE); //使能USART1时钟

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART3); //GPIOA9复用为USART1
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART3); //GPIOA10复用为USART1

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11; //GPIOA9与GPIOA10
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;             //复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;        //速度50MHz
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;           //推挽复用输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;             //上拉
    GPIO_Init(GPIOB, &GPIO_InitStructure);                   //初始化PA9，PA10

    USART_InitStructure.USART_BaudRate = bound;                                     //波特率设置
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;                     //字长为8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1;                          //一个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;                             //无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;                 //收发模式
    USART_Init(USART3, &USART_InitStructure);                                       //初始化串口1

    USART_Cmd(USART3, ENABLE);
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);            //开启串口接受中断
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;         //串口1中断通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; //抢占优先级3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;        //子优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           //IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);                           //根据指定的参数初始化VIC寄存器、

    USART_ClearFlag(USART3, USART_FLAG_TC);
}

void printf3(unsigned char *Data)
{
    int i;
    for (i = 0; i < strlen(Data); i++)
    {
        while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET)
            ;
        USART_SendData(USART3, Data[i]);
    }
    while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET)
        ;
}

void USART3_IRQHandler(void) //串口1中断服务程序
{
    u8 Res;
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) //接收中断(接收到的数据必须是0x0d 0x0a结尾)
    {
        Res = USART_ReceiveData(USART3); //(USART1->DR);	//读取接收到的数据

        if ((USART3_RX_STA & 0x8000) == 0) //接收未完成
        {
            if (USART3_RX_STA & 0x4000) //接收到了0x0d
            {
                if (Res != 0x0a)
                    USART3_RX_STA = 0; //接收错误,重新开始
                else
                    USART3_RX_STA |= 0x8000; //接收完成了
            }
            else //还没收到0X0D
            {
                if (Res == 0x0d)
                    USART3_RX_STA |= 0x4000;
                else
                {
                    USART3_RX_BUF[USART3_RX_STA & 0X3FFF] = Res;
                    USART3_RX_STA++;
                    if (USART3_RX_STA > (USART3_REC_LEN - 1))
                        USART3_RX_STA = 0; //接收数据错误,重新开始接收
                }
            }
        }
    }
}
