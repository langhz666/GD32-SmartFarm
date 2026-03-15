/*
 * @Author: langhz666 3204498297@qq.com
 * @Date: 2026-03-15 00:08:13
 * @LastEditors: langhz666 3204498297@qq.com
 * @LastEditTime: 2026-03-15 21:35:00
 * @FilePath: \GD32F103C8T6\App\task_sensor.c
 * @Description: 传感器数据采集任务，负责读取所有传感器数据、阈值检测、报警处理和蓝牙通知
 */

#include "app_shared.h"              // 包含应用层共享头文件，定义全局变量和任务函数声明
#include "driver_aht20/driver_aht20.h"  // 包含AHT20温湿度传感器驱动头文件，提供温湿度读取函数
#include "driver_bmp280/driver_bmp280.h"  // 包含BMP280气压传感器驱动头文件，提供气压读取函数
#include "driver_light/driver_light.h"    // 包含光照传感器驱动头文件，提供光照读取函数
#include "driver_soil/driver_soil.h"      // 包含土壤湿度传感器驱动头文件，提供土壤湿度读取函数
#include "driver_rain/driver_rain.h"      // 包含降雨量传感器驱动头文件，提供降雨量读取函数
#include "driver_buzzer/driver_buzzer.h"  // 包含蜂鸣器驱动头文件，提供蜂鸣器控制函数
#include "driver_led/driver_led.h"        // 包含LED驱动头文件，提供LED控制函数
#include "driver_bluetooth/driver_bluetooth.h"  // 包含蓝牙驱动头文件，提供蓝牙发送函数
#include "delay.h"                        // 包含延时函数头文件，提供延时函数
#include <stdio.h>                        // 包含标准输入输出头文件，提供sprintf等函数

/**
 * @brief 传感器数据采集任务
 * @param pvParameters 任务参数（未使用）
 * 
 * @note 任务功能：
 *       1. 采集所有传感器数据（温湿度、气压、光照、土壤湿度、降雨量）
 *       2. 将数据发送到队列供OLED显示
 *       3. 自动灌溉控制（土壤湿度低于阈值时开启水泵）
 *       4. 阈值检测和报警（通过蓝牙发送报警信息）
 *       5. 蜂鸣器报警提示
 * 
 * @note FreeRTOS任务特性：
 *       - 任务优先级：1（最低优先级）
 *       - 任务栈大小：TASK_STACK_SIZE_SENSOR
 *       - 任务周期：1000ms（1秒采集一次）
 *       - 使用vTaskDelay实现周期性延时，不占用CPU资源
 * 
 * @note 队列通信：
 *       - 使用xQueueOverwrite()向SensorDataQueue发送数据
 *       - 队列长度为1，始终保存最新数据
 *       - 不需要等待队列空间，直接覆盖旧数据
 * 
 * @note 报警机制：
 *       - 检测温度、湿度、光照、土壤湿度、降雨量是否超出阈值
 *       - 超出阈值时通过蓝牙发送报警信息到手机
 *       - 蜂鸣器滴滴3次提示报警
 *       - 报警信息格式："ALARM: Parameter too high/low! value > threshold"
 */
