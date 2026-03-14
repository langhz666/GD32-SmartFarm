#include "app_shared.h"
#include "driver_led/driver_led.h"

void LED_Task(void *pvParameters)
{
    while(1)
    {
        led_toggle(1);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
