#include "app_shared.h"

void floatToIntDec(float value, int *intPart, int *decPart)
{
    *intPart = (int)value;
    *decPart = (int)((value - *intPart) * 10);
    if (*decPart < 0) *decPart = -*decPart;
}

uint8_t getIntLen(int val)
{
    uint8_t len = 0;
    if (val < 0) { len++; val = -val; }
    if (val == 0) return 1;
    while (val > 0) { len++; val /= 10; }
    return len;
}
