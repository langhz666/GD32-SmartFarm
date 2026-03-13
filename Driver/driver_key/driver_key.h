#ifndef __DRIVER_KEY_H
#define __DRIVER_KEY_H

#include "gd32f10x.h"

#define KEY_RCU      RCU_GPIOC
#define KEY_PORT     GPIOC
#define KEY1_PIN     GPIO_PIN_14
#define KEY2_PIN     GPIO_PIN_15
// #define KEY3_PIN     GPIO_PIN_15

#define KEY_NONE     0
#define KEY1_PRES    1
#define KEY2_PRES    2
// #define KEY3_PRES    3

void Key_Init(void);
uint8_t Key_Scan(uint8_t mode);

#endif
