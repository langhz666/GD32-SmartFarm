/*
 * @Author: langhz666 3204498297@qq.com
 * @Date: 2026-03-15 00:08:13
 * @LastEditors: langhz666 3204498297@qq.com
 * @LastEditTime: 2026-03-15 20:38:45
 * @FilePath: \GD32F103C8T6\App\task_sensor.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "app_shared.h"
#include "driver_aht20/driver_aht20.h"
#include "driver_bmp280/driver_bmp280.h"
#include "driver_light/driver_light.h"
#include "driver_soil/driver_soil.h"
#include "driver_rain/driver_rain.h"
#include "driver_buzzer/driver_buzzer.h"
#include "driver_led/driver_led.h"
#include "driver_bluetooth/driver_bluetooth.h"
#include "delay.h"
#include <stdio.h>

void Sensor_Task(void *pvParameters)
{
    SensorData_t data;

    while(1)
    {
        AHT20_Read_Temp_Humi(&data.temp0, &data.humi);
        BMP280_Read_Temp_Press(&data.temp1, &data.press);
        data.light = Light_Get();
        data.soil = Get_Soil_humidity();
        data.rain = Get_Rain_size();

        xQueueOverwrite(SensorDataQueue, &data);
        oled_dirty = 1;
        
        if (data.soil < farmSafeRange.minSoilMoisture)
        {
            gpio_bit_write(LED2_GPIO_PORT, LED2_GPIO_PIN, RESET);
            if (pumpState != 1)
            {
                pumpState = 1;
                oled_dirty = 1;
            }
        }
        else
        {
            gpio_bit_write(LED2_GPIO_PORT, LED2_GPIO_PIN, SET);
            if (pumpState != 0)
            {
                pumpState = 0;
                oled_dirty = 1;
            }
        }
        
        uint8_t alarmFlag = 0;
        char alarmMsg[128];
        
        if (data.temp0 < farmSafeRange.minTemperature)
        {
            sprintf(alarmMsg, "ALARM: Temperature too low! %.1fC < %.1fC\r\n", data.temp0, farmSafeRange.minTemperature);
            blt_send_string(alarmMsg);
            alarmFlag = 1;
        }
        else if (data.temp0 > farmSafeRange.maxTemperature)
        {
            sprintf(alarmMsg, "ALARM: Temperature too high! %.1fC > %.1fC\r\n", data.temp0, farmSafeRange.maxTemperature);
            blt_send_string(alarmMsg);
            alarmFlag = 1;
        }
        
        if (data.humi < farmSafeRange.minHumidity)
        {
            sprintf(alarmMsg, "ALARM: Humidity too low! %.1f%% < %.1f%%\r\n", data.humi, farmSafeRange.minHumidity);
            blt_send_string(alarmMsg);
            alarmFlag = 1;
        }
        else if (data.humi > farmSafeRange.maxHumidity)
        {
            sprintf(alarmMsg, "ALARM: Humidity too high! %.1f%% > %.1f%%\r\n", data.humi, farmSafeRange.maxHumidity);
            blt_send_string(alarmMsg);
            alarmFlag = 1;
        }
        
        if (data.light < farmSafeRange.minLightIntensity)
        {
            sprintf(alarmMsg, "ALARM: Light too low! %d < %d\r\n", data.light, farmSafeRange.minLightIntensity);
            blt_send_string(alarmMsg);
            alarmFlag = 1;
        }
        else if (data.light > farmSafeRange.maxLightIntensity)
        {
            sprintf(alarmMsg, "ALARM: Light too high! %d > %d\r\n", data.light, farmSafeRange.maxLightIntensity);
            blt_send_string(alarmMsg);
            alarmFlag = 1;
        }
        
        if (data.soil < farmSafeRange.minSoilMoisture)
        {
            sprintf(alarmMsg, "ALARM: Soil too dry! %d%% < %d%%\r\n", data.soil, farmSafeRange.minSoilMoisture);
            blt_send_string(alarmMsg);
            alarmFlag = 1;
        }
        else if (data.soil > farmSafeRange.maxSoilMoisture)
        {
            sprintf(alarmMsg, "ALARM: Soil too wet! %d%% > %d%%\r\n", data.soil, farmSafeRange.maxSoilMoisture);
            blt_send_string(alarmMsg);
            alarmFlag = 1;
        }
        
        if (data.rain > farmSafeRange.maxRainGauge)
        {
            sprintf(alarmMsg, "ALARM: Rain too high! %d%% > %d%%\r\n", data.rain, farmSafeRange.maxRainGauge);
            blt_send_string(alarmMsg);
            alarmFlag = 1;
        }
        
        if (alarmFlag)
        {
            for (uint8_t i = 0; i < 3; i++)
            {
                Buzzer_On();
                DelayNms(150);
                Buzzer_Off();
                DelayNms(100);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
