#include "driver_light/driver_light.h"
#include "driver_usart/driver_usart.h"
#include "delay.h"
#include "stdio.h"

// 保持7位地址不变
#define COMMAND_MODE  0x10
#define ADDRESS       0x23 

// 宏定义写地址和读地址（左移1位后加上读写位）
#define ADDRESS_WRITE ((ADDRESS << 1) | 0)  // 结果为 0x46
#define ADDRESS_READ  ((ADDRESS << 1) | 1)  // 结果为 0x47

// 传感器分辨率为1-65535
uint16_t Light_Get(void) 
{
  uint8_t receive_light_data[2] = {0};
  uint16_t light_result = 0;
  
  // 1. 发送测量命令
  IIC_Light_Start();
  IIC_Light_SendByte(ADDRESS_WRITE); // 发送写地址 0x46
  if (IIC_Light_WaitAck()) 
  {
    IIC_Light_Stop();
    return 0; // 如果没收到ACK，说明传感器没连上
  }
  
  IIC_Light_SendByte(COMMAND_MODE);  // 发送连续高分辨率模式命令
  if (IIC_Light_WaitAck())
  {
    IIC_Light_Stop();
    return 0;
  }
  IIC_Light_Stop();
  
  // 2. 等待传感器完成测量 (最大需要180ms)
  DelayNms(180);
  
  // 3. 读取测量数据
  IIC_Light_Start();
  IIC_Light_SendByte(ADDRESS_READ);  // 发送读地址 0x47
  if (IIC_Light_WaitAck())
  {
    IIC_Light_Stop();
    return 0;
  }
  
  receive_light_data[0] = IIC_Light_ReadByte(); // 读高8位
  IIC_Light_SendAck();                          // 给传感器回ACK，表示还要继续读
  receive_light_data[1] = IIC_Light_ReadByte(); // 读低8位
  IIC_Light_SendNack();                         // 给传感器回NACK，表示不读了
  
  IIC_Light_Stop();
  
  // 4. 数据合成与计算
  light_result = (receive_light_data[0] << 8) | receive_light_data[1];
  light_result = (uint16_t)(light_result / 1.2f);
  
  return light_result;
}
