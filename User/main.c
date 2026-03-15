/*
 * @Author: langhz666 3204498297@qq.com
 * @Date: 2026-03-08 21:22:10
 * @LastEditors: langhz666 3204498297@qq.com
 * @LastEditTime: 2026-03-15 21:21:50
 * @FilePath: \GD32F103C8T6\User\main.c
 * @Description: 主函数入口，负责系统初始化和FreeRTOS任务调度启动
 */
#include "gd32f10x.h"         // 包含GD32F10x系列芯片头文件，定义芯片寄存器和外设
#include "gd32f10x_libopt.h"  // 包含GD32F10x库选项头文件，定义库函数和配置
#include "app.h"               // 包含应用层头文件，定义应用层函数和配置
#include <stdio.h>              // 包含标准输入输出头文件，提供printf等函数

/**
 * @brief 主函数入口
 * @note FreeRTOS应用程序的标准启动流程：
 *       1. 硬件驱动初始化（GPIO、外设等）
 *       2. 创建FreeRTOS内核对象（队列、信号量等）
 *       3. 创建任务
 *       4. 启动任务调度器
 * 
 * @details FreeRTOS调度器启动后，main函数将不再返回
 *          如果调度器启动失败，会进入死循环并打印错误信息
 */
int main(void)
{
    App_InitDrivers();           // 初始化所有硬件驱动（LED、OLED、传感器等）
    App_CreateQueues();         // 创建FreeRTOS队列（传感器数据队列、页面事件队列）
    App_CreateSemaphores();     // 创建FreeRTOS信号量（OLED互斥锁）
    App_CreateTasks();          // 创建FreeRTOS任务（LED、OLED、Key、Sensor、Encoder任务）
    vTaskStartScheduler();      // 启动FreeRTOS任务调度器，开始多任务调度

    printf("ERROR: Scheduler failed to start!\r\n"); // 如果调度器启动失败，打印错误信息
    while(1);                // 进入死循环，防止程序继续执行（正常情况下不会执行到这里）
}
