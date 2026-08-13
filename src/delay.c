#include "delay.h"

extern void thh_wait_1ms(void);

void delay_ms_asm(unsigned int ms)
{
    while(ms--)
    {
        thh_wait_1ms();
    }
}
