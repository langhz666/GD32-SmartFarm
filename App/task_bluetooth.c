#include "app_shared.h"
#include "driver_bluetooth/driver_bluetooth.h"
#include "driver_led/driver_led.h"
#include "driver_key/driver_key.h"
#include <stdio.h>

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
