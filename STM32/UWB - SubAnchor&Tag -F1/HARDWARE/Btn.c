#include "Btn.h"

void BTN_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = pin_BTN0 | pin_BTN1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(gpio_BTN0, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = pin_BTN2;
    GPIO_Init(gpio_BTN2, &GPIO_InitStructure);
    GPIO_SetBits(gpio_BTN0, pin_BTN0 | pin_BTN1);
    GPIO_ResetBits(gpio_BTN2, pin_BTN2);
}
