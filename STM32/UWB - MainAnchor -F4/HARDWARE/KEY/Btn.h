#ifndef __BTN_H
#define __BTN_H
#include "sys.h"

#define pin_BTN0 GPIO_Pin_2
#define pin_BTN1 GPIO_Pin_3
#define gpio_BTN GPIOE

void BTN_Init(void);

#endif
