/*
 * @Author: langhz666 3204498297@qq.com
 * @Date: 2026-03-15 00:07:03
 * @LastEditors: langhz666 3204498297@qq.com
 * @LastEditTime: 2026-03-15 20:40:03
 * @FilePath: \GD32F103C8T6\App\app_shared.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __APP_SHARED_H  // 防止头文件重复包含，如果没有定义__APP_SHARED_H宏
#define __APP_SHARED_H  // 定义__APP_SHARED_H宏，表示已经包含过此头文件

#include "app.h"           // 包含应用层头文件，定义应用层结构体、枚举和函数声明
#include "FreeRTOS.h"       // 包含FreeRTOS内核头文件，定义FreeRTOS内核函数和宏
#include "queue.h"          // 包含FreeRTOS队列头文件，定义队列类型和操作函数
#include "semphr.h"         // 包含FreeRTOS信号量头文件，定义信号量和互斥锁类型和操作函数
#include "gd32f10x.h"       // 包含GD32F10x系列芯片头文件，定义芯片寄存器和外设

extern QueueHandle_t SensorDataQueue;  // 声明外部变量SensorDataQueue，传感器数据队列句柄
extern QueueHandle_t PageEventQueue;   // 声明外部变量PageEventQueue，页面事件队列句柄
extern SemaphoreHandle_t OLED_Mutex;   // 声明外部变量OLED_Mutex，OLED互斥锁句柄

extern int16_t Num;  // 声明外部变量Num，编码器数值变量
extern uint16_t Light;  // 声明外部变量Light，光照强度变量
extern uint8_t oled_dirty;  // 声明外部变量oled_dirty，OLED显示脏标志（1=需要刷新，0=无需刷新）
extern FarmSafeRange_t farmSafeRange;  // 声明外部变量farmSafeRange，农场安全阈值范围结构体
extern RangeEditIndex_t rangeEditIndex;  // 声明外部变量rangeEditIndex，当前编辑的阈值项索引
extern RangeEditState_t rangeEditState;  // 声明外部变量rangeEditState，阈值编辑状态（浏览/编辑）
extern uint8_t isEditingPage;  // 声明外部变量isEditingPage，是否在编辑页面标志（0=不在编辑页面，1=在编辑页面）
extern DisplayPage_t currentPage;  // 声明外部变量currentPage，当前显示页面（主页/阈值页）
extern uint8_t buzzerAlarmEnabled;  // 声明外部变量buzzerAlarmEnabled，蜂鸣器警报开关（0=关闭，1=开启）

void floatToIntDec(float value, int *intPart, int *decPart);  // 声明函数floatToIntDec，将浮点数拆分为整数部分和小数部分
uint8_t getIntLen(int val);  // 声明函数getIntLen，获取整数的字符串长度

void LED_Task(void *pvParameters);  // 声明函数LED_Task，LED闪烁任务
void OLED_Task(void *pvParameters);  // 声明函数OLED_Task，OLED显示任务
void Key_Task(void *pvParameters);  // 声明函数Key_Task，按键处理任务
void Sensor_Task(void *pvParameters);  // 声明函数Sensor_Task，传感器数据采集任务
void Encoder_Task(void *pvParameters);  // 声明函数Encoder_Task，编码器处理任务

#endif  // 结束头文件包含保护
