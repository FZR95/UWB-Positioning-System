#ifndef __BTN_H
#define __BTN_H
#include "sys.h"


#define BTN0 PCin(1)
#define BTN1 PCin(0)
#define BTN2 PAin(0)
#define pin_BTN0 GPIO_Pin_1
#define pin_BTN1 GPIO_Pin_0
#define pin_BTN2 GPIO_Pin_0
#define gpio_BTN0 GPIOC
#define gpio_BTN1 GPIOC
#define gpio_BTN2 GPIOA

void BTN_Init(void);

#endif
