#ifndef __APP_SHARED_H
#define __APP_SHARED_H

#include "app.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "gd32f10x.h"

extern QueueHandle_t SensorDataQueue;
extern QueueHandle_t KeyEventQueue;
extern QueueHandle_t PageEventQueue;
extern SemaphoreHandle_t OLED_Mutex;
extern SemaphoreHandle_t UART_Mutex;

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
void Bluetooth_Task(void *pvParameters);
void Encoder_Task(void *pvParameters);

#endif
