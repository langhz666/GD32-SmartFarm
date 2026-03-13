#include "gd32f10x.h"
#include "gd32f10x_libopt.h"
#include "driver_usart/driver_usart.h"
#include "driver_oled/driver_oled.h"
#include "driver_encoder/driver_encoder.h"
#include "driver_light/driver_light.h"
#include "driver_light/iic_light.h"
#include "driver_soil/driver_soil.h"
#include "driver_rain/driver_rain.h"
#include "driver_aht20/driver_aht20.h"
#include "driver_bmp280/driver_bmp280.h"
#include "driver_bluetooth/driver_bluetooth.h"
#include "driver_timer/driver_timer.h"
#include "driver_key/driver_key.h"
#include "driver_w25q64/driver_w25q64.h"
#include "driver_buzzer/driver_buzzer.h"
#include "driver_led/driver_led.h"
#include "driver_adc/driver_adc.h"
#include "systick.h"
#include "delay.h"
#include <stdio.h>
#include <string.h>




int16_t Num = 0;			//定义待被旋转编码器调节的变量 
uint16_t Light = 0;		    //定义光照强度变量
float temp0 = 0.0f;		    //定义温度变量
float humi = 0.0f;		    //定义湿度变量
float press = 0.0f;		    //定义气压变量
float temp1 = 0.0f;	        //定义温度变量

int main(void)
{
    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);
    led_init();
    DelayInit();
    usart_config();
    blt_config();
    timer2_config();
    OLED_Init();//初始化OLED显示模块
    Encoder_Init();		//旋转编码器初始化
    IIC_Light_Init();	//IIC初始化
    AHT20_Init();		//初始化AHT20模块
    BMP280_Init();		//初始化BMP280模块
    Key_Init();		//按键初始化 
    W25Q64_Init();
    Buzzer_PWM_Init();
    ADC_DMA_MultiChannel_Init();
    OLED_ShowString(1, 1, "Nums:", OLED_8X16);			
    OLED_ShowString(1, 17, "Light:", OLED_8X16);		    
    OLED_Update();//更新OLED显示内容
    W25Q64_Test();
    while(1)
    {
        uint8_t key = Key_Scan(0);

        if (key)
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
        }
        DelayNms(10);
    }
}
