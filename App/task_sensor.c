#include "app_shared.h"
#include "driver_aht20/driver_aht20.h"
#include "driver_bmp280/driver_bmp280.h"
#include "driver_light/driver_light.h"
#include "driver_soil/driver_soil.h"
#include "driver_rain/driver_rain.h"

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
        oled_dirty = 1;
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
