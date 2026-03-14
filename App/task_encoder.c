#include "app_shared.h"
#include "driver_encoder/driver_encoder.h"

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
            oled_dirty = 1;
            
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
                oled_dirty = 1;
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
