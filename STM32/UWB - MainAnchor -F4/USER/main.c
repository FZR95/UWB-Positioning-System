#include "sys.h"
#include "UI.h"
#include "twr.h"
#include "port.h"
#include "trilateration_basic.h"
#include "bsp_spi_flash.h"
#include "ESP8266.h"
#include "usart.h"
#include "usart3.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	peripherals_init();
	
	//* initialize Anchor position */ /
	char i;
	unsigned long FLASH_ANCHOR_ADDR = 0x010000;
	unsigned int FLASH_ANCHOR_IDX = 0;
	unsigned char flashbuffer[7];
	// read positions from flash first
	FLASH_ANCHOR_IDX = 0;
	for (i = 0; i < 3; i++)
	{
		SPI_FLASH_BufferRead(&flashbuffer, FLASH_ANCHOR_ADDR + FLASH_ANCHOR_IDX, sizeof(flashbuffer));
		Anchor_vec[i].x = atof(flashbuffer);
		FLASH_ANCHOR_IDX += sizeof(flashbuffer);
		SPI_FLASH_BufferRead(&flashbuffer, FLASH_ANCHOR_ADDR + FLASH_ANCHOR_IDX, sizeof(flashbuffer));
		Anchor_vec[i].y = atof(flashbuffer);
		FLASH_ANCHOR_IDX += sizeof(flashbuffer);
	}

	UI_Opts_Position();
	// write to flash
	SPI_FLASH_SectorErase(FLASH_ANCHOR_ADDR);
	FLASH_ANCHOR_IDX = 0;
	for (i = 0; i < 3; i++)
	{
		sprintf(flashbuffer, "%4.2f", Anchor_vec[i].x);
		SPI_FLASH_BufferWrite(&flashbuffer, FLASH_ANCHOR_ADDR + FLASH_ANCHOR_IDX, sizeof(flashbuffer));
		FLASH_ANCHOR_IDX += sizeof(flashbuffer);
		sprintf(flashbuffer, "%4.2f", Anchor_vec[i].y);
		SPI_FLASH_BufferWrite(&flashbuffer, FLASH_ANCHOR_ADDR + FLASH_ANCHOR_IDX, sizeof(flashbuffer));
		FLASH_ANCHOR_IDX += sizeof(flashbuffer);
	}
	Anchor_vec[0].z = 0;
	Anchor_vec[1].z = 80;
	Anchor_vec[2].z = 80;
	//* show Anchor position *//
	UI_Init();
	//Esp8266_Init();
	//Esp8266_IOT();
	//IOT_Pulish_Anchorpos();
	ds_twr_initiator();
	while (1)
		;
}
