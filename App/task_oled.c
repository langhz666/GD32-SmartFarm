/*
 * @Author: langhz666 3204498297@qq.com
 * @Date: 2026-03-13 18:04:36
 * @LastEditors: langhz666 3204498297@qq.com
 * @LastEditTime: 2026-03-15 22:25:11
 * @FilePath: \GD32F103C8T6\App\task_oled.c
 * @Description: OLED显示任务，负责OLED屏幕的显示更新、页面渲染和阈值编辑界面显示
 */

#include "app_shared.h"              // 包含应用层共享头文件，定义全局变量和任务函数声明
#include "driver_oled/driver_oled.h"  // 包含OLED驱动头文件，提供OLED显示函数
#include <stdio.h>                    // 包含标准输入输出头文件，提供sprintf等函数

/**
 * @brief 绘制下划线（用于标识当前编辑的阈值项）
 * @param x 下划线起始X坐标
 * @param y 下划线Y坐标
 * @param len 下划线长度（像素）
 * @param isActive 是否激活（当前编辑项）
 * 
 * @note 功能说明：
 *       - 在编辑模式下，当前编辑项会显示闪烁的下划线
 *       - 浏览模式下，所有下划线常亮显示
 *       - 闪烁周期：500ms（250ms亮，250ms灭）
 * 
 * @note FreeRTOS时间获取：
 *       - 使用xTaskGetTickCount()获取系统滴答计数
 *       - 滴答计数除以250得到闪烁状态
 *       - 不需要额外的定时器资源
 */
static void drawUnderline(uint8_t x, uint8_t y, uint8_t len, uint8_t isActive)
{
    if (isActive)  // 判断是否是当前编辑项
    {
        uint8_t blink_on = ((xTaskGetTickCount() / 250) % 2 == 0);  // 计算闪烁状态，每250ms切换一次亮灭
        if (rangeEditState == RANGE_EDIT_STATE_BROWSING || (rangeEditState == RANGE_EDIT_STATE_EDITING && blink_on))  // 判断是否在浏览模式或在编辑模式且闪烁状态为亮
        {
            for (uint8_t i = 0; i < len; i++)  // 循环绘制下划线的每个像素点
            {
                OLED_DrawPoint(x + i, y);  // 在指定坐标绘制像素点，形成下划线
            }
        }
    }
}

/**
 * @brief 反色显示标题文本（白底黑字）
 * @param x 起始X坐标
 * @param y 起始Y坐标
 * @param title 标题文本
 * @param fontSize 字体大小
 * 
 * @note 功能说明：
 *       - 先显示文本，再反色显示区域
 *       - 用于区分标题和数据
 *       - 支持中英文混合文本
 */
static void reverseTitle(int16_t x, int16_t y, char *title, uint8_t fontSize)
{
    uint16_t width = 0;  // 定义文本宽度变量，初始值为0
    char *p = title;  // 定义指针，指向标题文本的起始位置
    
    while (*p != '\0')  // 循环遍历标题文本的每个字符，直到字符串结束符
    {
        if (*p >= 0x80)  // 判断是否为中文字符（ASCII码大于等于0x80）
        {
            width += 12;  // 中文字符宽度为12像素
            p += 2;  // 指针向后移动2个字节（中文字符占2个字节）
        }
        else  // ASCII字符
        {
            width += 6;  // ASCII字符宽度为6像素
            p++;  // 指针向后移动1个字节
        }
    }
    
    OLED_ReverseArea(x, y, width, fontSize);  // 反色显示指定区域，实现标题反色效果
}

/**
 * @brief 渲染阈值设置页面1（温度、湿度、光照）
 * 
 * @note 显示内容：
 *       - 标题：报警阈值(1/2)
 *       - 温度范围：minTemperature ~ maxTemperature
 *       - 湿度范围：minHumidity ~ maxHumidity
 *       - 光照范围：minLightIntensity ~ maxLightIntensity
 * 
 * @note 下划线位置：
 *       - 根据rangeEditIndex确定当前编辑项
 *       - 每个参数有最小值和最大值两个编辑项
 *       - 下划线位置动态计算，确保对齐
 */
