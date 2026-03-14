#ifndef __ENCODER_H
#define __ENCODER_H

#include "gd32f10x.h"

#define ENCODER_KEY_RCU      RCU_GPIOB
#define ENCODER_KEY_PORT     GPIOB
#define ENCODER_KEY_PIN      GPIO_PIN_2

#define ENCODER_KEY_NONE     0
#define ENCODER_KEY_PRES     1

void Encoder_Init(void);
int16_t Encoder_Get(void);
uint8_t Encoder_Key_Scan(uint8_t mode);

#endif
