
#ifndef LCD_H
#define LCD_H

#include <stdint.h>

/*
 * Initialize the I2C LCD.
 * Uses /dev/i2c-1 and I2C address 0x27.
 */
void LCD_init(void);

/* Clear the display and release the I2C device. */
void LCD_cleanup(void);

/* Clear the display without closing the I2C device. */
void LCD_clear(void);

/*
 * Set the cursor position using zero-based row and column.
 * Common 16x2 and 20x4 LCD row offsets are supported.
 */
void LCD_setCursor(uint8_t row, uint8_t col);

/* Print a null-terminated string at the current cursor position. */
void LCD_printString(const char *str);

#endif
