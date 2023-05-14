#include "Led.h"

void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    GPIO_InitStructure.GPIO_Pin = pin_LED0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(gpio_LED0, &GPIO_InitStructure);
    GPIO_SetBits(gpio_LED0, pin_LED0);

    GPIO_InitStructure.GPIO_Pin = pin_LED1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(gpio_LED1, &GPIO_InitStructure);
    GPIO_SetBits(gpio_LED1, pin_LED1);
}
