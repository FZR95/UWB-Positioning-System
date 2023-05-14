/*! ----------------------------------------------------------------------------
 * @file    port.c
 * @brief   HW specific definitions and functions for portability
 *
 * @attention
 *
 * Copyright 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author DecaWave
 */
#include "deca_sleep.h"
#include "delay.h"
#include "led.h"
#include "Btn.h"
#include "usart.h"
#include "usart3.h"
#include "port.h"
#include "bsp_xpt2046_lcd.h"
#include "lcd.h"
#include "bsp_spi_flash.h"

#define rtc_init(x) RTC_Configuration(x)
#define interrupt_init(x) NVIC_Configuration(x)

/* DW1000 IRQ handler definition. */
port_deca_isr_t port_deca_isr = NULL;

int No_Configuration(void)
{
    return -1;
}

//*********************************************************************************
//!
//!  SPI
//!
//*********************************************************************************

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn spi_peripheral_init()
 *
 * @brief Initialise all SPI peripherals at once.
 *
 * @param none
 *
 * @return none
 */
void spi_peripheral_init(void)
{
    SPI_FLASH_Deinit();
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); //使能GPIOB时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);  //使能SPI1时钟

    SPI_DeInit(SPI1);

    //GPIOFB3,4,5初始化设置
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7; //PB3~5复用功能输出
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;                        //复用功能
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;                      //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;                   //100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;                        //上拉
    GPIO_Init(GPIOA, &GPIO_InitStructure);                              //初始化

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;     //复用功能
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;    //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; //100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;      //上拉
    GPIO_Init(GPIOA, &GPIO_InitStructure);            //初始化

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource4, GPIO_AF_SPI1); //PB5复用为 SPI1
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1); //PB3复用为 SPI1
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1); //PB4复用为 SPI1
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1); //PB5复用为 SPI1

    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;   //设置SPI单向或者双向的数据模式:SPI设置为双线双向全双工
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;                        //设置SPI工作模式:设置为主SPI
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;                    //设置SPI的数据大小:SPI发送接收8位帧结构
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;                           //串行同步时钟的空闲状态为高电平
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;                         //串行同步时钟的第二个跳变沿（上升或下降）数据被采样
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;                            //NSS信号由硬件（NSS管脚）还是软件（使用SSI位）管理:内部NSS信号有SSI位控制
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256; //定义波特率预分频的值:波特率预分频值为256
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;                   //指定数据传输从MSB位还是LSB位开始:数据传输从MSB位开始
    SPI_InitStructure.SPI_CRCPolynomial = 7;                             //CRC值计算的多项式
    SPI_Init(SPI1, &SPI_InitStructure);                                  //根据SPI_InitStruct中指定的参数初始化外设SPIx寄存器

    // Disable SPIx SS Output
    SPI_SSOutputCmd(SPIx, DISABLE);

    // Enable SPIx
    SPI_Cmd(SPIx, ENABLE);

    // Set CS high
    GPIO_SetBits(GPIOA, GPIO_Pin_4);
}

