#ifndef DRIVER_USART_H
#define DRIVER_USART_H

#include "gd32f10x.h"

// Function prototypes
void usart_config(void);

void uart_send_string(uint32_t usart_periph, char *str);

#endif // DRIVER_USART_H


