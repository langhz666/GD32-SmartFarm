#include "app_shared.h"
#include "driver_key/driver_key.h"
#include "driver_buzzer/driver_buzzer.h"
#include "delay.h"

void Key_Task(void *pvParameters)
{
    uint8_t key;

    while(1)
    {
        key = Key_Scan(0);
        if (key)
        {
            Buzzer_On();
            DelayNms(50);
            Buzzer_Off();
            
            oled_dirty = 1;
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
                        App_SaveRangeConfig();
                    }
                    oled_dirty = 1;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
