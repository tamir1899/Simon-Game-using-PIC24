#include <xc.h>
#include "lcd.h"
#include "delay.h"
#include "simon.h"

#pragma config ICS = PGx1
#pragma config FWDTEN = OFF
#pragma config GWRP = OFF
#pragma config GCP = OFF
#pragma config JTAGEN = OFF
#pragma config I2C1SEL = PRI
#pragma config IOL1WAY = OFF
#pragma config OSCIOFNC = ON
#pragma config FCKSM = CSECMD
#pragma config FNOSC = FRC
#pragma config IESO  = OFF
#pragma config POSCMOD = NONE

#define PIN_LCD_RST_TRIS  TRISBbits.TRISB10
#define PIN_LCD_RST_LAT   LATBbits.LATB10

#define SDA1_TRIS TRISBbits.TRISB9
#define SCL1_TRIS TRISBbits.TRISB8

static void pic24_init(void)
{
    _RCDIV = 0;
    AD1PCFG = 0xFFFF;

    PIN_LCD_RST_TRIS = 0;
    PIN_LCD_RST_LAT = 1;

    SDA1_TRIS = 1;
    SCL1_TRIS = 1;

    delay_ms_asm(20);
}

static void lcd_hardware_reset_pulse(void)
{
    PIN_LCD_RST_LAT = 0;
    delay_ms_asm(10);
    PIN_LCD_RST_LAT = 1;
    delay_ms_asm(40);
}

int main(void)
{
    pic24_init();

    lcd_hardware_reset_pulse();
    lcd_init();

    simon_init();

    lcd_setCursor(0,0);
    lcd_printStr("SIMON GAME");
    lcd_setCursor(0,1);
    lcd_printStr("L: 0");
    lcd_setCursor(0,2);
    lcd_printStr("HS: 0");
    lcd_setCursor(0,3);
    lcd_printStr("Press GRN");

    simon_play_game();

    while(1);
    return 0;
}
