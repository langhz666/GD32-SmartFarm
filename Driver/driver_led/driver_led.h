#ifndef _DRIVER_LED_H_
#define _DRIVER_LED_H_ 

#include "gd32f10x.h"
#include "delay.h"

void ledboard_init(void);

void ledstate_set(uint8_t state);

void led_toggle(void);

void led_blink(uint16_t blink);

#endif

   
