#include "driver_led/driver_led.h"
#include "driver_led.h"

uint8_t led_blink_mode = 0;

void led_init(void)
{
    rcu_periph_clock_enable(LED0_GPIO_CLK); //打开GPIOC时钟
    rcu_periph_clock_enable(LED1_GPIO_CLK); //打开GPIOB时钟
    rcu_periph_clock_enable(LED2_GPIO_CLK); //打开GPIOB时钟

    // 默认状态下，LED熄灭（GPIO输出高电平）
    gpio_bit_set(LED0_GPIO_PORT, LED0_GPIO_PIN);
    gpio_bit_set(LED1_GPIO_PORT, LED1_GPIO_PIN);
    gpio_bit_set(LED2_GPIO_PORT, LED2_GPIO_PIN);

    gpio_init(LED0_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LED0_GPIO_PIN); //将PC13配置为推挽输出，最高支持50MHz
    gpio_init(LED1_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LED1_GPIO_PIN); //将PB8配置为推挽输出，最高支持50MHz
    gpio_init(LED2_GPIO_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LED2_GPIO_PIN); //将PB9配置为推挽输出，最高支持50MHz
}

void ledstate_set(uint8_t led ,uint8_t state)
{
    if (state)
    {
        switch(led)
        {
            case 0:
                gpio_bit_reset(LED0_GPIO_PORT, LED0_GPIO_PIN); //低电平点亮LED
                break;
            case 1:
                gpio_bit_reset(LED1_GPIO_PORT, LED1_GPIO_PIN); //低电平点亮LED
                break;
            case 2:
                gpio_bit_reset(LED2_GPIO_PORT, LED2_GPIO_PIN); //低电平点亮LED
                break;
        }
    }
    else
    {
        switch(led)
        {
            case 0:
                gpio_bit_set(LED0_GPIO_PORT, LED0_GPIO_PIN); //高电平熄灭LED
                break;
            case 1:
                gpio_bit_set(LED1_GPIO_PORT, LED1_GPIO_PIN); //高电平熄灭LED
                break;
            case 2:
                gpio_bit_set(LED2_GPIO_PORT, LED2_GPIO_PIN); //高电平熄灭LED
                break;
        }
    }
}

void led_toggle(uint8_t led)
{
    switch(led)
    {
        case 0:
            gpio_bit_write(LED0_GPIO_PORT, LED0_GPIO_PIN, (bit_status)(1 - gpio_input_bit_get(LED0_GPIO_PORT, LED0_GPIO_PIN))); //切换LED状态
            break;
        case 1:
            gpio_bit_write(LED1_GPIO_PORT, LED1_GPIO_PIN, (bit_status)(1 - gpio_input_bit_get(LED1_GPIO_PORT, LED1_GPIO_PIN))); //切换LED状态
            break;
        case 2:
            gpio_bit_write(LED2_GPIO_PORT, LED2_GPIO_PIN, (bit_status)(1 - gpio_input_bit_get(LED2_GPIO_PORT, LED2_GPIO_PIN))); //切换LED状态
            break;
    }
}

void led_blink(uint8_t led, uint16_t blink)
{
    ledstate_set(led, 1); //点亮LED
    DelayNms(blink); //延时blinkms
    ledstate_set(led, 0); //熄灭LED
    DelayNms(blink); //延时blinkms
}
