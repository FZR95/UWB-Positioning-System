/*! ----------------------------------------------------------------------------
 * @file	port.h
 * @brief	HW specific definitions and functions for portability
 *
 * @attention
 *
 * Copyright 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author DecaWave
 */


#ifndef PORT_H_
#define PORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f10x.h"


#define SPIy_PRESCALER				SPI_BaudRatePrescaler_128

#define SPIy						SPI2
#define SPIy_GPIO					GPIOB
#define SPIy_CS						GPIO_Pin_12
#define SPIy_CS_GPIO				GPIOB
#define SPIy_SCK					GPIO_Pin_13
#define SPIy_MISO					GPIO_Pin_14
#define SPIy_MOSI					GPIO_Pin_15

#define SPIx_PRESCALER				SPI_BaudRatePrescaler_8

#define SPIx						SPI1
#define SPIx_GPIO					GPIOA
#define SPIx_CS						GPIO_Pin_4
#define SPIx_CS_GPIO				GPIOA
#define SPIx_SCK					GPIO_Pin_5
#define SPIx_MISO					GPIO_Pin_6
#define SPIx_MOSI					GPIO_Pin_7

#define DW1000_RSTn					GPIO_Pin_8
#define DW1000_RSTn_GPIO			GPIOA

#define DECAIRQ                     GPIO_Pin_3
#define DECAIRQ_GPIO                GPIOC
#define DECAIRQ_EXTI                EXTI_Line3
#define DECAIRQ_EXTI_PORT           GPIO_PortSourceGPIOC
#define DECAIRQ_EXTI_PIN            GPIO_PinSource3
#define DECAIRQ_EXTI_IRQn           EXTI3_IRQn
#define DECAIRQ_EXTI_USEIRQ         ENABLE
#define DECAIRQ_EXTI_NOIRQ          DISABLE

#define DECA_WAKEUP                 GPIO_Pin_8
#define DECA_WAKEUP_GPIO            GPIOC
#define DECA_EXTON                  GPIO_Pin_6
#define DECA_EXTON_GPIO             GPIOC

#define DECA_IO0                    GPIO_Pin_4
#define DECA_IO0_GPIO               GPIOC
#define DECA_IO1                    GPIO_Pin_5
#define DECA_IO1_GPIO               GPIOC
#define DECA_IO2                    GPIO_Pin_0
#define DECA_IO2_GPIO               GPIOB
#define DECA_IO3                    GPIO_Pin_1
#define DECA_IO3_GPIO               GPIOB
#define DECA_IO4                    GPIO_Pin_10
#define DECA_IO4_GPIO               GPIOB
#define DECA_IO5                    GPIO_Pin_11
#define DECA_IO5_GPIO               GPIOB
#define DECA_IO6                    GPIO_Pin_7
#define DECA_IO6_GPIO               GPIOC
#define DECA_IO7                    GPIO_Pin_9
#define DECA_IO7_GPIO               GPIOC

#define USARTx						USART2

#define port_USARTx_busy_sending()	0 //(USART_GetFlagStatus((USARTx),(USART_FLAG_TXE))==(RESET))
#define port_USARTx_no_data()		0 //(USART_GetFlagStatus((USARTx),(USART_FLAG_RXNE))==(RESET))
#define port_USARTx_send_data(x)	  //USART_SendData((USARTx),(uint8_t)(x))
#define port_USARTx_receive_data()	0 //USART_ReceiveData(USARTx)

#define port_SPIx_busy_sending()		(SPI_I2S_GetFlagStatus((SPIx),(SPI_I2S_FLAG_TXE))==(RESET))
#define port_SPIx_no_data()				(SPI_I2S_GetFlagStatus((SPIx),(SPI_I2S_FLAG_RXNE))==(RESET))
#define port_SPIx_send_data(x)			SPI_I2S_SendData((SPIx),(x))
#define port_SPIx_receive_data()		SPI_I2S_ReceiveData(SPIx)
#define port_SPIx_disable()				SPI_Cmd(SPIx,DISABLE)
#define port_SPIx_enable()              SPI_Cmd(SPIx,ENABLE)
#define port_SPIx_set_chip_select()		GPIO_SetBits(SPIx_CS_GPIO,SPIx_CS)
#define port_SPIx_clear_chip_select()	GPIO_ResetBits(SPIx_CS_GPIO,SPIx_CS)


#define port_GET_stack_pointer()		__get_MSP()
#define port_GET_rtc_time()				RTC_GetCounter()
#define port_SET_rtc_time(x)			RTC_SetCounter(x)

ITStatus EXTI_GetITEnStatus(uint32_t x);

#define port_GetEXT_IRQStatus()             EXTI_GetITEnStatus(DECAIRQ_EXTI_IRQn)
#define port_DisableEXT_IRQ()               NVIC_DisableIRQ(DECAIRQ_EXTI_IRQn)
#define port_EnableEXT_IRQ()                NVIC_EnableIRQ(DECAIRQ_EXTI_IRQn)
#define port_CheckEXT_IRQ()                 GPIO_ReadInputDataBit(DECAIRQ_GPIO, DECAIRQ)
int NVIC_DisableDECAIRQ(void);

int is_IRQ_enabled(void);

#define gpio_set(x)				0
#define gpio_reset(x)				0
			
/* DW1000 IRQ (EXTI9_5_IRQ) handler type. */
typedef void (*port_deca_isr_t)(void);

/* DW1000 IRQ handler declaration. */
extern port_deca_isr_t port_deca_isr;

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn peripherals_init()
 *
 * @brief Initialise all peripherals.
 *
 * @param none
 *
 * @return none
 */
void peripherals_init (void);

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
void port_set_deca_isr(port_deca_isr_t deca_isr);

void SPI_ChangeRate(uint16_t scalingfactor);
void SPI_ConfigFastRate(uint16_t scalingfactor);

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn spi_set_rate_low()
 *
 * @brief Set SPI rate to less than 3 MHz to properly perform DW1000 initialisation.
 *
 * @param none
 *
 * @return none
 */
void spi_set_rate_low (void);

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn spi_set_rate_high()
 *
 * @brief Set SPI rate as close to 20 MHz as possible for optimum performances.
 *
 * @param none
 *
 * @return none
 */
void spi_set_rate_high (void);

void reset_DW1000(void);

#endif /* PORT_H_ */
