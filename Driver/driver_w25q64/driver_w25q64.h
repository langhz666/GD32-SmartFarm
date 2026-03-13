#ifndef _DRIVER_W25Q64_H
#define _DRIVER_W25Q64_H

#include "gd32f10x.h" // 根据你的实际芯片型号修改头文件
#include <stdint.h> // 标准整数类型定义头文件
#include "driver_usart/driver_usart.h" // 用于调试输出
#include <stdio.h> // 用于 sprintf 函数格式化字符串输出
#include <string.h>



// 定义测试地址 (使用第 0 个扇区的开头)
#define TEST_FLASH_ADDR     0x000000 

// 准备要写入的测试字符串
uint8_t WriteBuffer[] = "Hello W25Q64! This is a test from GD32 software SPI.";
// 准备一个空数组，用于存放读出来的数据
uint8_t ReadBuffer[100] = {0};

// --- GPIO 端口和引脚定义 ---
#define W25Q_RCU      RCU_GPIOB
#define W25Q_PORT     GPIOB
#define W25Q_CS_PIN   GPIO_PIN_12
#define W25Q_CLK_PIN  GPIO_PIN_13
#define W25Q_MISO_PIN GPIO_PIN_14
#define W25Q_MOSI_PIN GPIO_PIN_15

// --- 引脚底层操作宏 (提高代码执行速度) ---
#define W25Q_CS_HIGH()    gpio_bit_set(W25Q_PORT, W25Q_CS_PIN)
#define W25Q_CS_LOW()     gpio_bit_reset(W25Q_PORT, W25Q_CS_PIN)

#define W25Q_CLK_HIGH()   gpio_bit_set(W25Q_PORT, W25Q_CLK_PIN)
#define W25Q_CLK_LOW()    gpio_bit_reset(W25Q_PORT, W25Q_CLK_PIN)

#define W25Q_MOSI_HIGH()  gpio_bit_set(W25Q_PORT, W25Q_MOSI_PIN)
#define W25Q_MOSI_LOW()   gpio_bit_reset(W25Q_PORT, W25Q_MOSI_PIN)

#define W25Q_MISO_READ()  gpio_input_bit_get(W25Q_PORT, W25Q_MISO_PIN)

// --- W25Q64 常用指令码 ---
#define W25X_WriteEnable      0x06 
#define W25X_ReadStatusReg    0x05 
#define W25X_ReadData         0x03 
#define W25X_PageProgram      0x02 
#define W25X_SectorErase      0x20 
#define W25X_JedecDeviceID    0x9F 

// --- 函数声明 ---
void W25Q64_Init(void);
uint16_t W25Q64_ReadID(void);
void W25Q64_ReadData(uint32_t ReadAddr, uint8_t* pBuffer, uint16_t NumByteToRead);
void W25Q64_EraseSector(uint32_t Dst_Addr);
void W25Q64_PageProgram(uint32_t WriteAddr, uint8_t* pBuffer, uint16_t NumByteToWrite);

#endif /* _DRIVER_W25Q64_H */