void SPI_ChangeRate(uint16_t scalingfactor)
{
    uint16_t tmpreg = 0;

    /* Get the SPIx CR1 value */
    tmpreg = SPIx->CR1;

    /*clear the scaling bits*/
    tmpreg &= 0xFFC7;

    /*set the scaling bits*/
    tmpreg |= scalingfactor;

    /* Write to SPIx CR1 */
    SPIx->CR1 = tmpreg;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn spi_set_rate_low()
 *
 * @brief Set SPI rate to less than 3 MHz to properly perform DW1000 initialisation.
 *
 * @param none
 *
 * @return none
 */
void spi_set_rate_low(void)
{
    SPI_ChangeRate(SPI_BaudRatePrescaler_128);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn spi_set_rate_high()
 *
 * @brief Set SPI rate as close to 20 MHz as possible for optimum performances.
 *
 * @param none
 *
 * @return none
 */
void spi_set_rate_high(void)
{
    SPI_ChangeRate(SPI_BaudRatePrescaler_4);
}

void SPI_ConfigFastRate(uint16_t scalingfactor)
{
    SPI_InitTypeDef SPI_InitStructure;

    SPI_I2S_DeInit(SPIx);

    // SPIx Mode setup
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low; //
    //SPI_InitStructure.SPI_CPOL = SPI_CPOL_High; //
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    //SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge; //
    //SPI_InitStructure.SPI_NSS = SPI_NSS_Hard;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = scalingfactor; //sets BR[2:0] bits - baudrate in SPI_CR1 reg bits 4-6
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;

    SPI_Init(SPIx, &SPI_InitStructure);

    // Enable SPIx
    SPI_Cmd(SPIx, ENABLE);
}

//*********************************************************************************
//!
//!  MISC
//!
//*********************************************************************************

void RTC_Configuration(void)
{
    // /* Enable PWR and BKP clocks */
    // RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

    // /* Allow access to BKP Domain */
    // PWR_BackupAccessCmd(ENABLE);

    // /* Reset Backup Domain */
    // BKP_DeInit();

    // /* Enable LSE */
    // RCC_LSEConfig(RCC_LSE_ON);
    // /* Wait till LSE is ready */
    // while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
    // {
    // }

    // /* Select LSE as RTC Clock Source */
    // RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

    // /* Enable RTC Clock */
    // RCC_RTCCLKCmd(ENABLE);

    // /* Wait for RTC registers synchronization */
    // RTC_WaitForSynchro();

    // /* Wait until last write operation on RTC registers has finished */
    // RTC_WaitForLastTask();

    // /* Enable the RTC Second */
    // RTC_ITConfig(RTC_IT_SEC, ENABLE);

    // /* Wait until last write operation on RTC registers has finished */
    // RTC_WaitForLastTask();

    // /* Set RTC prescaler: set RTC period to 1sec */
    // RTC_SetPrescaler(32767); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */

    // /* Wait until last write operation on RTC registers has finished */
    // RTC_WaitForLastTask();
}

void reset_DW1000(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // Enable GPIO used for DW1000 reset
    GPIO_InitStructure.GPIO_Pin = DW1000_RSTn;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(DW1000_RSTn_GPIO, &GPIO_InitStructure);

    //drive the RSTn pin low
    GPIO_ResetBits(DW1000_RSTn_GPIO, DW1000_RSTn);

    //put the pin back to tri-state ... as input
    GPIO_InitStructure.GPIO_Pin = DW1000_RSTn;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(DW1000_RSTn_GPIO, &GPIO_InitStructure);

    delay_ms(10);
}

//*********************************************************************************
//!
//!  interrupt IRQ
//!
//*********************************************************************************

int NVIC_DisableDECAIRQ(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;

    /* Configure EXTI line */
    EXTI_InitStructure.EXTI_Line = DECAIRQ_EXTI;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; //MPW3 IRQ polarity is high by default
    EXTI_InitStructure.EXTI_LineCmd = DECAIRQ_EXTI_NOIRQ;
    EXTI_Init(&EXTI_InitStructure);

    return 0;
}

int NVIC_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    // Enable GPIO used as DECA IRQ for interrupt
    GPIO_InitStructure.GPIO_Pin = DECAIRQ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; //IRQ pin should be Pull Down to prevent unnecessary EXT IRQ while DW1000 goes to sleep mode
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_Init(DECAIRQ_GPIO, &GPIO_InitStructure);

    /* Connect EXTI Line to GPIO Pin */
    SYSCFG_EXTILineConfig(DECAIRQ_EXTI_PORT, DECAIRQ_EXTI_PIN);

    /* Configure EXTI line */
    EXTI_InitStructure.EXTI_Line = DECAIRQ_EXTI;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; //MPW3 IRQ polarity is high by default
    EXTI_InitStructure.EXTI_LineCmd = DECAIRQ_EXTI_USEIRQ;
    EXTI_Init(&EXTI_InitStructure);

    /* Set NVIC Grouping to 16 groups of interrupt without sub-grouping */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    /* Enable and set EXTI Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel = DECAIRQ_EXTI_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = DECAIRQ_EXTI_USEIRQ;

    NVIC_Init(&NVIC_InitStructure);

    return 0;
}

/**
  * @brief  Checks whether the specified EXTI line is enabled or not.
  * @param  EXTI_Line: specifies the EXTI line to check.
  *   This parameter can be:
  *     @arg EXTI_Linex: External interrupt line x where x(0..19)
  * @retval The "enable" state of EXTI_Line (SET or RESET).
  */
ITStatus EXTI_GetITEnStatus(uint32_t EXTI_Line)
{
    ITStatus bitstatus = RESET;
    uint32_t enablestatus = 0;
    /* Check the parameters */
    assert_param(IS_GET_EXTI_LINE(EXTI_Line));

    enablestatus = EXTI->IMR & EXTI_Line;
    if (enablestatus != (uint32_t)RESET)
    {
        bitstatus = SET;
    }
    else
    {
        bitstatus = RESET;
    }
    return bitstatus;
}

int is_IRQ_enabled(void)
{
    return ((NVIC->ISER[((uint32_t)(DECAIRQ_EXTI_IRQn) >> 5)] & (uint32_t)0x01 << (DECAIRQ_EXTI_IRQn & (uint8_t)0x1F)) ? 1 : 0);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn port_set_deca_isr()
 *
 * @brief This function is used to install the handling function for DW1000 IRQ.
 *
 * NOTE:
 *   - As EXTI9_5_IRQHandler does not check that port_deca_isr is not null, the user application must ensure that a
 *     proper handler is set by calling this function before any DW1000 IRQ occurs!
 *   - This function makes sure the DW1000 IRQ line is deactivated while the handler is installed.
 *
 * @param deca_isr function pointer to DW1000 interrupt handler to install
 *
 * @return none
 */
void port_set_deca_isr(port_deca_isr_t deca_isr)
{
    /* Check DW1000 IRQ activation status. */
    ITStatus en = port_GetEXT_IRQStatus();

    /* If needed, deactivate DW1000 IRQ during the installation of the new handler. */
    if (en)
    {
        port_DisableEXT_IRQ();
    }
    port_deca_isr = deca_isr;
    if (en)
    {
        port_EnableEXT_IRQ();
    }
}

//*********************************************************************************
//!
//!  peripherals_init()
//!
//*********************************************************************************

void peripherals_init(void)
{
    RCC_AHB1PeriphClockCmd(
        RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB |
            RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD |
            RCC_AHB1Periph_GPIOE,
        ENABLE);

    /* Enable SPI1 clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

    /* Enable SPI2 clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
    LED_Init();
    GPIO_ResetBits(LED_PORT, LED2_GPIO);
    delay_init(168);
    uart_init(115200);
    uart3_init(115200);

    BTN_Init();
    NVIC_Configuration();
    LCD_Init();
    XPT2046_Init();
    SPI_FLASH_Init(); // Flash uses SPI1 on PortB
    Calibrate_or_Get_TouchParaWithFlash(6, 0);
    
    printf("\nMCU READY\n");
}
