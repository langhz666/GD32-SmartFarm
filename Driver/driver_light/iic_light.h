#ifndef DRIVER_I2C_H
#define DRIVER_I2C_H

#include "gd32f10x.h"
#include <stdbool.h>

/* ========================= 宏定义区 ========================= */
/* 读取SDA引脚电平（用于ACK检测、读数据） */
#define IIC_READ_SDA()              gpio_input_bit_get(GPIOA, GPIO_PIN_1)

/* SCL时钟线控制（开漏输出：输出高本质上是“释放”，由上拉拉高） */
#define IIC_SCL_HIGH()              gpio_bit_set(GPIOA, GPIO_PIN_0)
#define IIC_SCL_LOW()               gpio_bit_reset(GPIOA, GPIO_PIN_0)

/* SDA数据线控制（开漏输出：输出高本质上是“释放”，由上拉拉高） */
#define IIC_SDA_HIGH()              gpio_bit_set(GPIOA, GPIO_PIN_1)
#define IIC_SDA_LOW()               gpio_bit_reset(GPIOA, GPIO_PIN_1)

/*
 * IIC时序延时IIC_DELAY_US(us)
 */
#define IIC_DELAY_US(us)            do { DelayNus((us)); } while (0)

/* ========================= 接口函数声明 ========================= */
/*
 * @brief  软件IIC初始化（PB6=SCL, PB7=SDA）
 */
void IIC_Light_Init(void);

/*
 * @brief  产生IIC起始信号（Start）
 */
void IIC_Light_Start(void);

/*
 * @brief  产生IIC停止信号（Stop）
 */
void IIC_Light_Stop(void);

/*
 * @brief  发送1个字节（高位在前，MSB first）
 * @param  tx_byte: 待发送数据
 */
void IIC_Light_SendByte(uint8_t tx_byte);

/*
 * @brief  读取1个字节（高位在前，MSB first）
 * @return 读取到的1字节数据
 */
uint8_t IIC_Light_ReadByte(void);

/*
 * @brief  等待从机应答（ACK）
 * @return true  - 收到ACK
 * @return false - 超时未收到ACK
 */
uint8_t IIC_Light_WaitAck(void);

/*
 * @brief  主机发送ACK（应答）
 */
void IIC_Light_SendAck(void);

/*
 * @brief  主机发送NACK（非应答）
 */
void IIC_Light_SendNack(void);

/**
 * @brief  I2C write data
 * @param  SlaveAddress: slave address
 * @param  Data: data buffer
 * @param  len: data length
 * @retval 0: success 1: fail
 */
uint8_t IIC_Light_WriteData(uint8_t SlaveAddress, uint8_t *Data, uint8_t len);

/**
 * @brief  I2C read data
 * @param  SlaveAddress: slave address
 * @param  Data: data buffer
 * @param  len: data length
 * @retval 0: success 1: fail
 */
uint8_t IIC_Light_ReadData(uint8_t SlaveAddress, uint8_t *Data, uint8_t len);


#endif 
