#include "driver_led/driver_led.h"


void ledboard_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOC); //打开GPIOC时钟
    gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_13); //将PC13配置为推挽输出，最高支持50MHz
}

void ledstate_set(uint8_t state)
{
    if (state)
    {
        gpio_bit_reset(GPIOC, GPIO_PIN_13); //低电平点亮LED
    }
    else
    {
        gpio_bit_set(GPIOC, GPIO_PIN_13); //高电平熄灭LED
    }
}

void led_toggle(void)
{
    gpio_bit_write(GPIOC, GPIO_PIN_13, (bit_status)(1 - gpio_input_bit_get(GPIOC, GPIO_PIN_13))); //切换LED状态
}

void led_blink(uint16_t blink)
{
    ledstate_set(1); //点亮LED
    DelayNms(blink); //延时blinkms
    ledstate_set(0); //熄灭LED
    DelayNms(blink); //延时blinkms
}
