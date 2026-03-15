/*
 * @Author: langhz666 3204498297@qq.com
 * @Date: 2026-03-13 18:04:36
 * @LastEditors: langhz666 3204498297@qq.com
 * @LastEditTime: 2026-03-15 21:48:00
 * @FilePath: \GD32F103C8T6\App\task_encoder.c
 * @Description: 编码器处理任务，负责编码器旋转检测、阈值编辑项选择和阈值数值调整
 */

#include "app_shared.h"              // 包含应用层共享头文件，定义全局变量和任务函数声明
#include "driver_encoder/driver_encoder.h"  // 包含编码器驱动头文件，提供编码器读取函数

/**
 * @brief 处理阈值数值编辑
 * @param direction 编码器旋转方向（1=顺时针增加，-1=逆时针减少）
 * 
 * @note 功能说明：
 *       - 根据rangeEditIndex确定当前编辑的阈值项
 *       - 根据编码器方向调整阈值数值
 *       - 自动进行边界检查和逻辑约束
 * 
 * @note 阈值调整步长：
 *       - 温度：±0.5°C
 *       - 湿度：±0.5%
 *       - 光照：±5
 *       - 土壤湿度：±1%
 *       - 降雨量：±1%
 * 
 * @note 边界约束：
 *       - 温度范围：0~100°C，最小值<最大值
 *       - 湿度范围：0~100%，最小值<最大值
 *       - 光照范围：0~65535，最小值<最大值
 *       - 土壤湿度范围：0~100%，最小值<最大值
 *       - 降雨量范围：0~100%
 */
