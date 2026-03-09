#ifndef __ENCODER_H
#define __ENCODER_H

#include "gd32f10x.h"                   // 替换为GD32的设备头文件

void Encoder_Init(void);
int16_t Encoder_Get(void);

#endif
