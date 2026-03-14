#include "gd32f10x.h"
#include "gd32f10x_libopt.h"
#include "app.h"
#include "driver_led/driver_led.h"
#include "delay.h"
#include "driver_usart/driver_usart.h"
#include "driver_bluetooth/driver_bluetooth.h"
#include "driver_timer/driver_timer.h"
#include "driver_oled/driver_oled.h"
#include "driver_encoder/driver_encoder.h"
#include "driver_light/iic_light.h"
#include "driver_aht20/driver_aht20.h"
#include "driver_bmp280/driver_bmp280.h"
#include "driver_key/driver_key.h"
#include "driver_w25q64/driver_w25q64.h"
#include "driver_buzzer/driver_buzzer.h"
#include "driver_adc/driver_adc.h"

int main(void)
{
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    
    led_init();
    DelayInit();
    usart_config();
    blt_config();
    timer2_config();
    OLED_Init();
    Encoder_Init();
    IIC_Light_Init();
    AHT20_Init();
    BMP280_Init();
    Key_Init();
    W25Q64_Init();
    Buzzer_PWM_Init();
    ADC_DMA_MultiChannel_Init();

    App_CreateQueues();
    App_CreateSemaphores();
    App_CreateTasks();

    vTaskStartScheduler();

    printf("ERROR: Scheduler failed to start!\r\n");
    while(1);
}
