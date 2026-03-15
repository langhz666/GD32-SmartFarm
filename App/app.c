#include "app_shared.h"
#include "driver_led/driver_led.h"
#include "driver_w25q64/driver_w25q64.h"
#include <string.h>

QueueHandle_t SensorDataQueue = NULL;
QueueHandle_t PageEventQueue = NULL;
SemaphoreHandle_t OLED_Mutex = NULL;

int16_t Num = 0;
uint16_t Light = 0;
uint8_t oled_dirty = 1;
uint8_t pumpState = 0;

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

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    taskDISABLE_INTERRUPTS();
    
    gpio_bit_write(LED0_GPIO_PORT, LED0_GPIO_PIN, SET);
    gpio_bit_write(LED1_GPIO_PORT, LED1_GPIO_PIN, SET);
    
    for(;;)
    {
    }
}

void App_CreateQueues(void)
{
    SensorDataQueue = xQueueCreate(1, sizeof(SensorData_t));
    PageEventQueue = xQueueCreate(5, sizeof(DisplayPage_t));

    configASSERT(SensorDataQueue != NULL);
    configASSERT(PageEventQueue != NULL);
}

void App_CreateSemaphores(void)
{
    OLED_Mutex = xSemaphoreCreateMutex();

    configASSERT(OLED_Mutex != NULL);
}

void App_CreateTasks(void)
{
    xTaskCreate(LED_Task, "LED", TASK_STACK_SIZE_LED, NULL, TASK_PRIORITY_LED, NULL);
    xTaskCreate(OLED_Task, "OLED", TASK_STACK_SIZE_OLED, NULL, TASK_PRIORITY_OLED, NULL);
    xTaskCreate(Key_Task, "Key", TASK_STACK_SIZE_KEY, NULL, TASK_PRIORITY_KEY, NULL);
    xTaskCreate(Sensor_Task, "Sensor", TASK_STACK_SIZE_SENSOR, NULL, TASK_PRIORITY_SENSOR, NULL);
    xTaskCreate(Encoder_Task, "Encoder", TASK_STACK_SIZE_ENCODER, NULL, TASK_PRIORITY_ENCODER, NULL);
}

void App_SaveRangeConfig(void)
{
    uint8_t buffer[sizeof(FarmSafeRange_t)];
    
    memcpy(buffer, &farmSafeRange, sizeof(FarmSafeRange_t));
    
    W25Q64_EraseSector(FLASH_RANGE_CONFIG_ADDR);
    
    W25Q64_PageProgram(FLASH_RANGE_CONFIG_ADDR, buffer, sizeof(FarmSafeRange_t));
}

void App_LoadRangeConfig(void)
{
    uint8_t buffer[sizeof(FarmSafeRange_t)];
    FarmSafeRange_t tempRange;
    
    W25Q64_ReadData(FLASH_RANGE_CONFIG_ADDR, buffer, sizeof(FarmSafeRange_t));
    
    memcpy(&tempRange, buffer, sizeof(FarmSafeRange_t));
    
    if (tempRange.minTemperature > 0 && tempRange.minTemperature < 100 &&
        tempRange.maxTemperature > 0 && tempRange.maxTemperature < 100 &&
        tempRange.minTemperature < tempRange.maxTemperature &&
        tempRange.minHumidity > 0 && tempRange.minHumidity < 100 &&
        tempRange.maxHumidity > 0 && tempRange.maxHumidity < 100 &&
        tempRange.minHumidity < tempRange.maxHumidity &&
        tempRange.minLightIntensity < tempRange.maxLightIntensity &&
        tempRange.minSoilMoisture < tempRange.maxSoilMoisture &&
        tempRange.maxRainGauge <= 100)
    {
        memcpy(&farmSafeRange, buffer, sizeof(FarmSafeRange_t));
    }
}
