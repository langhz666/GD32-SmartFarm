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
    ledboard_init();
    DelayInit();
    usart_config();
    blt_config();
    timer2_config();
    OLED_Init();//初始化OLED显示模块
    Encoder_Init();		//旋转编码器初始化
    IIC_Light_Init();	//IIC初始化
    AHT20_Init();		//初始化AHT20模块
    BMP280_Init();		//初始化BMP280模块
    ADC_DMA_MultiChannel_Init();
    OLED_ShowString(1, 1, "Nums:", OLED_8X16);			
    OLED_ShowString(1, 17, "Light:", OLED_8X16);		    
    OLED_Update();//更新OLED显示内容
    printf("hello world");
    // 在 main 函数的 while(1) 之前：

    if (AHT20_Init() == true) {
        printf("AHT20 初始化成功!\r\n");
    } else {
        printf("AHT20 初始化失败!\r\n");
    }

    // 重点检查这里的返回值！
    if (BMP280_Init() == true) {
        printf("BMP280 初始化成功, 读到正确的ID!\r\n");
    } else {
        printf("BMP280 初始化失败, 找不到设备!\r\n");
    }
    while(1)
    {
        Num += Encoder_Get();
        Light = Light_Get();
        AHT20_Read_Temp_Humi(&temp0, &humi);
        BMP280_Read_Temp_Press(&temp1, &press);
        OLED_ShowNum(40, 1, Num, 5, OLED_8X16);
        OLED_ShowNum(50, 17, Light, 5, OLED_8X16);
        OLED_Update();
        printf("soil_humidity = %d%%\r\n",Get_Soil_humidity());
        printf("rain_size = %d\r\n",Get_Rain_size());
        printf("temp0 = %.2f humi = %.2f\r\n", temp0, humi);
        printf("temp1 = %.2f press = %.2f\r\n", temp1, press);
        blt_forward_from_pc();
        blt_forward_to_pc();
    }
}