void Sensor_Task(void *pvParameters)
{
    SensorData_t data;  // 定义传感器数据结构体变量，用于存储采集到的传感器数据

    while(1)  // 无限循环，任务持续运行
    {
        AHT20_Read_Temp_Humi(&data.temp0, &data.humi);  // 从AHT20传感器读取温度和湿度数据，保存到data结构体的temp0和humi字段
        BMP280_Read_Temp_Press(&data.temp1, &data.press);  // 从BMP280传感器读取温度和气压数据，保存到data结构体的temp1和press字段
        data.light = Light_Get();  // 从光照传感器读取光照强度数据，保存到data结构体的light字段
        data.soil = Get_Soil_humidity();  // 从土壤湿度传感器读取土壤湿度数据，保存到data结构体的soil字段
        data.rain = Get_Rain_size();  // 从降雨量传感器读取降雨量数据，保存到data结构体的rain字段

        xQueueOverwrite(SensorDataQueue, &data);  // 将传感器数据发送到SensorDataQueue队列，使用覆盖模式，始终保存最新数据
        oled_dirty = 1;  // 设置OLED显示脏标志为1，通知OLED任务需要刷新显示
        
        if (data.soil < farmSafeRange.minSoilMoisture)  // 判断土壤湿度是否低于最小阈值
        {
            gpio_bit_write(LED2_GPIO_PORT, LED2_GPIO_PIN, RESET);  // 点亮LED2，指示水泵开启状态
            if (pumpState != 1)  // 判断水泵状态是否未开启
            {
                pumpState = 1;  // 设置水泵状态为1（开启）
                oled_dirty = 1;  // 设置OLED显示脏标志为1，通知OLED任务需要刷新显示
            }
        }
        else  // 土壤湿度在正常范围内
        {
            gpio_bit_write(LED2_GPIO_PORT, LED2_GPIO_PIN, SET);  // 熄灭LED2，指示水泵关闭状态
            if (pumpState != 0)  // 判断水泵状态是否未关闭
            {
                pumpState = 0;  // 设置水泵状态为0（关闭）
                oled_dirty = 1;  // 设置OLED显示脏标志为1，通知OLED任务需要刷新显示
            }
        }
        
        uint8_t alarmFlag = 0;  // 定义报警标志变量，初始值为0（无报警）
        char alarmMsg[128];  // 定义报警消息缓冲区，大小为128字节
        
        if (data.temp0 < farmSafeRange.minTemperature)  // 判断温度是否低于最小阈值
        {
            sprintf(alarmMsg, "ALARM: Temperature too low! %.1fC < %.1fC\r\n", data.temp0, farmSafeRange.minTemperature);  // 格式化报警消息，包含当前温度和阈值
            blt_send_string(alarmMsg);  // 通过蓝牙发送报警消息到手机
            alarmFlag = 1;  // 设置报警标志为1（有报警）
        }
        else if (data.temp0 > farmSafeRange.maxTemperature)  // 判断温度是否高于最大阈值
        {
            sprintf(alarmMsg, "ALARM: Temperature too high! %.1fC > %.1fC\r\n", data.temp0, farmSafeRange.maxTemperature);  // 格式化报警消息，包含当前温度和阈值
            blt_send_string(alarmMsg);  // 通过蓝牙发送报警消息到手机
            alarmFlag = 1;  // 设置报警标志为1（有报警）
        }
        
        if (data.humi < farmSafeRange.minHumidity)  // 判断湿度是否低于最小阈值
        {
            sprintf(alarmMsg, "ALARM: Humidity too low! %.1f%% < %.1f%%\r\n", data.humi, farmSafeRange.minHumidity);  // 格式化报警消息，包含当前湿度和阈值
            blt_send_string(alarmMsg);  // 通过蓝牙发送报警消息到手机
            alarmFlag = 1;  // 设置报警标志为1（有报警）
        }
        else if (data.humi > farmSafeRange.maxHumidity)  // 判断湿度是否高于最大阈值
        {
            sprintf(alarmMsg, "ALARM: Humidity too high! %.1f%% > %.1f%%\r\n", data.humi, farmSafeRange.maxHumidity);  // 格式化报警消息，包含当前湿度和阈值
            blt_send_string(alarmMsg);  // 通过蓝牙发送报警消息到手机
            alarmFlag = 1;  // 设置报警标志为1（有报警）
        }
        
        if (data.light < farmSafeRange.minLightIntensity)  // 判断光照是否低于最小阈值
        {
            sprintf(alarmMsg, "ALARM: Light too low! %d < %d\r\n", data.light, farmSafeRange.minLightIntensity);  // 格式化报警消息，包含当前光照和阈值
            blt_send_string(alarmMsg);  // 通过蓝牙发送报警消息到手机
            alarmFlag = 1;  // 设置报警标志为1（有报警）
        }
        else if (data.light > farmSafeRange.maxLightIntensity)  // 判断光照是否高于最大阈值
        {
            sprintf(alarmMsg, "ALARM: Light too high! %d > %d\r\n", data.light, farmSafeRange.maxLightIntensity);  // 格式化报警消息，包含当前光照和阈值
            blt_send_string(alarmMsg);  // 通过蓝牙发送报警消息到手机
            alarmFlag = 1;  // 设置报警标志为1（有报警）
        }
        
        if (data.soil < farmSafeRange.minSoilMoisture)  // 判断土壤湿度是否低于最小阈值
        {
            sprintf(alarmMsg, "ALARM: Soil too dry! %d%% < %d%%\r\n", data.soil, farmSafeRange.minSoilMoisture);  // 格式化报警消息，包含当前土壤湿度和阈值
            blt_send_string(alarmMsg);  // 通过蓝牙发送报警消息到手机
            alarmFlag = 1;  // 设置报警标志为1（有报警）
        }
        else if (data.soil > farmSafeRange.maxSoilMoisture)  // 判断土壤湿度是否高于最大阈值
        {
            sprintf(alarmMsg, "ALARM: Soil too wet! %d%% > %d%%\r\n", data.soil, farmSafeRange.maxSoilMoisture);  // 格式化报警消息，包含当前土壤湿度和阈值
            blt_send_string(alarmMsg);  // 通过蓝牙发送报警消息到手机
            alarmFlag = 1;  // 设置报警标志为1（有报警）
        }
        
        if (data.rain > farmSafeRange.maxRainGauge)  // 判断降雨量是否高于最大阈值
        {
            sprintf(alarmMsg, "ALARM: Rain too high! %d%% > %d%%\r\n", data.rain, farmSafeRange.maxRainGauge);  // 格式化报警消息，包含当前降雨量和阈值
            blt_send_string(alarmMsg);  // 通过蓝牙发送报警消息到手机
            alarmFlag = 1;  // 设置报警标志为1（有报警）
        }
        
        if (alarmFlag)  // 判断是否有报警
        {
            for (uint8_t i = 0; i < 3; i++)  // 循环3次，产生3声蜂鸣器报警
            {
                Buzzer_On();  // 开启蜂鸣器
                DelayNms(150);  // 延时150毫秒
                Buzzer_Off();  // 关闭蜂鸣器
                DelayNms(100);  // 延时100毫秒
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));  // 延时1000毫秒（1秒），释放CPU资源，实现周期性采集
    }
}
