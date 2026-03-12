#include "driver_key/driver_key.h"
#include "delay.h"
#include <stdio.h>

void Key_Init(void)
{
    rcu_periph_clock_enable(KEY_RCU);
    
    gpio_init(KEY_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, KEY1_PIN | KEY2_PIN );

    DelayNms(10);
    
    printf("Key_Init (IPU): KEY1=%d KEY2=%d \r\n", 
        gpio_input_bit_get(KEY_PORT, KEY1_PIN),
        gpio_input_bit_get(KEY_PORT, KEY2_PIN));
}

uint8_t Key_Scan(uint8_t mode)
{
    static uint8_t key_up = 1; 

    uint8_t key_val = KEY_NONE;

    if (mode == 1) 
    {
        key_up = 1;
    }

    if (key_up && (gpio_input_bit_get(KEY_PORT, KEY1_PIN) == RESET || 
                   gpio_input_bit_get(KEY_PORT, KEY2_PIN) == RESET))
    {
        DelayNms(10);
        
        key_up = 0;

        if (gpio_input_bit_get(KEY_PORT, KEY1_PIN) == RESET) key_val = KEY1_PRES;
        if (gpio_input_bit_get(KEY_PORT, KEY2_PIN) == RESET) key_val = KEY2_PRES;
    }
    else if (gpio_input_bit_get(KEY_PORT, KEY1_PIN) == SET && 
             gpio_input_bit_get(KEY_PORT, KEY2_PIN) == SET)
    {
        key_up = 1;
    }

    return key_val;
}
