#ifndef __DRIVER_BUZZER_H
#define __DRIVER_BUZZER_H

#include "gd32f10x.h"

#define BUZZER_RCU   RCU_GPIOA
#define BUZZER_TIMER RCU_TIMER2
#define BUZZER_PORT GPIOA
#define BUZZER_PIN  GPIO_PIN_6

void Buzzer_PWM_Init(void);

void Buzzer_On(void);

void Buzzer_Off(void);

#endif


