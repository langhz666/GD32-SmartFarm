/*
 * @Author: langhz666 3204498297@qq.com
 * @Date: 2026-03-12 14:31:32
 * @LastEditors: langhz666 3204498297@qq.com
 * @LastEditTime: 2026-03-23 19:20:38
 * @FilePath: \GD32F103C8T6\Driver\driver_key\driver_key.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __DRIVER_KEY_H
#define __DRIVER_KEY_H

#include "gd32f10x.h"

#define KEY1_RCU     RCU_GPIOC
#define KEY1_PORT    GPIOC
#define KEY1_PIN     GPIO_PIN_14

#define KEY2_RCU     RCU_GPIOB
#define KEY2_PORT    GPIOB
#define KEY2_PIN     GPIO_PIN_5
// #define KEY3_PIN     GPIO_PIN_15

#define KEY_NONE     0
#define KEY1_PRES    1
#define KEY2_PRES    2
// #define KEY3_PRES    3

void Key_Init(void);
uint8_t Key_Scan(uint8_t mode);

#endif