static void renderRangePage1(void)
{
    char buffer[32];  // 定义字符串缓冲区，用于格式化显示文本
    int minInt, minDec, maxInt, maxDec;  // 定义整数部分和小数部分变量，用于浮点数拆分
    uint8_t minLen, maxLen;  // 定义字符串长度变量，用于计算下划线位置

    OLED_ShowString(28, 0, "报警阈值(1/2)", OLED_12X12);  // 在坐标(28,0)显示标题文本"报警阈值(1/2)"，字体大小为12x12
    reverseTitle(28, 0, "报警阈值(1/2)", OLED_12X12);  // 反色显示标题区域，实现标题反色效果

    floatToIntDec(farmSafeRange.minTemperature, &minInt, &minDec);  // 将温度最小值拆分为整数部分和小数部分
    floatToIntDec(farmSafeRange.maxTemperature, &maxInt, &maxDec);  // 将温度最大值拆分为整数部分和小数部分
    minLen = getIntLen(minInt) + 2;  // 计算温度最小值字符串长度（整数长度+2，2表示小数点和一位小数）
    maxLen = getIntLen(maxInt) + 2;  // 计算温度最大值字符串长度（整数长度+2，2表示小数点和一位小数）

    sprintf(buffer, "%d.%d<温度<%d.%dC  ", minInt, minDec, maxInt, maxDec);  // 格式化温度范围字符串
    OLED_ShowString(20, 15, buffer, OLED_12X12);  // 在坐标(20,15)显示温度范围文本，字体大小为12x12

    drawUnderline(20, 28, minLen * 6, (rangeEditIndex == RANGE_EDIT_TEMPERATURE_MIN));  // 在温度最小值下方绘制下划线，如果当前编辑项是温度最小值则激活
    drawUnderline(20 + minLen * 6 + 36, 28, maxLen * 6, (rangeEditIndex == RANGE_EDIT_TEMPERATURE_MAX));  // 在温度最大值下方绘制下划线，如果当前编辑项是温度最大值则激活

    floatToIntDec(farmSafeRange.minHumidity, &minInt, &minDec);  // 将湿度最小值拆分为整数部分和小数部分
    floatToIntDec(farmSafeRange.maxHumidity, &maxInt, &maxDec);  // 将湿度最大值拆分为整数部分和小数部分
    minLen = getIntLen(minInt) + 2;  // 计算湿度最小值字符串长度（整数长度+2，2表示小数点和一位小数）
    maxLen = getIntLen(maxInt) + 2;  // 计算湿度最大值字符串长度（整数长度+2，2表示小数点和一位小数）

    sprintf(buffer, "%d.%d<湿度<%d.%d%% ", minInt, minDec, maxInt, maxDec);  // 格式化湿度范围字符串
    OLED_ShowString(20, 30, buffer, OLED_12X12);  // 在坐标(20,30)显示湿度范围文本，字体大小为12x12

    drawUnderline(20, 43, minLen * 6, (rangeEditIndex == RANGE_EDIT_HUMIDITY_MIN));  // 在湿度最小值下方绘制下划线，如果当前编辑项是湿度最小值则激活
    drawUnderline(20 + minLen * 6 + 36, 43, maxLen * 6, (rangeEditIndex == RANGE_EDIT_HUMIDITY_MAX));  // 在湿度最大值下方绘制下划线，如果当前编辑项是湿度最大值则激活

    minLen = getIntLen(farmSafeRange.minLightIntensity);  // 计算光照最小值字符串长度
    maxLen = getIntLen(farmSafeRange.maxLightIntensity);  // 计算光照最大值字符串长度

    sprintf(buffer, "%d<光照<%d    ", farmSafeRange.minLightIntensity, farmSafeRange.maxLightIntensity);  // 格式化光照范围字符串
    OLED_ShowString(20, 45, buffer, OLED_12X12);  // 在坐标(20,45)显示光照范围文本，字体大小为12x12

    drawUnderline(20, 58, minLen * 6, (rangeEditIndex == RANGE_EDIT_LIGHT_INTENSITY_MIN));  // 在光照最小值下方绘制下划线，如果当前编辑项是光照最小值则激活
    drawUnderline(20 + minLen * 6 + 36, 58, maxLen * 6, (rangeEditIndex == RANGE_EDIT_LIGHT_INTENSITY_MAX));  // 在光照最大值下方绘制下划线，如果当前编辑项是光照最大值则激活
}

