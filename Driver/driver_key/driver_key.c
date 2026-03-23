#include "driver_key/driver_key.h"
#include "delay.h"
#include <stdio.h>

void Key_Init(void)
{
    rcu_periph_clock_enable(KEY1_RCU);
    rcu_periph_clock_enable(KEY2_RCU);
    
    gpio_init(KEY1_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, KEY1_PIN);
    gpio_init(KEY2_PORT, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, KEY2_PIN);

    DelayNms(10);
}

uint8_t Key_Scan(uint8_t mode)
{
    static uint8_t key_up = 1; 

    uint8_t key_val = KEY_NONE;
    uint8_t key1_state = gpio_input_bit_get(KEY1_PORT, KEY1_PIN);
    uint8_t key2_state = gpio_input_bit_get(KEY2_PORT, KEY2_PIN);

    if (mode == 1) 
    {
        key_up = 1;
    }

    if (key_up && (key1_state == RESET || key2_state == RESET))
    {
        DelayNms(10);
        
        key_up = 0;

        if (gpio_input_bit_get(KEY1_PORT, KEY1_PIN) == RESET) key_val = KEY1_PRES;
        if (gpio_input_bit_get(KEY2_PORT, KEY2_PIN) == RESET) key_val = KEY2_PRES;
    }
    else if (key1_state == SET && key2_state == SET)
    {
        key_up = 1;
    }

    return key_val;
}
