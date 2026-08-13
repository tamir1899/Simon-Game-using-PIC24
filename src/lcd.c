#include <xc.h>
#include "lcd.h"
#include "delay.h"

#define LCD_ADDR   0x78

#define LCD_CONTROL_CMD   0x00
#define LCD_CONTROL_DATA  0x40

static void i2c_start(void)
{
    I2C1CONbits.SEN = 1;
    while (I2C1CONbits.SEN);
}

static void i2c_stop(void)
{
    I2C1CONbits.PEN = 1;
    while (I2C1CONbits.PEN);
}

static void i2c_write(uint8_t b)
{
    I2C1TRN = b;
    while (I2C1STATbits.TRSTAT);
}

static void lcd_send(uint8_t control, uint8_t value)
{
    i2c_start();
    i2c_write(LCD_ADDR);
    i2c_write(control);
    i2c_write(value);
    i2c_stop();
}

void lcd_cmd(char cmd)
{
    lcd_send(LCD_CONTROL_CMD, (uint8_t)cmd);
}

void lcd_printChar(char c)
{
    lcd_send(LCD_CONTROL_DATA, (uint8_t)c);
}

void lcd_printStr(const char *s)
{
    while (*s)
        lcd_printChar(*s++);
}

void lcd_setCursor(char x, char y)
{
    uint8_t addr = 0x00;

    switch (y) {
        case 0: addr = 0x00 + x; break;
        case 1: addr = 0x20 + x; break;
        case 2: addr = 0x40 + x; break;
        case 3: addr = 0x60 + x; break;
        default: addr = 0x00 + x; break;
    }

    lcd_cmd(0x80 | addr);
}

void lcd_clear(void)
{
    lcd_cmd(0x01);
    delay_ms_asm(2);
}

void lcd_init(void)
{
    I2C1CONbits.I2CEN = 0;
    I2C1BRG = 199;
    I2C1CONbits.I2CEN = 1;

    delay_ms_asm(40);

    lcd_cmd(0x39);
    delay_ms_asm(2);

    lcd_cmd(0x15);
    lcd_cmd(0x55);
    lcd_cmd(0x6E);
    delay_ms_asm(200);

    lcd_cmd(0x38);
    lcd_cmd(0x0C);
    lcd_cmd(0x01);
    delay_ms_asm(2);

    lcd_cmd(0x06);
}