/**
 * @brief 渲染阈值设置页面2（土壤湿度、降雨量）
 * 
 * @note 显示内容：
 *       - 标题：报警阈值(2/2)
 *       - 土壤湿度范围：minSoilMoisture ~ maxSoilMoisture
 *       - 降雨量上限：maxRainGauge
 */
static void renderRangePage2(void)
{
    char buffer[32];  // 定义字符串缓冲区，用于格式化显示文本
    uint8_t minLen, maxLen;  // 定义字符串长度变量，用于计算下划线位置

    OLED_ShowString(28, 0, "报警阈值(2/2)", OLED_12X12);  // 在坐标(28,0)显示标题文本"报警阈值(2/2)"，字体大小为12x12
    reverseTitle(28, 0, "报警阈值(2/2)", OLED_12X12);  // 反色显示标题区域，实现标题反色效果

    minLen = getIntLen(farmSafeRange.minSoilMoisture);  // 计算土壤湿度最小值字符串长度
    maxLen = getIntLen(farmSafeRange.maxSoilMoisture);  // 计算土壤湿度最大值字符串长度

    sprintf(buffer, "%d<土壤<%d%%   ", farmSafeRange.minSoilMoisture, farmSafeRange.maxSoilMoisture);  // 格式化土壤湿度范围字符串
    OLED_ShowString(20, 15, buffer, OLED_12X12);  // 在坐标(20,15)显示土壤湿度范围文本，字体大小为12x12

    drawUnderline(20, 28, minLen * 6, (rangeEditIndex == RANGE_EDIT_SOIL_MOISTURE_MIN));  // 在土壤湿度最小值下方绘制下划线，如果当前编辑项是土壤湿度最小值则激活
    drawUnderline(20 + minLen * 6 + 36, 28, maxLen * 6, (rangeEditIndex == RANGE_EDIT_SOIL_MOISTURE_MAX));  // 在土壤湿度最大值下方绘制下划线，如果当前编辑项是土壤湿度最大值则激活

    maxLen = getIntLen(farmSafeRange.maxRainGauge);  // 计算降雨量最大值字符串长度

    sprintf(buffer, "降雨<%d%%      ", farmSafeRange.maxRainGauge);  // 格式化降雨量字符串
    OLED_ShowString(20, 30, buffer, OLED_12X12);  // 在坐标(20,30)显示降雨量文本，字体大小为12x12

    drawUnderline(50, 43, maxLen * 6, (rangeEditIndex == RANGE_EDIT_RAIN_GAUGE_MAX));  // 在降雨量下方绘制下划线，如果当前编辑项是降雨量最大值则激活
}