static void handleEditValueChange(int8_t direction)
{
    float delta = (direction > 0) ? 0.5f : -0.5f;  // 定义浮点数增量，顺时针为+0.5，逆时针为-0.5
    int16_t intDelta = (direction > 0) ? 5 : -5;  // 定义整数增量，顺时针为+5，逆时针为-5
    
    switch (rangeEditIndex)  // 根据当前编辑项索引进行分支处理
    {
    case RANGE_EDIT_TEMPERATURE_MIN:  // 编辑温度最小值
        farmSafeRange.minTemperature += delta;  // 温度最小值增加或减少0.5°C
        if (farmSafeRange.minTemperature < 0) farmSafeRange.minTemperature = 0;  // 温度最小值不能小于0°C
        if (farmSafeRange.minTemperature >= farmSafeRange.maxTemperature)  // 检查温度最小值是否大于等于最大值
            farmSafeRange.minTemperature = farmSafeRange.maxTemperature - 0.5f;  // 温度最小值必须小于最大值，设置为最大值-0.5
        break;
    case RANGE_EDIT_TEMPERATURE_MAX:  // 编辑温度最大值
        farmSafeRange.maxTemperature += delta;  // 温度最大值增加或减少0.5°C
        if (farmSafeRange.maxTemperature > 100) farmSafeRange.maxTemperature = 100;  // 温度最大值不能大于100°C
        if (farmSafeRange.maxTemperature <= farmSafeRange.minTemperature)  // 检查温度最大值是否小于等于最小值
            farmSafeRange.maxTemperature = farmSafeRange.minTemperature + 0.5f;  // 温度最大值必须大于最小值，设置为最小值+0.5
        break;
    case RANGE_EDIT_HUMIDITY_MIN:  // 编辑湿度最小值
        farmSafeRange.minHumidity += delta;  // 湿度最小值增加或减少0.5%
        if (farmSafeRange.minHumidity < 0) farmSafeRange.minHumidity = 0;  // 湿度最小值不能小于0%
        if (farmSafeRange.minHumidity >= farmSafeRange.maxHumidity)  // 检查湿度最小值是否大于等于最大值
            farmSafeRange.minHumidity = farmSafeRange.maxHumidity - 0.5f;  // 湿度最小值必须小于最大值，设置为最大值-0.5
        break;
    case RANGE_EDIT_HUMIDITY_MAX:  // 编辑湿度最大值
        farmSafeRange.maxHumidity += delta;  // 湿度最大值增加或减少0.5%
        if (farmSafeRange.maxHumidity > 100) farmSafeRange.maxHumidity = 100;  // 湿度最大值不能大于100%
        if (farmSafeRange.maxHumidity <= farmSafeRange.minHumidity)  // 检查湿度最大值是否小于等于最小值
            farmSafeRange.maxHumidity = farmSafeRange.minHumidity + 0.5f;  // 湿度最大值必须大于最小值，设置为最小值+0.5
        break;
    case RANGE_EDIT_LIGHT_INTENSITY_MIN:  // 编辑光照最小值
        if (intDelta < 0 && farmSafeRange.minLightIntensity < (uint16_t)(-intDelta))  // 检查是否逆时针旋转且光照最小值小于5
        {
            farmSafeRange.minLightIntensity = 0;  // 光照最小值设置为0，避免下溢
        }
        else  // 正常情况
        {
            farmSafeRange.minLightIntensity += intDelta;  // 光照最小值增加或减少5
        }
        if (farmSafeRange.minLightIntensity >= farmSafeRange.maxLightIntensity)  // 检查光照最小值是否大于等于最大值
            farmSafeRange.minLightIntensity = farmSafeRange.maxLightIntensity - 5;  // 光照最小值必须小于最大值，设置为最大值-5
        break;
    case RANGE_EDIT_LIGHT_INTENSITY_MAX:  // 编辑光照最大值
        farmSafeRange.maxLightIntensity += intDelta;  // 光照最大值增加或减少5
        if (farmSafeRange.maxLightIntensity > 65535) farmSafeRange.maxLightIntensity = 65535;  // 光照最大值不能大于65535
        if (farmSafeRange.maxLightIntensity <= farmSafeRange.minLightIntensity)  // 检查光照最大值是否小于等于最小值
            farmSafeRange.maxLightIntensity = farmSafeRange.minLightIntensity + 5;  // 光照最大值必须大于最小值，设置为最小值+5
        break;
    case RANGE_EDIT_SOIL_MOISTURE_MIN:  // 编辑土壤湿度最小值
        if (direction < 0 && farmSafeRange.minSoilMoisture == 0)  // 检查是否逆时针旋转且土壤湿度最小值为0
        {
            // 不执行任何操作，保持土壤湿度最小值为0
        }
        else  // 正常情况
        {
            farmSafeRange.minSoilMoisture += (direction > 0) ? 1 : -1;  // 土壤湿度最小值增加或减少1%
        }
        if (farmSafeRange.minSoilMoisture >= farmSafeRange.maxSoilMoisture)  // 检查土壤湿度最小值是否大于等于最大值
            farmSafeRange.minSoilMoisture = farmSafeRange.maxSoilMoisture - 1;  // 土壤湿度最小值必须小于最大值，设置为最大值-1
        break;
    case RANGE_EDIT_SOIL_MOISTURE_MAX:  // 编辑土壤湿度最大值
        farmSafeRange.maxSoilMoisture += (direction > 0) ? 1 : -1;  // 土壤湿度最大值增加或减少1%
        if (farmSafeRange.maxSoilMoisture > 100) farmSafeRange.maxSoilMoisture = 100;  // 土壤湿度最大值不能大于100%
        if (farmSafeRange.maxSoilMoisture <= farmSafeRange.minSoilMoisture)  // 检查土壤湿度最大值是否小于等于最小值
            farmSafeRange.maxSoilMoisture = farmSafeRange.minSoilMoisture + 1;  // 土壤湿度最大值必须大于最小值，设置为最小值+1
        break;
    case RANGE_EDIT_RAIN_GAUGE_MAX:  // 编辑降雨量最大值
        if (direction < 0 && farmSafeRange.maxRainGauge == 0)  // 检查是否逆时针旋转且降雨量最大值为0
        {
            // 不执行任何操作，保持降雨量最大值为0
        }
        else  // 正常情况
        {
            farmSafeRange.maxRainGauge += (direction > 0) ? 1 : -1;  // 降雨量最大值增加或减少1%
        }
        if (farmSafeRange.maxRainGauge > 100) farmSafeRange.maxRainGauge = 100;  // 降雨量最大值不能大于100%
        break;
    default:  // 未知编辑项
        break;  // 不执行任何操作
    }
}

/**
 * @brief 编码器处理任务
 * @param pvParameters 任务参数（未使用）
 * 
 * @note 任务功能：
 *       1. 检测编码器旋转状态
 *       2. 页面切换时初始化编辑状态
 *       3. 浏览模式下：旋转编码器切换编辑项
 *       4. 编辑模式下：旋转编码器调整阈值数值
 *       5. 主页模式下：旋转编码器调整Num值（调试用）
 * 
 * @note FreeRTOS任务特性：
 *       - 任务优先级：2
 *       - 任务栈大小：TASK_STACK_SIZE_ENCODER
 *       - 任务周期：10ms（编码器扫描周期）
 *       - 使用vTaskDelay实现周期性延时
 * 
 * @note 编码器功能说明：
 *       主页模式（PAGE_HOME）：
 *       - 顺时针旋转：Num增加
 *       - 逆时针旋转：Num减少
 *       - 范围：-999 ~ 9999
 *       
 *       阈值页浏览模式（RANGE_EDIT_STATE_BROWSING）：
 *       - 顺时针旋转：切换到下一个编辑项
 *       - 逆时针旋转：切换到上一个编辑项
 *       - 循环切换：最后一项 -> 第一项
 *       
 *       阈值页编辑模式（RANGE_EDIT_STATE_EDITING）：
 *       - 顺时针旋转：增加当前编辑项的数值
 *       - 逆时针旋转：减少当前编辑项的数值
 *       - 自动边界检查和逻辑约束
 * 
 * @note 编辑项顺序：
 *       1. 温度最小值 (RANGE_EDIT_TEMPERATURE_MIN)
 *       2. 温度最大值 (RANGE_EDIT_TEMPERATURE_MAX)
 *       3. 湿度最小值 (RANGE_EDIT_HUMIDITY_MIN)
 *       4. 湿度最大值 (RANGE_EDIT_HUMIDITY_MAX)
 *       5. 光照最小值 (RANGE_EDIT_LIGHT_INTENSITY_MIN)
 *       6. 光照最大值 (RANGE_EDIT_LIGHT_INTENSITY_MAX)
 *       7. 土壤湿度最小值 (RANGE_EDIT_SOIL_MOISTURE_MIN)
 *       8. 土壤湿度最大值 (RANGE_EDIT_SOIL_MOISTURE_MAX)
 *       9. 降雨量最大值 (RANGE_EDIT_RAIN_GAUGE_MAX)
 * 
 * @note 全局变量操作：
 *       - currentPage：当前显示页面
 *       - rangeEditIndex：当前编辑项索引
 *       - rangeEditState：编辑状态（浏览/编辑）
 *       - isEditingPage：是否在编辑页面
 *       - oled_dirty：OLED显示脏标志
 *       - Num：主页模式下的数值（调试用）
 */
