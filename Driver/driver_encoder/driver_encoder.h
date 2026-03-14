#ifndef __ENCODER_H
#define __ENCODER_H

#include "gd32f10x.h"

#define ENCODER_KEY_RCU      RCU_GPIOA

#define ENCODER_KEY_PORT  GPIOA
#define ENCODER_KEY_PIN   GPIO_PIN_7

#define ENCODER_KEY_PRES  1    // 确保有这个按键按下的返回值定义
#define ENCODER_KEY_NONE  0    // 确保有按键未按下的返回值定义

void Encoder_Init(void);
int16_t Encoder_Get(void);
uint8_t Encoder_Key_Scan(uint8_t mode);

#endif
