/*
 * @Author: langhz666 3204498297@qq.com
 * @Date: 2026-03-13 18:04:36
 * @LastEditors: langhz666 3204498297@qq.com
 * @LastEditTime: 2026-03-15 21:38:00
 * @FilePath: \GD32F103C8T6\App\task_key.c
 * @Description: 按键处理任务，负责按键扫描、页面切换和阈值编辑模式切换
 */

#include "app_shared.h"              // 包含应用层共享头文件，定义全局变量和任务函数声明
#include "driver_key/driver_key.h"    // 包含按键驱动头文件，提供按键扫描函数
#include "driver_buzzer/driver_buzzer.h"  // 包含蜂鸣器驱动头文件，提供蜂鸣器控制函数
#include "delay.h"                    // 包含延时函数头文件，提供延时函数

/**
 * @brief 按键处理任务
 * @param pvParameters 任务参数（未使用）
 * 
 * @note 任务功能：
 *       1. 扫描按键状态（KEY1、KEY2）
 *       2. 按键按下时蜂鸣器提示
 *       3. KEY1：切换显示页面（主页 -> 阈值页1 -> 阈值页2 -> 主页）
 *       4. KEY2：在阈值页面切换编辑/浏览模式
 *       5. 编辑模式退出时保存配置到Flash
 * 
 * @note FreeRTOS任务特性：
 *       - 任务优先级：3（最高优先级，确保按键响应及时）
 *       - 任务栈大小：TASK_STACK_SIZE_KEY
 *       - 任务周期：10ms（按键扫描周期）
 *       - 使用vTaskDelay实现周期性延时
 * 
 * @note 按键功能说明：
 *       KEY1（页面切换）：
 *       - 主页(PAGE_HOME) -> 阈值页1(PAGE_RANGE)
 *       - 阈值页1(PAGE_RANGE) -> 阈值页2(PAGE_RANGE)
 *       - 阈值页2(PAGE_RANGE) -> 主页(PAGE_HOME)
 *       - 通过PageEventQueue通知OLED_Task更新显示
 *       
 *       KEY2（编辑模式切换）：
 *       - 仅在阈值页面有效
 *       - 浏览模式 -> 编辑模式：开始编辑阈值
 *       - 编辑模式 -> 浏览模式：保存配置到Flash
 * 
 * @note 队列通信：
 *       - 使用xQueueSend()向PageEventQueue发送页面切换事件
 *       - 不等待队列空间（超时为0），如果队列满则丢弃事件
 *       - OLED_Task从队列接收事件并更新显示
 * 
 * @note 全局变量操作：
 *       - currentPage：当前显示页面索引
 *       - rangeEditState：阈值编辑状态（浏览/编辑）
 *       - oled_dirty：OLED显示脏标志，触发OLED_Task刷新显示
 */
void Key_Task(void *pvParameters)
{
    uint8_t key;  // 定义按键变量，用于存储按键扫描结果

    while(1)  // 无限循环，任务持续运行
    {
        key = Key_Scan(0);  // 扫描按键状态，参数0表示非阻塞模式，返回按键值（0表示无按键按下）
        if (key)  // 判断是否有按键按下
        {
            Buzzer_On();  // 开启蜂鸣器，发出按键提示音
            DelayNms(50);  // 延时50毫秒，蜂鸣器响50毫秒
            Buzzer_Off();  // 关闭蜂鸣器
            
            oled_dirty = 1;  // 设置OLED显示脏标志为1，通知OLED任务需要刷新显示
            if (key == KEY1_PRES)  // 判断是否按下KEY1
            {
                currentPage++;  // 当前页面索引加1，切换到下一个页面
                if (currentPage > PAGE_RANGE)  // 判断当前页面索引是否超过阈值页
                {
                    currentPage = PAGE_HOME;  // 超过阈值页则回到主页
                }
                xQueueSend(PageEventQueue, &currentPage, 0);  // 将新的页面索引发送到PageEventQueue队列，超时为0（不等待）
            }
            else if (key == KEY2_PRES)  // 判断是否按下KEY2
            {
                if (currentPage == PAGE_RANGE)  // 判断当前是否在阈值页面
                {
                    if (rangeEditState == RANGE_EDIT_STATE_BROWSING)  // 判断当前是否在浏览模式
                    {
                        rangeEditState = RANGE_EDIT_STATE_EDITING;  // 切换到编辑模式，开始编辑阈值
                    }
                    else  // 当前在编辑模式
                    {
                        rangeEditState = RANGE_EDIT_STATE_BROWSING;  // 切换到浏览模式，结束编辑
                        App_SaveRangeConfig();  // 保存阈值配置到Flash
                    }
                    oled_dirty = 1;  // 设置OLED显示脏标志为1，通知OLED任务需要刷新显示
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // 延时10毫秒，释放CPU资源，实现周期性按键扫描
    }
}