/**
 * @brief OLED显示任务
 * @param pvParameters 任务参数（未使用）
 * 
 * @note 任务功能：
 *       1. 从PageEventQueue接收页面切换事件
 *       2. 从SensorDataQueue读取传感器数据显示
 *       3. 根据currentPage渲染不同页面
 *       4. 编辑模式下定时刷新实现闪烁效果
 *       5. 使用互斥锁保护OLED资源
 * 
 * @note FreeRTOS任务特性：
 *       - 任务优先级：2
 *       - 任务栈大小：TASK_STACK_SIZE_OLED
 *       - 任务周期：50ms（显示刷新周期）
 *       - 使用vTaskDelay实现周期性延时
 * 
 * @note 队列通信：
 *       - PageEventQueue：接收页面切换事件（非阻塞，超时0）
 *       - SensorDataQueue：读取传感器数据（非阻塞，超时0，使用xQueuePeek不删除数据）
 * 
 * @note 互斥锁使用：
 *       - 使用OLED_Mutex保护OLED显示资源
 *       - 获取锁超时时间：5ms
 *       - 获取锁成功后执行显示操作，完成后释放锁
 *       - 如果获取锁失败，跳过本次刷新
 * 
 * @note 显示页面说明：
 *       PAGE_HOME（主页）：
 *       - 显示所有传感器实时数据
 *       - 温度、湿度、光照、土壤湿度、降雨量、水泵状态
 *       - 数据从SensorDataQueue读取
 *       
 *       PAGE_RANGE（阈值页）：
 *       - 显示阈值设置界面
 *       - 分两页显示：页面1（温度、湿度、光照）、页面2（土壤、降雨）
 *       - 编辑模式下当前编辑项闪烁
 *       - 根据rangeEditIndex确定当前编辑项
 * 
 * @note 刷新机制：
 *       - oled_dirty标志触发刷新
 *       - 页面切换时设置need_clear标志清屏
 *       - 编辑模式下每250ms刷新一次实现闪烁
 *       - 非编辑模式下仅在数据更新时刷新
 */
