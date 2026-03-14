#ifndef __DRIVER_BMP280_H
#define __DRIVER_BMP280_H

#include "gd32f10x.h" // 根据实际型号调整
#include "delay.h"
#include <stdbool.h>

#define BMP280_IIC_RCU_PORT   RCU_GPIOB
#define BMP280_IIC_PORT       GPIOB
#define BMP280_IIC_SCL_PIN    GPIO_PIN_10
#define BMP280_IIC_SDA_PIN    GPIO_PIN_11

/* 延时宏和 GPIO 操作宏 (确保带有 BMP280_ 前缀) */
#define BMP280_IIC_DELAY_US(x) DelayNus(x) // 假设你有 delay_us

#define BMP280_IIC_READ_SDA()  gpio_input_bit_get(BMP280_IIC_PORT, BMP280_IIC_SDA_PIN)
#define BMP280_IIC_SCL_HIGH()  gpio_bit_set(BMP280_IIC_PORT, BMP280_IIC_SCL_PIN)
#define BMP280_IIC_SCL_LOW()   gpio_bit_reset(BMP280_IIC_PORT, BMP280_IIC_SCL_PIN)
#define BMP280_IIC_SDA_HIGH()  gpio_bit_set(BMP280_IIC_PORT, BMP280_IIC_SDA_PIN)
#define BMP280_IIC_SDA_LOW()   gpio_bit_reset(BMP280_IIC_PORT, BMP280_IIC_SDA_PIN)

bool BMP280_Init(void);
void BMP280_Read_Temp_Press(float *temp, float *press);

#endif
