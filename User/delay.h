#ifndef _DELAY_H_
#define _DELAY_H_

#include "stdint.h"

void DelayInit(void);
void DelayNus(uint32_t nUs);
void DelayNms(uint32_t nMs);

#endif