void OLED_Task(void *pvParameters)
{
    char buffer[32];  // 定义字符串缓冲区，用于格式化显示文本
    SensorData_t sensor_data;  // 定义传感器数据结构体变量，用于存储从队列读取的传感器数据
    DisplayPage_t received_page;  // 定义页面变量，用于存储从队列接收的页面索引
    static uint8_t need_clear = 0;  // 定义静态变量，清屏标志，1表示需要清屏，0表示不需要清屏
    static TickType_t last_blink_tick = 0;  // 定义静态变量，记录上一次闪烁的时间戳
    
    while(1)  // 无限循环，任务持续运行
    {
        if (xQueueReceive(PageEventQueue, &received_page, 0) == pdTRUE)  // 尝试从PageEventQueue接收页面切换事件，超时为0（非阻塞）
        {
            currentPage = received_page;  // 更新当前页面索引为接收到的页面索引
            need_clear = 1;  // 设置清屏标志为1，表示需要清屏
            oled_dirty = 1;  // 设置OLED显示脏标志为1，通知OLED任务需要刷新显示
        }

        if (currentPage == PAGE_RANGE && rangeEditState == RANGE_EDIT_STATE_EDITING)  // 判断当前是否在阈值页面且处于编辑模式
        {
            if (xTaskGetTickCount() - last_blink_tick >= pdMS_TO_TICKS(250))  // 判断距离上一次闪烁是否超过250毫秒
            {
                oled_dirty = 1;  // 设置OLED显示脏标志为1，通知OLED任务需要刷新显示
                last_blink_tick = xTaskGetTickCount();  // 更新上一次闪烁的时间戳为当前时间
            }
        }

        if (oled_dirty)  // 判断OLED显示脏标志是否为1（需要刷新显示）
        {
            if (xSemaphoreTake(OLED_Mutex, pdMS_TO_TICKS(5)) == pdTRUE)  // 尝试获取OLED互斥锁，超时为5毫秒
            {
                if (need_clear || currentPage == PAGE_RANGE)  // 判断是否需要清屏或当前页面是阈值页
                {
                    OLED_Clear();  // 清空OLED屏幕
                    need_clear = 0;  // 清除清屏标志，设置为0（不需要清屏）
                }

                if (currentPage == PAGE_RANGE)  // 判断当前页面是否为阈值页
                {
                    if (rangeEditIndex <= RANGE_EDIT_LIGHT_INTENSITY_MAX)  // 判断当前编辑项是否在页面1范围内（温度、湿度、光照）
                    {
                        renderRangePage1();  // 渲染阈值设置页面1
                    }
                    else  // 当前编辑项在页面2范围内（土壤湿度、降雨量）
                    {
                        renderRangePage2();  // 渲染阈值设置页面2
                    }
                }
                else if (currentPage == PAGE_HOME)  // 判断当前页面是否为主页
                {
                    if (xQueuePeek(SensorDataQueue, &sensor_data, 0) == pdTRUE)  // 尝试从SensorDataQueue读取传感器数据，超时为0（非阻塞，不删除队列数据）
                    {
                        OLED_ShowString(30, 0, "Smart Farm", OLED_12X12);  // 在坐标(30,0)显示标题"Smart Farm"，字体大小为12x12
                        reverseTitle(30, 0, "Smart Farm", OLED_12X12);  // 反色显示标题区域，实现标题反色效果
                        
                        OLED_ShowString(9, 14, "温度", OLED_12X12);  // 在坐标(9,14)显示标签"温度"，字体大小为12x12
                        sprintf(buffer, "%d.%dC   ", (int)sensor_data.temp0, (int)((sensor_data.temp0 - (int)sensor_data.temp0) * 10));  // 格式化温度字符串，保留一位小数
                        OLED_ShowString(6, 26, buffer, OLED_12X12);  // 在坐标(6,26)显示温度值，字体大小为12x12
                        
                        OLED_ShowString(52, 14, "湿度", OLED_12X12);  // 在坐标(52,14)显示标签"湿度"，字体大小为12x12
                        sprintf(buffer, "%d.%d%%  ", (int)sensor_data.humi, (int)((sensor_data.humi - (int)sensor_data.humi) * 10));  // 格式化湿度字符串，保留一位小数
                        OLED_ShowString(52-3, 26, buffer, OLED_12X12);  // 在坐标(49,26)显示湿度值，字体大小为12x12
                        
                        OLED_ShowString(95, 14, "光照", OLED_12X12);  // 在坐标(95,14)显示标签"光照"，字体大小为12x12
                        sprintf(buffer, "%dls    ", sensor_data.light);  // 格式化光照字符串
                        OLED_ShowString(95-3, 26, buffer, OLED_12X12);  // 在坐标(92,26)显示光照值，字体大小为12x12
                        
                        OLED_ShowString(9, 41, "土壤", OLED_12X12);  // 在坐标(9,41)显示标签"土壤"，字体大小为12x12
                        sprintf(buffer, "%d%%   ", sensor_data.soil);  // 格式化土壤湿度字符串
                        OLED_ShowString(9+6, 53, buffer, OLED_12X12);  // 在坐标(15,53)显示土壤湿度值，字体大小为12x12
                        
                        OLED_ShowString(52, 41, "降雨", OLED_12X12);  // 在坐标(52,41)显示标签"降雨"，字体大小为12x12
                        sprintf(buffer, "%d%%   ", sensor_data.rain);  // 格式化降雨量字符串
                        OLED_ShowString(58, 53, buffer, OLED_12X12);  // 在坐标(58,53)显示降雨量值，字体大小为12x12
                        
                        OLED_ShowString(95, 41, "水泵", OLED_12X12);  // 在坐标(95,41)显示标签"水泵"，字体大小为12x12
                        OLED_ShowString(101, 53, pumpState ? "开" : "关", OLED_12X12);  // 在坐标(101,53)显示水泵状态，根据pumpState显示"开"或"关"，字体大小为12x12
                    }
                }
                
                OLED_Update();  // 更新OLED显示，将缓冲区内容发送到OLED屏幕
                xSemaphoreGive(OLED_Mutex);  // 释放OLED互斥锁，允许其他任务访问OLED
            }
            oled_dirty = 0;  // 清除OLED显示脏标志，设置为0（不需要刷新显示）
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));  // 延时50毫秒，释放CPU资源，实现周期性显示刷新
    }
}
