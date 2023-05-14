#include <stdio.h>
#include <string.h>
#include <math.h>
#include "DW1000.h"
#include "delay.h"
#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"
#include "lcd.h"
#include "Led.h"
#include "port.h"

// #define _IS_IRQ_ENABLED 1

//****************************************************************************************************
//*
//*  MAIN
//*
//****************************************************************************************************
int dw_init(void)
{
    spi_peripheral_init(); // DECA1000 uses SPI1 on PortA
    //* MISC PIN CONFIGURATIONs *//
    GPIO_ResetBits(DECA_WAKEUP_GPIO, DECA_WAKEUP); // Unused WAKEUP pin
    //port_set_deca_isr(dwt_isr);                    // Install DW1000 IRQ handler.
    /* IO CONFIGURATIONs */
    //GPIO_ResetBits(DECA_IO5_GPIO, DECA_IO5); //SPI MODE 1
    //GPIO_SetBits(DECA_IO6_GPIO, DECA_IO6);   //SPI MODE 1

    printf("hello dwm1000!\n");

    reset_DW1000();     // Reset and initialise DW1000. Target specific drive of RSTn line into DW1000 low for a period.
    spi_set_rate_low(); // For initialisation, DW1000 clocks must be temporarily set to crystal speed. After initialisation SPI rate can be increased for optimum performance.

    if (dwt_initialise(DWT_LOADUCODE) == DWT_ERROR)
    {
        printf("dwm1000 init fail! 0x%x\n",dwt_readdevid());
        while (1)
        {
            LCD_ShowString(20,100,300,24,24,"DWM1000 FAILED!");
            GPIO_SetBits(LED_PORT, LED3_GPIO);
            GPIO_ResetBits(LED_PORT, LED2_GPIO);
            delay_ms(500);
            GPIO_SetBits(LED_PORT, LED2_GPIO);
            GPIO_ResetBits(LED_PORT, LED3_GPIO);
            delay_ms(500);
        };
    }

    spi_set_rate_high();

    printf("Ready:%x\n", dwt_readdevid());

#ifdef _IS_IRQ_ENABLED
    /* Enable wanted interrupts (TX confirmation, RX good frames, RX timeouts and RX errors). */
    dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | DWT_INT_RFTO | DWT_INT_RXPTO | DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFSL | DWT_INT_SFDT, 1);
#endif
    return 1;
}
