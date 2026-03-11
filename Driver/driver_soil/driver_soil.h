#ifndef _DRIVER_SOIL_H_
#define _DRIVER_SOIL_H_

#include "gd32f10x.h"


uint16_t Get_Soil_Raw(void);

/**
 * @brief  将 ADC 原始值转换为 0-100 的湿度百分比
 * @retval 0-100 的湿度百分比
 */
uint8_t Get_Soil_humidity(void);


#endif


