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

void LED_Task(void *pvParameters)
{
    while(1)
    {
        led_toggle(1);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void OLED_Task(void *pvParameters)
{
    char buffer[32];
    SensorData_t sensor_data;
    DisplayPage_t received_page;
    static DisplayPage_t current_page = PAGE_HOME;

    OLED_Clear();
    OLED_ShowString(30, 0, "Smart Farm", OLED_8X16);
    OLED_Update();

    while(1)
    {
        if (xQueueReceive(PageEventQueue, &received_page, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            current_page = received_page;
            OLED_Clear();
        }

        if (xSemaphoreTake(OLED_Mutex, portMAX_DELAY) == pdTRUE)
        {
            if (current_page == PAGE_HOME)
            {
                if (xQueuePeek(SensorDataQueue, &sensor_data, 0) == pdTRUE)
                {
                    OLED_ShowString(30, 0, "Smart Farm", OLED_8X16);
                    
                    OLED_ShowString(9, 14, "温度", OLED_8X16);
                    sprintf(buffer, "%d.%d", (int)sensor_data.temp0, (int)((sensor_data.temp0 - (int)sensor_data.temp0) * 10));
                    uint8_t len = strlen(buffer);
                    uint8_t x = 21 - len * 4;
                    OLED_ShowString(x, 26, buffer, OLED_8X16);
                    OLED_ShowString(x + len * 4, 26, "C", OLED_8X16);
                    
                    OLED_ShowString(52, 14, "湿度", OLED_8X16);
                    sprintf(buffer, "%d.%d%%", (int)sensor_data.humi, (int)((sensor_data.humi - (int)sensor_data.humi) * 10));
                    len = strlen(buffer);
                    x = 64 - len * 4;
                    OLED_ShowString(x, 26, buffer, OLED_8X16);
                    
                    OLED_ShowString(95, 14, "光照", OLED_8X16);
                    sprintf(buffer, "%d", sensor_data.light);
                    len = strlen(buffer);
                    x = 107 - len * 4;
                    OLED_ShowString(x, 26, buffer, OLED_8X16);
                    
                    OLED_ShowString(9, 41, "土壤", OLED_8X16);
                    sprintf(buffer, "%d%%", sensor_data.soil);
                    len = strlen(buffer);
                    x = 21 - len * 4;
                    OLED_ShowString(x, 52, buffer, OLED_8X16);
                    
                    OLED_ShowString(52, 41, "降雨", OLED_8X16);
                    sprintf(buffer, "%d%%", sensor_data.rain);
                    len = strlen(buffer);
                    x = 64 - len * 4;
                    OLED_ShowString(x, 52, buffer, OLED_8X16);
                    
                    OLED_ShowString(95, 41, "水泵", OLED_8X16);
                    OLED_ShowString(101, 52, "关", OLED_8X16);
                }
            }
            else if (current_page == PAGE_RANGE_1)
            {
                OLED_ShowString(25, 0, "报警阈值(1/2)", OLED_8X16);
                
                OLED_ShowString(9, 14, "温度", OLED_8X16);
                sprintf(buffer, "20.0 < 温度 < 30.0");
                uint8_t len = strlen(buffer);
                uint8_t x = 64 - len * 4;
                OLED_ShowString(x, 26, buffer, OLED_8X16);
                
                OLED_ShowString(52, 14, "湿度", OLED_8X16);
                sprintf(buffer, "40.0 < 湿度 < 60.0");
                len = strlen(buffer);
                x = 64 - len * 4;
                OLED_ShowString(x, 26, buffer, OLED_8X16);
                
                OLED_ShowString(95, 14, "光照", OLED_8X16);
                sprintf(buffer, "0 < 光照 < 1000");
                len = strlen(buffer);
                x = 107 - len * 4;
                OLED_ShowString(x, 26, buffer, OLED_8X16);
            }
            else if (current_page == PAGE_RANGE_2)
            {
                OLED_ShowString(25, 0, "报警阈值(2/2)", OLED_8X16);
                
                OLED_ShowString(9, 14, "土壤", OLED_8X16);
                sprintf(buffer, "30 < 土壤 < 70");
                uint8_t len = strlen(buffer);
                uint8_t x = 64 - len * 4;
                OLED_ShowString(x, 26, buffer, OLED_8X16);
                
                OLED_ShowString(52, 14, "降雨", OLED_8X16);
                sprintf(buffer, "0 < 降雨 < 50");
                len = strlen(buffer);
                x = 64 - len * 4;
                OLED_ShowString(x, 26, buffer, OLED_8X16);
            }
            
            OLED_Update();
            xSemaphoreGive(OLED_Mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void Key_Task(void *pvParameters)
{
    uint8_t key;
    static DisplayPage_t current_page = PAGE_HOME;

    while(1)
    {
        key = Key_Scan(0);
        if (key)
        {
            if (key == KEY1_PRES)
            {
                current_page++;
                if (current_page > PAGE_RANGE_2)
                {
                    current_page = PAGE_HOME;
                }
                xQueueSend(PageEventQueue, &current_page, 0);
            }
            else if (key == KEY2_PRES)
            {
                if (current_page > PAGE_HOME)
                {
                    current_page--;
                }
                else
                {
                    current_page = PAGE_RANGE_2;
                }
                xQueueSend(PageEventQueue, &current_page, 0);
            }
            else
            {
                xQueueSend(KeyEventQueue, &key, 0);
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
                    printf("Temp0: %.1fC, Temp1: %.1fC, Humi: %.1f%%, Press: %.0fPa, Light: %d\r\n",
                           sensor_data.temp0, sensor_data.temp1, sensor_data.humi,
                           sensor_data.press, sensor_data.light);
                    xSemaphoreGive(UART_Mutex);
                }
            }
            last_print_time = xTaskGetTickCount();
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void Encoder_Task(void *pvParameters)
{
    int8_t encoder_value;

    while(1)
    {
        encoder_value = Encoder_Get();
        if (encoder_value != 0)
        {
            Num += encoder_value;
            if (Num > 9999) Num = 9999;
            if (Num < -999) Num = -999;
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
