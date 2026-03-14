#include "app.h"
#include "driver_usart/driver_usart.h"
#include "driver_oled/driver_oled.h"
#include "driver_encoder/driver_encoder.h"
#include "driver_light/driver_light.h"
#include "driver_light/iic_light.h"
#include "driver_soil/driver_soil.h"
#include "driver_rain/driver_rain.h"
#include "driver_aht20/driver_aht20.h"
#include "driver_bmp280/driver_bmp280.h"
#include "driver_bluetooth/driver_bluetooth.h"
#include "driver_key/driver_key.h"
#include "driver_w25q64/driver_w25q64.h"
#include "driver_buzzer/driver_buzzer.h"
#include "driver_led/driver_led.h"
#include "driver_adc/driver_adc.h"
#include <stdio.h>
#include <string.h>

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    taskDISABLE_INTERRUPTS();
    
    gpio_bit_write(LED0_GPIO_PORT, LED0_GPIO_PIN, SET);
    gpio_bit_write(LED1_GPIO_PORT, LED1_GPIO_PIN, SET);
    
    for(;;)
    {
    }
}

static QueueHandle_t SensorDataQueue = NULL;
static QueueHandle_t KeyEventQueue = NULL;
static QueueHandle_t PageEventQueue = NULL;
static SemaphoreHandle_t OLED_Mutex = NULL;
static SemaphoreHandle_t UART_Mutex = NULL;

int16_t Num = 0;
uint16_t Light = 0;
uint8_t oled_dirty = 1; // 【新增】全局脏标记，1表示需要刷新屏幕，0表示画面无变化无需刷新

FarmSafeRange_t farmSafeRange = {
    .minTemperature = 20.0f,
    .maxTemperature = 30.0f,
    .minHumidity = 40.0f,
    .maxHumidity = 60.0f,
    .minLightIntensity = 0,
    .maxLightIntensity = 1000,
    .minSoilMoisture = 30,
    .maxSoilMoisture = 70,
    .maxRainGauge = 50
};

RangeEditIndex_t rangeEditIndex = RANGE_EDIT_TEMPERATURE_MIN;
RangeEditState_t rangeEditState = RANGE_EDIT_STATE_BROWSING;
uint8_t isEditingPage = 0;
DisplayPage_t currentPage = PAGE_HOME;

