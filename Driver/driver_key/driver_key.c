/*
 * @Author: langhz666 3204498297@qq.com
 * @Date: 2026-03-12 14:31:21
 * @LastEditors: langhz666 3204498297@qq.com
 * @LastEditTime: 2026-03-14 20:49:00
 * @FilePath: \GD32F103C8T6\Driver\driver_key\driver_key.c
 * @Description: ����Ĭ������,������`customMade`, ��koroFileHeader�鿴���� ��������: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
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
    uint8_t key1_state = gpio_input_bit_get(KEY_PORT, KEY1_PIN);
    uint8_t key2_state = gpio_input_bit_get(KEY_PORT, KEY2_PIN);

    if (mode == 1) 
    {
        key_up = 1;
    }

    if (key_up && (key1_state == RESET || key2_state == RESET))
    {
        DelayNms(10);
        
        key_up = 0;

        if (gpio_input_bit_get(KEY_PORT, KEY1_PIN) == RESET) key_val = KEY1_PRES;
        if (gpio_input_bit_get(KEY_PORT, KEY2_PIN) == RESET) key_val = KEY2_PRES;
    }
    else if (key1_state == SET && key2_state == SET)
    {
        key_up = 1;
    }

    return key_val;
}
