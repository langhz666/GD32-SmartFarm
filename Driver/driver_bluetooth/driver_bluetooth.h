#ifndef _DRIVER_BLUETOOTH_H_
#define _DRIVER_BLUETOOTH_H_ 

#include "gd32f10x.h"

#define BLT_RX_BUF_SIZE 128

#define BLT_DEBUG_ENABLE 1

void blt_config(void);

void blt_send_byte(uint8_t byte);

void blt_send_string(char *str);

void blt_forward_from_pc(void);

void blt_forward_to_pc(void);

void blt_led_control(char *cmd);


#endif
