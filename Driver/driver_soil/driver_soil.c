#include "driver_soil/driver_soil.h"
#include "driver_adc/driver_adc.h"
#include "delay.h"

extern uint16_t adc_buffer[ADC_CHANNEL_COUNT];

uint16_t Get_Soil_Raw(void)
{
    return adc_buffer[ADC_CHANNEL_SOIL];
}

// 定义你实测的校准极值
#define MAX_VALUE 4085  // 0% 湿度时的 ADC 值
#define MIN_VALUE 1400  // 100% 湿度时的 ADC 值

/**
 * @brief  将 ADC 原始值转换为 0-100 的湿度百分比
 * @retval 0-100 的湿度百分比
 */
uint8_t Get_Soil_humidity(void) 
{
    uint16_t raw_adc = Get_Soil_Raw();
    uint8_t percentage = 0;

    // 1. 限制数据范围（边界裁切）
    // 如果比最干还干，就当做最干；如果比最湿还湿，就当做最湿
    if (raw_adc > MAX_VALUE) {
        raw_adc = MAX_VALUE;
    }
    if (raw_adc < MIN_VALUE) {
        raw_adc = MIN_VALUE;
    }

    // 2. 整数线性映射计算
    // 公式: (最干值 - 当前值) * 100 / (最干值 - 最湿值)
    percentage = (MAX_VALUE - raw_adc) * 100 / (MAX_VALUE - MIN_VALUE);

    return (uint8_t)percentage;
}
