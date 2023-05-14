#ifndef __OLEDSPI_H
#define __OLEDSPI_H
#include "sys.h"

#define Brightness 0xCF
#define OLEDAddr 0x78
#define X_WIDTH 128
#define Y_WIDTH 64

////////////////////////////
//  File Name : OLED_SPI
//  Description : Drive OLED screen using STM32F103 software SPI.
//  Init Date : 2020.11
//  Last Change : 2020.11.21
//  Version : 1.0
//  Made by FZR
////////////////////////////
//  D0 - SCLK
//  D1 - MOSI
////////////////////////////

#define SCLK_OLED_SPI PBout(13)
#define SDA_OLED_SPI PBout(15)
#define RES_OLED_SPI PBout(9)
#define DC_OLED_SPI PBout(8)
#define CS_OLED_SPI PBout(12)


void OLED_Init(void);
void OLED_Fill(unsigned char dat);
void OLED_Fill_Part(unsigned char dat, unsigned char x0, unsigned char y0, unsigned char length, unsigned char height);
void OLED_Draw_RowLine(unsigned char ypos, unsigned char x0, unsigned char xx);
void OLED_Draw_ColLine(unsigned char xpos, unsigned char y0, unsigned char yx);
void OLED_DRAW_Dat(unsigned char x0, unsigned char y0, const unsigned char dat[], unsigned char width);
void OLED_Draw_Chars(unsigned char x0, unsigned char y0, const unsigned char dat[], unsigned char width);
void OLED_Draw_Multi(unsigned char x0, unsigned char y0, unsigned char xx, unsigned char yx, const unsigned char dat[]);

#endif
