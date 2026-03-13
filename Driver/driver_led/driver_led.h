#ifndef _DRIVER_LED_H_
#define _DRIVER_LED_H_ 

#include "gd32f10x.h"
#include "delay.h"


#define LED0_GPIO_PORT GPIOC
#define LED0_GPIO_CLK RCU_GPIOC
#define LED0_GPIO_PIN GPIO_PIN_13

#define LED1_GPIO_PORT GPIOB
#define LED1_GPIO_CLK RCU_GPIOB
#define LED1_GPIO_PIN GPIO_PIN_8

#define LED2_GPIO_PORT GPIOB
#define LED2_GPIO_CLK RCU_GPIOB
#define LED2_GPIO_PIN GPIO_PIN_9


void led_init(void);

void ledstate_set(uint8_t led, uint8_t state);

void led_toggle(uint8_t led);

void led_blink(uint8_t led, uint16_t blink);

#endif

   
