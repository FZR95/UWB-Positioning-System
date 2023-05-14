#include "SPI.h"

void SPI1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    //  GPIO Init
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_SetBits(GPIOA, GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7);

    //  SPI Init
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;   //Direction
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;                        //Master or Slave
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;                    //DataSize
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;                           //SPI Mode Set
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;                         //
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;                            //NSS信号由硬件（NSS管脚）还是软件（使用SSI位）管理:内部NSS信号有SSI位控制
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256; //Baud Rate
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;                   //FirstBit
    SPI_InitStructure.SPI_CRCPolynomial = 7;                             //CRC值计算的多项式
    SPI_Init(SPI1, &SPI_InitStructure);                                  //根据SPI_InitStruct中指定的参数初始化外设SPIx寄存器

    SPI_Cmd(SPI1, ENABLE); // Enable

    SPI1_ReadWriteByte(0xff);
}

//SPI 速度设置函数
//SpeedSet:
//SPI_BaudRatePrescaler_2   2分频
//SPI_BaudRatePrescaler_8   8分频
//SPI_BaudRatePrescaler_16  16分频
//SPI_BaudRatePrescaler_256 256分频

void SPI1_SetSpeed(unsigned char SPI_BaudRatePrescaler)
{
    assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));
    SPI1->CR1 &= 0XFFC7;
    SPI1->CR1 |= SPI_BaudRatePrescaler; //设置SPI2速度
    SPI_Cmd(SPI1, ENABLE);
}

//SPIx 读写一个字节
//TxData:要写入的字节
//返回值:读取到的字节
unsigned char SPI1_ReadWriteByte(unsigned char TxData)
{
    u8 retry = 0;
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) //检查指定的SPI标志位设置与否:发送缓存空标志位
    {
        retry++;
        if (retry > 200)
            return 0;
    }
    SPI_I2S_SendData(SPI1, TxData); //通过外设SPIx发送一个数据
    retry = 0;

    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) //检查指定的SPI标志位设置与否:接受缓存非空标志位
    {
        retry++;
        if (retry > 200)
            return 0;
    }
    return SPI_I2S_ReceiveData(SPI1); //返回通过SPIx最近接收的数据
}