void Encoder_Task(void *pvParameters)
{
    int8_t encoder_value;  // 定义编码器值变量，用于存储编码器旋转方向和步数
    static DisplayPage_t last_page = PAGE_HOME;  // 定义静态变量，记录上一次的页面索引，用于检测页面切换

    while(1)  // 无限循环，任务持续运行
    {
        encoder_value = Encoder_Get();  // 读取编码器值，返回旋转方向和步数（0=无旋转，正数=顺时针，负数=逆时针）
        
        if (currentPage != last_page)  // 判断当前页面是否发生变化
        {
            last_page = currentPage;  // 更新上一次的页面索引为当前页面
            oled_dirty = 1;  // 设置OLED显示脏标志为1，通知OLED任务需要刷新显示
            
            if (currentPage == PAGE_RANGE)  // 判断当前是否切换到阈值页面
            {
                isEditingPage = 1;  // 设置编辑页面标志为1，表示当前在编辑页面
                rangeEditIndex = RANGE_EDIT_TEMPERATURE_MIN;  // 重置编辑项索引为温度最小值
                rangeEditState = RANGE_EDIT_STATE_BROWSING;  // 重置编辑状态为浏览模式
            }
            else  // 切换到主页
            {
                isEditingPage = 0;  // 设置编辑页面标志为0，表示当前不在编辑页面
            }
        }
        
        if (isEditingPage)  // 判断当前是否在编辑页面
        {
            if (encoder_value != 0)  // 判断编码器是否有旋转
            {
                oled_dirty = 1;  // 设置OLED显示脏标志为1，通知OLED任务需要刷新显示
                if (rangeEditState == RANGE_EDIT_STATE_BROWSING)  // 判断当前是否在浏览模式
                {
                    if (encoder_value > 0)  // 判断编码器是否顺时针旋转
                    {
                        if (rangeEditIndex < RANGE_EDIT_COUNT - 1)  // 判断当前编辑项是否不是最后一项
                        {
                            rangeEditIndex = (RangeEditIndex_t)(rangeEditIndex + 1);  // 编辑项索引加1，切换到下一项
                        }
                        else  // 当前是最后一项
                        {
                            rangeEditIndex = RANGE_EDIT_TEMPERATURE_MIN;  // 循环回到第一项（温度最小值）
                        }
                    }
                    else  // 编码器逆时针旋转
                    {
                        if (rangeEditIndex > 0)  // 判断当前编辑项是否不是第一项
                        {
                            rangeEditIndex = (RangeEditIndex_t)(rangeEditIndex - 1);  // 编辑项索引减1，切换到上一项
                        }
                        else  // 当前是第一项
                        {
                            rangeEditIndex = (RangeEditIndex_t)(RANGE_EDIT_COUNT - 1);  // 循环回到最后一项
                        }
                    }
                }
                else  // 当前在编辑模式
                {
                    handleEditValueChange(encoder_value);  // 调用阈值数值编辑函数，根据编码器方向调整当前编辑项的数值
                }
            }
        }
        else  // 当前在主页
        {
            if (encoder_value != 0)  // 判断编码器是否有旋转
            {
                oled_dirty = 1;  // 设置OLED显示脏标志为1，通知OLED任务需要刷新显示
                Num += encoder_value;  // Num值加上编码器值，实现数值调整
                if (Num > 9999) Num = 9999;  // Num值不能大于9999，超过则设置为9999
                if (Num < -999) Num = -999;  // Num值不能小于-999，超过则设置为-999
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));  // 延时10毫秒，释放CPU资源，实现周期性编码器扫描
    }
}
