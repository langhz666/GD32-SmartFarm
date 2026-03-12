#ifndef _DRIVER_AHT20_H
#define _DRIVER_AHT20_H

#include "gd32f10x.h"
#include "delay.h"
#include <stdbool.h>

/* ===================================================================== */
/* I2C 引脚定义 (可根据你的实际接线自由修改)                             */
/* ===================================================================== */
#define AHT20_IIC_RCU_PORT      RCU_GPIOB
#define AHT20_IIC_PORT          GPIOB
#define AHT20_IIC_SCL_PIN       GPIO_PIN_10
#define AHT20_IIC_SDA_PIN       GPIO_PIN_11

/* ========================= 宏定义区 ========================= */
/* 读取SDA引脚电平（用于ACK检测、读数据） */
#define AHT20_IIC_READ_SDA()              gpio_input_bit_get(AHT20_IIC_PORT, AHT20_IIC_SDA_PIN)

/* SCL时钟线控制（开漏输出：输出高本质上是“释放”，由上拉拉高） */
#define AHT20_IIC_SCL_HIGH()              gpio_bit_set(AHT20_IIC_PORT, AHT20_IIC_SCL_PIN)
#define AHT20_IIC_SCL_LOW()               gpio_bit_reset(AHT20_IIC_PORT, AHT20_IIC_SCL_PIN)

/* SDA数据线控制（开漏输出：输出高本质上是“释放”，由上拉拉高） */
#define AHT20_IIC_SDA_HIGH()              gpio_bit_set(AHT20_IIC_PORT, AHT20_IIC_SDA_PIN)
#define AHT20_IIC_SDA_LOW()               gpio_bit_reset(AHT20_IIC_PORT, AHT20_IIC_SDA_PIN)

/*
 * IIC时序延时AHT20_IIC_DELAY_US(us)
 */
#define AHT20_IIC_DELAY_US(us)            do { DelayNus((us)); } while (0)

/* ===================================================================== */
/* 核心函数声明                                                          */
/* ===================================================================== */

void AHT20_IIC_Init(void);

void AHT20_IIC_Start(void);

void AHT20_IIC_Stop(void);

void AHT20_IIC_SendByte(uint8_t tx_byte);

uint8_t AHT20_IIC_ReadByte(void);

bool AHT20_IIC_WaitAck(void);

void AHT20_IIC_SendAck(void);

void AHT20_IIC_SendNack(void);

bool AHT20_Init(void);

bool AHT20_Read_Temp_Humi(float *temp, float *humi);

#endif // _DRIVER_AHT20_H
