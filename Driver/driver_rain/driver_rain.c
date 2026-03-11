#include "driver_rain/driver_rain.h"
#include "driver_adc/driver_adc.h"
#include "delay.h"
#include "driver_rain.h"

/*硬件连接PA5为AO引脚*/
extern uint16_t adc_buffer[ADC_CHANNEL_COUNT];

uint16_t Get_Rain_Raw(void)
{
    // 配置里通道 5 排在第 1 位，直接返回数组的第 1 个元素
    return adc_buffer[ADC_CHANNEL_RAIN];
}

/**
 * 获得下雨大小
 * 此值仅是雨水打在传感器上，导致传感器电阻减小
 * 随后测量到的传感器的电压值
 * 仅能相对地展示降雨大小，无法精确地表示降雨量
 * @return 0~100的降雨值，0表示无雨，100表示大雨
 */
uint8_t Get_Rain_size(void) 
{
  uint16_t adc = Get_Rain_Raw();
  if (adc > 4000) 
  {
    adc = 4000;
  }
  return 100 - adc / 40;
}
