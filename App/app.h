/*
 * @Author: langhz666 3204498297@qq.com
 * @Date: 2026-03-13 18:04:36
 * @LastEditors: langhz666 3204498297@qq.com
 * @LastEditTime: 2026-03-15 19:49:00
 * @FilePath: \GD32F103C8T6\App\app.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _APP_H
#define _APP_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "gd32f10x.h"

#define TASK_PRIORITY_LED         (tskIDLE_PRIORITY + 2)
#define TASK_PRIORITY_OLED        (tskIDLE_PRIORITY + 2)
#define TASK_PRIORITY_KEY         (tskIDLE_PRIORITY + 3)
#define TASK_PRIORITY_SENSOR      (tskIDLE_PRIORITY + 1)
#define TASK_PRIORITY_ENCODER     (tskIDLE_PRIORITY + 2)

#define TASK_STACK_SIZE_LED       64
#define TASK_STACK_SIZE_OLED      512
#define TASK_STACK_SIZE_KEY       64
#define TASK_STACK_SIZE_SENSOR    128
#define TASK_STACK_SIZE_ENCODER   256

typedef struct {
    float temp0;
    float temp1;
    float humi;
    float press;
    uint16_t light;
    uint16_t soil;
    uint16_t rain;
} SensorData_t;

typedef enum {
    PAGE_HOME = 0,
    PAGE_RANGE = 1
} DisplayPage_t;

typedef struct {
    float minTemperature;
    float maxTemperature;
    float minHumidity;
    float maxHumidity;
    uint16_t minLightIntensity;
    uint16_t maxLightIntensity;
    uint16_t minSoilMoisture;
    uint16_t maxSoilMoisture;
    uint16_t maxRainGauge;
} FarmSafeRange_t;

typedef enum {
    RANGE_EDIT_TEMPERATURE_MIN = 0,
    RANGE_EDIT_TEMPERATURE_MAX,
    RANGE_EDIT_HUMIDITY_MIN,
    RANGE_EDIT_HUMIDITY_MAX,
    RANGE_EDIT_LIGHT_INTENSITY_MIN,
    RANGE_EDIT_LIGHT_INTENSITY_MAX,
    RANGE_EDIT_SOIL_MOISTURE_MIN,
    RANGE_EDIT_SOIL_MOISTURE_MAX,
    RANGE_EDIT_RAIN_GAUGE_MAX,
    RANGE_EDIT_COUNT
} RangeEditIndex_t;

typedef enum {
    RANGE_EDIT_STATE_BROWSING = 0,
    RANGE_EDIT_STATE_EDITING
} RangeEditState_t;

extern uint8_t pumpState;

#define FLASH_RANGE_CONFIG_ADDR  0x000000

void App_SaveRangeConfig(void);
void App_LoadRangeConfig(void);

void App_CreateTasks(void);
void App_CreateQueues(void);
void App_CreateSemaphores(void);

#endif
