#include "Btn.h"

void BTN_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE); //使能GPIOA,GPIOE时钟

    GPIO_InitStructure.GPIO_Pin = pin_BTN0 | pin_BTN1; //KEY0 KEY1 KEY2对应引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;                        //普通输入模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;                  //100M
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;                        //上拉
    GPIO_Init(gpio_BTN, &GPIO_InitStructure);                              //初始化GPIOE2,3,4

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;      //WK_UP对应引脚PA0
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN; //下拉
    GPIO_Init(GPIOA, &GPIO_InitStructure);         //初始化GPIOA0
}
