#include "delay.h"
#include "Bmp.h"
#include "OLED_SPI.h"

// void OLED_WrCmd(unsigned char SPI_Data)
// {
// 	DC_OLED_SPI = 0;
// 	CS_OLED_SPI = 0;
// 	SPI2_ReadWriteByte(SPI_Data);
// 	CS_OLED_SPI = 1;
// }

// void OLED_WrDat(unsigned char SPI_Data)
// {
// 	DC_OLED_SPI = 1;
// 	CS_OLED_SPI = 0;
// 	SPI2_ReadWriteByte(SPI_Data);
// 	CS_OLED_SPI = 1;
// }

void OLED_WrCmd(unsigned char SPI_Data)
{
	unsigned char i = 8;
	DC_OLED_SPI = 0;
	CS_OLED_SPI = 0;
	SCLK_OLED_SPI = 0;
	//asm("nop");
	while (i--)
	{
		if (SPI_Data & 0x80)
		{
			SDA_OLED_SPI = 1;
		}
		else
		{
			SDA_OLED_SPI = 0;
		}
		SCLK_OLED_SPI = 1;
		SCLK_OLED_SPI = 0;
		SPI_Data <<= 1;
	}
	CS_OLED_SPI = 1;
}

void OLED_WrDat(unsigned char SPI_Data)
{
	unsigned char i = 8;
	DC_OLED_SPI = 1;
	CS_OLED_SPI = 0;
	SCLK_OLED_SPI = 0;
	while (i--)
	{
		if (SPI_Data & 0x80)
		{
			SDA_OLED_SPI = 1;
		}
		else
		{
			SDA_OLED_SPI = 0;
		}
		SCLK_OLED_SPI = 1;
		SCLK_OLED_SPI = 0;
		SPI_Data <<= 1;
	}
	CS_OLED_SPI = 1;
}

void OLED_Set_Pos(unsigned char x, unsigned char y)
{
	OLED_WrCmd(0xb0 + y);
	OLED_WrCmd(((x & 0xf0) >> 4) | 0x10);
	OLED_WrCmd((x & 0x0f));
}

void OLED_Fill(unsigned char dat)
{
	unsigned char y, x;
	for (y = 0; y < 8; y++)
	{
		OLED_WrCmd(0xb0 + y);
		OLED_WrCmd(0x00);
		OLED_WrCmd(0x10);
		for (x = 0; x < X_WIDTH; x++)
			OLED_WrDat(dat);
	}
}

void OLED_Fill_Part(unsigned char dat, unsigned char x0, unsigned char y0, unsigned char length, unsigned char height)
{
	unsigned char i, j;
	OLED_Set_Pos(x0, y0);
	for (j = 0; j < height; j++)
	{
		OLED_Set_Pos(x0, y0 + j);
		for (i = 0; i < length; i++)
		{
			OLED_WrDat(dat);
		}
	}
}

void OLED_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_SetBits(GPIOB, GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_15);

	// SPI2_Init();
	// SPI2_SetSpeed(SPI_BaudRatePrescaler_256);
	RES_OLED_SPI = 1;
	delay_ms(100);
	RES_OLED_SPI = 0;
	delay_ms(100);
	RES_OLED_SPI = 1;

	OLED_WrCmd(0xAE);		//--turn off oled panel
	OLED_WrCmd(0x00);		//---set low column address
	OLED_WrCmd(0x10);		//---set high column address
	OLED_WrCmd(0x40);		//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
	OLED_WrCmd(0x81);		//--set contrast control register_
	OLED_WrCmd(Brightness); // Set SEG Output Current Brightness
	OLED_WrCmd(0xA1);		//--Set SEG/Column Mapping     0xa0左右反置 0xa1正常
	OLED_WrCmd(0xC8);		//Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
	OLED_WrCmd(0xA6);		//--set normal display
	OLED_WrCmd(0xA8);		//--set multiplex ratio(1 to 64)
	OLED_WrCmd(0x3f);		//--1/64 duty
	OLED_WrCmd(0xD3);		//-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
	OLED_WrCmd(0x00);		//-not offset
	OLED_WrCmd(0xd5);		//--set display clock divide ratio/oscillator frequency
	OLED_WrCmd(0x80);		//--set divide ratio, Set Clock as 100 Frames/Sec
	OLED_WrCmd(0xD9);		//--set pre-charge period
	OLED_WrCmd(0xF1);		//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	OLED_WrCmd(0xDA);		//--set com pins hardware configuration
	OLED_WrCmd(0x12);		//
	OLED_WrCmd(0xDB);		//--set vcomh
	OLED_WrCmd(0x40);		//Set VCOM Deselect Level
	OLED_WrCmd(0x20);		//-Set Page Addressing Mode (0x00/0x01/0x02)
	OLED_WrCmd(0x02);		//
	OLED_WrCmd(0x8D);		//--set Charge Pump enable/disable
	OLED_WrCmd(0x14);		//--set(0x10) disable
	OLED_WrCmd(0xA4);		// Disable Entire Display On (0xa4/0xa5)
	OLED_WrCmd(0xA6);		// Disable Inverse Display On (0xa6/a7)
	OLED_WrCmd(0xAF);		//--turn on oled panel
	OLED_Fill(0x00);
}

void OLED_Draw_RowLine(unsigned char ypos, unsigned char x0, unsigned char xx)
{
	unsigned char i;
	OLED_Set_Pos(x0, ypos);
	for (i = 0; i < xx; i++)
	{
		OLED_WrDat(0x01);
	}
}

void OLED_Draw_ColLine(unsigned char xpos, unsigned char y0, unsigned char yx)
{
	unsigned char i;
	OLED_Set_Pos(xpos, y0);
	for (i = 0; i < yx; i++)
	{
		OLED_WrDat(0xFF);
		OLED_Set_Pos(xpos, y0 + i);
	}
}

void OLED_Draw_Singlecol(unsigned char x, unsigned char y, char dat)
{
	OLED_Set_Pos(x, y);
	OLED_WrDat(dat);
}

void OLED_DRAW_Dat(unsigned char x0, unsigned char y0, const unsigned char dat[], unsigned char width)
{
	unsigned char i;
	OLED_Set_Pos(x0, y0);
	for (i = 0; i < width; i++)
	{
		OLED_WrDat(dat[i]);
	}
}

void OLED_Draw_Chars(unsigned char x0, unsigned char y0, const unsigned char dat[], unsigned char width)
{
	unsigned char i;
	OLED_Set_Pos(x0, y0);
	for (i = 0; i < width; i++)
	{
		OLED_WrDat(dat[i]);
	}
	OLED_Set_Pos(x0, y0 + 1);
	for (i = 0; i < width; i++)
	{
		OLED_WrDat(dat[i + width]);
	}
}

void OLED_Draw_Multi(unsigned char x0, unsigned char y0, unsigned char length, unsigned char height, const unsigned char dat[])
{
	unsigned char i, j;
	OLED_Set_Pos(x0, y0);
	for (j = 0; j < height; j++)
	{
		OLED_Set_Pos(x0, y0 + j);
		for (i = 0; i < length; i++)
		{
			OLED_WrDat(dat[i + j * length]);
		}
	}
}
