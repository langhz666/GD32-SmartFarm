/*
 * @Author: langhz666 3204498297@qq.com
 * @Date: 2026-03-13 18:04:36
 * @LastEditors: langhz666 3204498297@qq.com
 * @LastEditTime: 2026-03-15 22:24:11
 * @FilePath: \GD32F103C8T6\App\task_led.c
 * @Description: LED闪烁任务，负责系统运行状态指示
 */

#include "app_shared.h"              // 包含应用层共享头文件，定义全局变量和任务函数声明
#include "driver_led/driver_led.h"  // 包含LED驱动头文件，提供LED控制函数

/**
 * @brief LED闪烁任务
 * @param pvParameters 任务参数（未使用）
 * 
 * @note 任务功能：
 *       - 控制LED1以500ms周期闪烁
 *       - 用于指示系统正常运行
 *       - 如果LED停止闪烁，说明系统可能出现问题
 * 
 * @note FreeRTOS任务特性：
 *       - 任务优先级：2
 *       - 任务栈大小：TASK_STACK_SIZE_LED
 *       - 任务周期：500ms（LED闪烁周期）
 *       - 使用vTaskDelay实现周期性延时，不占用CPU资源
 * 
 * @note 系统状态指示：
 *       - LED正常闪烁：系统正常运行，任务调度正常
 *       - LED常亮或常灭：系统可能死机或任务调度异常
 *       - LED闪烁频率变化：系统负载变化或任务优先级问题
 */
void LED_Task(void *pvParameters)
{
    while(1)  // 无限循环，任务持续运行
    {
        led_toggle(1);  // 切换LED1的状态（亮->灭或灭->亮），实现LED闪烁效果
        vTaskDelay(pdMS_TO_TICKS(500));  // 延时500毫秒，释放CPU资源，实现周期性LED闪烁
    }
}
