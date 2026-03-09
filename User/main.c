#include "gd32f10x.h"
#include "gd32f10x_libopt.h"
#include "driver_usart/driver_usart.h"
#include "driver_oled/driver_oled.h"
#include "driver_encoder/driver_encoder.h"
#include "driver_light/driver_light.h"
#include "driver_light/iic_light.h"
#include "stdio.h"
#include "systick.h"
#include "delay.h"
 
 
int16_t Num = 0;			//定义待被旋转编码器调节的变量 
uint16_t Light = 0;		    //定义光照强度变量

int main(void)
{
    rcu_periph_clock_enable(RCU_GPIOC); //打开GPIOC时钟
    gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_13); //将PC13配置为推挽输出，最高支持50MHz
    DelayInit();
    usart_config();
    OLED_Init();//初始化OLED
    Encoder_Init();		//旋转编码器初始化
    IIC_Light_Init();	//IIC初始化
    OLED_ShowString(1, 1, "Nums:", OLED_8X16);			//1行1列显示字符串Num:
    OLED_ShowString(1, 17, "Light:", OLED_8X16);		    //1行2列显示字符串Light:
    OLED_Update();//更新OLED显示内容

    while(1)
    {
        Num += Encoder_Get();
        Light = Light_Get();
        OLED_ShowNum(40, 1, Num, 5, OLED_8X16);		    //4行1列显示Num变量值
        OLED_ShowNum(50, 17, Light, 5, OLED_8X16);		    //4行2列显示Light变量值
        OLED_Update();//更新OLED显示内容
//        gpio_bit_reset(GPIOC, GPIO_PIN_13);/* 将 GPIOC 的第 13 号引脚输出高电平（置 0） */
//        DelayNms(500);/* 延时 500 毫秒（0.5秒） */
//        gpio_bit_set(GPIOC, GPIO_PIN_13);/* 将 GPIOC 的第 13 号引脚输出高电平（置 1） */
//        DelayNms(500);/* 延时 500 毫秒（0.5秒） */
    }
}
