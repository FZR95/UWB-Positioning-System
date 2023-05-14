#ifndef __SPI_H
#define __SPI_H
#include "sys.h"

void SPI1_Init(void);
void SPI1_SetSpeed(unsigned char SpeedSet);
unsigned char SPI1_ReadWriteByte(unsigned char TxData);

#endif