void LED_Task(void *pvParameters)
{
    while(1)
    {
        led_toggle(1);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void floatToIntDec(float value, int *intPart, int *decPart)
{
    *intPart = (int)value;
    *decPart = (int)((value - *intPart) * 10);
    if (*decPart < 0) *decPart = -*decPart;
}

static uint8_t getIntLen(int val)
{
    uint8_t len = 0;
    if (val < 0) { len++; val = -val; }
    if (val == 0) return 1;
    while (val > 0) { len++; val /= 10; }
    return len;
}

// 【优化】通过系统心跳直接计算闪烁状态，不再依赖外部传参，彻底解决状态不同步问题
static void drawUnderline(uint8_t x, uint8_t y, uint8_t len, uint8_t isActive)
{
    if (isActive)
    {
        // 每 250ms 翻转一次状态，实现完美的闪烁效果
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
            oled_dirty = 1; // 页面切换，强制重绘
        }

        // 【优化】仅在编辑模式下，为了让下划线闪烁，每250ms主动触发一次局部重绘
        if (currentPage == PAGE_RANGE && rangeEditState == RANGE_EDIT_STATE_EDITING)
        {
            if (xTaskGetTickCount() - last_blink_tick >= pdMS_TO_TICKS(250))
            {
                oled_dirty = 1;
                last_blink_tick = xTaskGetTickCount();
            }
        }

        // 【核心机制】只有在需要刷新时（收到数据、旋转编码器、光标闪烁），才调用底层I2C！
        if (oled_dirty)
        {
            if (xSemaphoreTake(OLED_Mutex, pdMS_TO_TICKS(5)) == pdTRUE)
            {
                // 如果是切换了页面，或者是处于编辑界面（彻底消除重影），就整体清屏
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
            oled_dirty = 0; // 刷新完毕，清除脏标记
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void Key_Task(void *pvParameters)
{
    uint8_t key;

    while(1)
    {
        key = Key_Scan(0);
        if (key)
        {
            oled_dirty = 1; // 发生按键事件，唤醒屏幕重绘
            if (key == KEY1_PRES)
            {
                currentPage++;
                if (currentPage > PAGE_RANGE)
                {
                    currentPage = PAGE_HOME;
                }
                xQueueSend(PageEventQueue, &currentPage, 0);
            }
            else if (key == KEY2_PRES)
            {
                if (currentPage == PAGE_RANGE)
                {
                    if (rangeEditState == RANGE_EDIT_STATE_BROWSING)
                    {
                        rangeEditState = RANGE_EDIT_STATE_EDITING;
                    }
                    else
                    {
                        rangeEditState = RANGE_EDIT_STATE_BROWSING;
                    }
                    oled_dirty = 1;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

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
        oled_dirty = 1; // 传感器数据有更新，唤醒屏幕重绘
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void Bluetooth_Task(void *pvParameters)
{
    uint8_t key;
    SensorData_t sensor_data;
    static TickType_t last_print_time = 0;

    while(1)
    {
        if (xQueueReceive(KeyEventQueue, &key, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            if (xSemaphoreTake(UART_Mutex, portMAX_DELAY) == pdTRUE)
            {
                switch (key)
                {
                case KEY1_PRES:
                    printf("KEY1 pressed\r\n");
                    led_toggle(1);
                    break;

                case KEY2_PRES:
                    printf("KEY2 pressed\r\n");
                    led_toggle(2);
                    break;

                default:
                    break;
                }
                xSemaphoreGive(UART_Mutex);
            }
        }

        if (xTaskGetTickCount() - last_print_time >= pdMS_TO_TICKS(1000))
        {
            if (xQueuePeek(SensorDataQueue, &sensor_data, 0) == pdTRUE)
            {
                if (xSemaphoreTake(UART_Mutex, portMAX_DELAY) == pdTRUE)
                {
                    int t0_int, t0_dec, t1_int, t1_dec, h_int, h_dec;
                    
                    floatToIntDec(sensor_data.temp0, &t0_int, &t0_dec);
                    floatToIntDec(sensor_data.temp1, &t1_int, &t1_dec);
                    floatToIntDec(sensor_data.humi, &h_int, &h_dec);
                    
                    printf("Temp0: %d.%dC, Temp1: %d.%dC, Humi: %d.%d%%, Press: %dPa, Light: %d\r\n",
                           t0_int, t0_dec, t1_int, t1_dec, h_int, h_dec,
                           (int)sensor_data.press, sensor_data.light);
                    xSemaphoreGive(UART_Mutex);
                }
            }
            last_print_time = xTaskGetTickCount();
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void handleEditValueChange(int8_t direction)
{
    float delta = (direction > 0) ? 0.5f : -0.5f;
    int16_t intDelta = (direction > 0) ? 5 : -5;
    
    switch (rangeEditIndex)
    {
    case RANGE_EDIT_TEMPERATURE_MIN:
        farmSafeRange.minTemperature += delta;
        if (farmSafeRange.minTemperature < 0) farmSafeRange.minTemperature = 0;
        if (farmSafeRange.minTemperature >= farmSafeRange.maxTemperature) 
            farmSafeRange.minTemperature = farmSafeRange.maxTemperature - 0.5f;
        break;
    case RANGE_EDIT_TEMPERATURE_MAX:
        farmSafeRange.maxTemperature += delta;
        if (farmSafeRange.maxTemperature > 100) farmSafeRange.maxTemperature = 100;
        if (farmSafeRange.maxTemperature <= farmSafeRange.minTemperature)
            farmSafeRange.maxTemperature = farmSafeRange.minTemperature + 0.5f;
        break;
    case RANGE_EDIT_HUMIDITY_MIN:
        farmSafeRange.minHumidity += delta;
        if (farmSafeRange.minHumidity < 0) farmSafeRange.minHumidity = 0;
        if (farmSafeRange.minHumidity >= farmSafeRange.maxHumidity)
            farmSafeRange.minHumidity = farmSafeRange.maxHumidity - 0.5f;
        break;
    case RANGE_EDIT_HUMIDITY_MAX:
        farmSafeRange.maxHumidity += delta;
        if (farmSafeRange.maxHumidity > 100) farmSafeRange.maxHumidity = 100;
        if (farmSafeRange.maxHumidity <= farmSafeRange.minHumidity)
            farmSafeRange.maxHumidity = farmSafeRange.minHumidity + 0.5f;
        break;
    case RANGE_EDIT_LIGHT_INTENSITY_MIN:
        if (intDelta < 0 && farmSafeRange.minLightIntensity < (uint16_t)(-intDelta))
        {
            farmSafeRange.minLightIntensity = 0;
        }
        else
        {
            farmSafeRange.minLightIntensity += intDelta;
        }
        if (farmSafeRange.minLightIntensity >= farmSafeRange.maxLightIntensity)
            farmSafeRange.minLightIntensity = farmSafeRange.maxLightIntensity - 5;
        break;
    case RANGE_EDIT_LIGHT_INTENSITY_MAX:
        farmSafeRange.maxLightIntensity += intDelta;
        if (farmSafeRange.maxLightIntensity > 65535) farmSafeRange.maxLightIntensity = 65535;
        if (farmSafeRange.maxLightIntensity <= farmSafeRange.minLightIntensity)
            farmSafeRange.maxLightIntensity = farmSafeRange.minLightIntensity + 5;
        break;
    case RANGE_EDIT_SOIL_MOISTURE_MIN:
        if (direction < 0 && farmSafeRange.minSoilMoisture == 0)
        {
        }
        else
        {
            farmSafeRange.minSoilMoisture += (direction > 0) ? 1 : -1;
        }
        if (farmSafeRange.minSoilMoisture >= farmSafeRange.maxSoilMoisture)
            farmSafeRange.minSoilMoisture = farmSafeRange.maxSoilMoisture - 1;
        break;
    case RANGE_EDIT_SOIL_MOISTURE_MAX:
        farmSafeRange.maxSoilMoisture += (direction > 0) ? 1 : -1;
        if (farmSafeRange.maxSoilMoisture > 100) farmSafeRange.maxSoilMoisture = 100;
        if (farmSafeRange.maxSoilMoisture <= farmSafeRange.minSoilMoisture)
            farmSafeRange.maxSoilMoisture = farmSafeRange.minSoilMoisture + 1;
        break;
    case RANGE_EDIT_RAIN_GAUGE_MAX:
        if (direction < 0 && farmSafeRange.maxRainGauge == 0)
        {
        }
        else
        {
            farmSafeRange.maxRainGauge += (direction > 0) ? 1 : -1;
        }
        if (farmSafeRange.maxRainGauge > 100) farmSafeRange.maxRainGauge = 100;
        break;
    default:
        break;
    }
}

void Encoder_Task(void *pvParameters)
{
    int8_t encoder_value;
    static DisplayPage_t last_page = PAGE_HOME;

    while(1)
    {
        encoder_value = Encoder_Get();
        
        if (currentPage != last_page)
        {
            last_page = currentPage;
            oled_dirty = 1; // 监测到跨页情况重绘屏幕
            
            if (currentPage == PAGE_RANGE)
            {
                isEditingPage = 1;
                rangeEditIndex = RANGE_EDIT_TEMPERATURE_MIN;
                rangeEditState = RANGE_EDIT_STATE_BROWSING;
            }
            else
            {
                isEditingPage = 0;
            }
        }
        
        if (isEditingPage)
        {
            if (encoder_value != 0)
            {
                oled_dirty = 1; // 数值发生改变时立刻触发屏幕重绘
                if (rangeEditState == RANGE_EDIT_STATE_BROWSING)
                {
                    if (encoder_value > 0)
                    {
                        if (rangeEditIndex < RANGE_EDIT_COUNT - 1)
                        {
                            rangeEditIndex = (RangeEditIndex_t)(rangeEditIndex + 1);
                        }
                        else
                        {
                            rangeEditIndex = RANGE_EDIT_TEMPERATURE_MIN;
                        }
                    }
                    else
                    {
                        if (rangeEditIndex > 0)
                        {
                            rangeEditIndex = (RangeEditIndex_t)(rangeEditIndex - 1);
                        }
                        else
                        {
                            rangeEditIndex = (RangeEditIndex_t)(RANGE_EDIT_COUNT - 1);
                        }
                    }
                }
                else
                {
                    handleEditValueChange(encoder_value);
                }
            }
        }
        else
        {
            if (encoder_value != 0)
            {
                oled_dirty = 1; 
                Num += encoder_value;
                if (Num > 9999) Num = 9999;
                if (Num < -999) Num = -999;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void App_CreateQueues(void)
{
    SensorDataQueue = xQueueCreate(1, sizeof(SensorData_t));
    KeyEventQueue = xQueueCreate(10, sizeof(uint8_t));
    PageEventQueue = xQueueCreate(5, sizeof(DisplayPage_t));

    configASSERT(SensorDataQueue != NULL);
    configASSERT(KeyEventQueue != NULL);
    configASSERT(PageEventQueue != NULL);
}

void App_CreateSemaphores(void)
{
    OLED_Mutex = xSemaphoreCreateMutex();
    UART_Mutex = xSemaphoreCreateMutex();

    configASSERT(OLED_Mutex != NULL);
    configASSERT(UART_Mutex != NULL);
}

void App_CreateTasks(void)
{
    xTaskCreate(LED_Task, "LED", TASK_STACK_SIZE_LED, NULL,
                TASK_PRIORITY_LED, NULL);

    xTaskCreate(OLED_Task, "OLED", TASK_STACK_SIZE_OLED, NULL,
                TASK_PRIORITY_OLED, NULL);

    xTaskCreate(Key_Task, "Key", TASK_STACK_SIZE_KEY, NULL,
                TASK_PRIORITY_KEY, NULL);

    xTaskCreate(Sensor_Task, "Sensor", TASK_STACK_SIZE_SENSOR, NULL,
                TASK_PRIORITY_SENSOR, NULL);

    xTaskCreate(Bluetooth_Task, "Bluetooth", TASK_STACK_SIZE_BLUETOOTH, NULL,
                TASK_PRIORITY_BLUETOOTH, NULL);

    xTaskCreate(Encoder_Task, "Encoder", TASK_STACK_SIZE_ENCODER, NULL,
                TASK_PRIORITY_ENCODER, NULL);
}
