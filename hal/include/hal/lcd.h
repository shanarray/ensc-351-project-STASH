// hal/include/hal/lcd.h
#ifndef LCD_H
#define LCD_H

#include <stdbool.h>
#include <stdint.h>

void LCD_init(void);
void LCD_cleanup(void);
void LCD_clear(void);
void LCD_setCursor(uint8_t row, uint8_t col);
void LCD_printString(const char *str);

#endif