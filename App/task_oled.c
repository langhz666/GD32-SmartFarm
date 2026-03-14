#include "app_shared.h"
#include "driver_oled/driver_oled.h"
#include <stdio.h>

static void drawUnderline(uint8_t x, uint8_t y, uint8_t len, uint8_t isActive)
{
    if (isActive)
    {
        uint8_t blink_on = ((xTaskGetTickCount() / 250) % 2 == 0); 
        if (rangeEditState == RANGE_EDIT_STATE_BROWSING || (rangeEditState == RANGE_EDIT_STATE_EDITING && blink_on))
        {
            for (uint8_t i = 0; i < len; i++)
            {
                OLED_DrawPoint(x + i, y);
            }
        }
    }
}

static void renderRangePage1(void)
{
    char buffer[32];
    int minInt, minDec, maxInt, maxDec;
    uint8_t minLen, maxLen;

    OLED_ShowString(28, 0, "报警阈值(1/2)", OLED_12X12);

    floatToIntDec(farmSafeRange.minTemperature, &minInt, &minDec);
    floatToIntDec(farmSafeRange.maxTemperature, &maxInt, &maxDec);
    minLen = getIntLen(minInt) + 2;
    maxLen = getIntLen(maxInt) + 2;

    sprintf(buffer, "%d.%d<温度<%d.%dC  ", minInt, minDec, maxInt, maxDec);
    OLED_ShowString(0, 15, buffer, OLED_12X12);

    drawUnderline(0, 28, minLen * 6, (rangeEditIndex == RANGE_EDIT_TEMPERATURE_MIN));
    drawUnderline(minLen * 6 + 36, 28, maxLen * 6, (rangeEditIndex == RANGE_EDIT_TEMPERATURE_MAX));

    floatToIntDec(farmSafeRange.minHumidity, &minInt, &minDec);
    floatToIntDec(farmSafeRange.maxHumidity, &maxInt, &maxDec);
    minLen = getIntLen(minInt) + 2;
    maxLen = getIntLen(maxInt) + 2;

    sprintf(buffer, "%d.%d<湿度<%d.%d%% ", minInt, minDec, maxInt, maxDec);
    OLED_ShowString(0, 30, buffer, OLED_12X12);

    drawUnderline(0, 43, minLen * 6, (rangeEditIndex == RANGE_EDIT_HUMIDITY_MIN));
    drawUnderline(minLen * 6 + 36, 43, maxLen * 6, (rangeEditIndex == RANGE_EDIT_HUMIDITY_MAX));

    minLen = getIntLen(farmSafeRange.minLightIntensity);
    maxLen = getIntLen(farmSafeRange.maxLightIntensity);

    sprintf(buffer, "%d<光照<%d    ", farmSafeRange.minLightIntensity, farmSafeRange.maxLightIntensity);
    OLED_ShowString(0, 45, buffer, OLED_12X12);

    drawUnderline(0, 58, minLen * 6, (rangeEditIndex == RANGE_EDIT_LIGHT_INTENSITY_MIN));
    drawUnderline(minLen * 6 + 36, 58, maxLen * 6, (rangeEditIndex == RANGE_EDIT_LIGHT_INTENSITY_MAX));
}

static void renderRangePage2(void)
{
    char buffer[32];
    uint8_t minLen, maxLen;

    OLED_ShowString(28, 0, "报警阈值(2/2)", OLED_12X12);

    minLen = getIntLen(farmSafeRange.minSoilMoisture);
    maxLen = getIntLen(farmSafeRange.maxSoilMoisture);

    sprintf(buffer, "%d<土壤<%d%%   ", farmSafeRange.minSoilMoisture, farmSafeRange.maxSoilMoisture);
    OLED_ShowString(0, 15, buffer, OLED_12X12);

    drawUnderline(0, 28, minLen * 6, (rangeEditIndex == RANGE_EDIT_SOIL_MOISTURE_MIN));
    drawUnderline(minLen * 6 + 36, 28, maxLen * 6, (rangeEditIndex == RANGE_EDIT_SOIL_MOISTURE_MAX));

    maxLen = getIntLen(farmSafeRange.maxRainGauge);

    sprintf(buffer, "降雨<%d%%      ", farmSafeRange.maxRainGauge);
    OLED_ShowString(0, 30, buffer, OLED_12X12);

    drawUnderline(30, 43, maxLen * 6, (rangeEditIndex == RANGE_EDIT_RAIN_GAUGE_MAX));
}

void OLED_Task(void *pvParameters)
{
    char buffer[32];
    SensorData_t sensor_data;
    DisplayPage_t received_page;
    static uint8_t need_clear = 0;
    static TickType_t last_blink_tick = 0;
    
    while(1)
    {
        if (xQueueReceive(PageEventQueue, &received_page, 0) == pdTRUE)
        {
            currentPage = received_page;
            need_clear = 1;
            oled_dirty = 1;
        }

        if (currentPage == PAGE_RANGE && rangeEditState == RANGE_EDIT_STATE_EDITING)
        {
            if (xTaskGetTickCount() - last_blink_tick >= pdMS_TO_TICKS(250))
            {
                oled_dirty = 1;
                last_blink_tick = xTaskGetTickCount();
            }
        }

        if (oled_dirty)
        {
            if (xSemaphoreTake(OLED_Mutex, pdMS_TO_TICKS(5)) == pdTRUE)
            {
                if (need_clear || currentPage == PAGE_RANGE)
                {
                    OLED_Clear();
                    need_clear = 0;
                }

                if (currentPage == PAGE_RANGE)
                {
                    if (rangeEditIndex <= RANGE_EDIT_LIGHT_INTENSITY_MAX)
                    {
                        renderRangePage1();
                    }
                    else
                    {
                        renderRangePage2();
                    }
                }
                else if (currentPage == PAGE_HOME)
                {
                    if (xQueuePeek(SensorDataQueue, &sensor_data, 0) == pdTRUE)
                    {
                        OLED_ShowString(30, 0, "Smart Farm", OLED_12X12);
                        
                        OLED_ShowString(9, 14, "温度", OLED_12X12);
                        sprintf(buffer, "%d.%dC   ", (int)sensor_data.temp0, (int)((sensor_data.temp0 - (int)sensor_data.temp0) * 10));
                        OLED_ShowString(6, 26, buffer, OLED_12X12);
                        
                        OLED_ShowString(52, 14, "湿度", OLED_12X12);
                        sprintf(buffer, "%d.%d%%  ", (int)sensor_data.humi, (int)((sensor_data.humi - (int)sensor_data.humi) * 10));
                        OLED_ShowString(52-3, 26, buffer, OLED_12X12);
                        
                        OLED_ShowString(95, 14, "光照", OLED_12X12);
                        sprintf(buffer, "%dls    ", sensor_data.light);
                        OLED_ShowString(95-3, 26, buffer, OLED_12X12);
                        
                        OLED_ShowString(9, 41, "土壤", OLED_12X12);
                        sprintf(buffer, "%d%%   ", sensor_data.soil);
                        OLED_ShowString(9+6, 53, buffer, OLED_12X12);
                        
                        OLED_ShowString(52, 41, "降雨", OLED_12X12);
                        sprintf(buffer, "%d%%   ", sensor_data.rain);
                        OLED_ShowString(58, 53, buffer, OLED_12X12);
                        
                        OLED_ShowString(95, 41, "水泵", OLED_12X12);
                        OLED_ShowString(101, 53, "关", OLED_12X12);
                    }
                }
                
                OLED_Update();
                xSemaphoreGive(OLED_Mutex);
            }
            oled_dirty = 0;
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
