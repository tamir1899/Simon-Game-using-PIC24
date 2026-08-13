#ifndef LCD_H
#define LCD_H

#include <xc.h>

void lcd_init(void);
void lcd_cmd(char command);
void lcd_setCursor(char x, char y);
void lcd_printChar(char c);
void lcd_printStr(const char *str);
void lcd_clear(void);

#endif
