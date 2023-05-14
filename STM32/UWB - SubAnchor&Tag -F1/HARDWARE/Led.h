#ifndef __LED_H
#define __LED_H
#include "sys.h"


#define LED0 PAout(1)
#define LED1 PCout(2)
#define pin_LED0 GPIO_Pin_1
#define pin_LED1 GPIO_Pin_2
#define gpio_LED0 GPIOA
#define gpio_LED1 GPIOC

void LED_Init(void);

#endif
