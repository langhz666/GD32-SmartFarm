#ifndef _DRIVER_ADC_H_
#define _DRIVER_ADC_H_ 

// 定义通道数
#define ADC_CHANNEL_COUNT 2
#define ADC_CHANNEL_SOIL    0   // 土壤 - 通道4
#define ADC_CHANNEL_RAIN    1   // 雨滴 - 通道5

#include "gd32f10x.h"

void ADC_DMA_MultiChannel_Init(void);

#endif


