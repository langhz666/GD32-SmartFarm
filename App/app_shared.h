/*
 * @Author: langhz666 3204498297@qq.com
 * @Date: 2026-03-15 00:07:03
 * @LastEditors: langhz666 3204498297@qq.com
 * @LastEditTime: 2026-03-15 20:40:03
 * @FilePath: \GD32F103C8T6\App\app_shared.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __APP_SHARED_H
#define __APP_SHARED_H

#include "app.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "gd32f10x.h"

extern QueueHandle_t SensorDataQueue;
extern QueueHandle_t PageEventQueue;
extern SemaphoreHandle_t OLED_Mutex;

extern int16_t Num;
extern uint16_t Light;
extern uint8_t oled_dirty;
extern FarmSafeRange_t farmSafeRange;
extern RangeEditIndex_t rangeEditIndex;
extern RangeEditState_t rangeEditState;
extern uint8_t isEditingPage;
extern DisplayPage_t currentPage;

void floatToIntDec(float value, int *intPart, int *decPart);
uint8_t getIntLen(int val);

void LED_Task(void *pvParameters);
void OLED_Task(void *pvParameters);
void Key_Task(void *pvParameters);
void Sensor_Task(void *pvParameters);
void Encoder_Task(void *pvParameters);

#endif
