#ifndef I2C_H
#define I2C_H

#include <stdint.h>

// I2C base address and length
#define I2C_BASE 0xFE804000
#define I2C_LEN  (4 * 1024)

// Register offsets
#define C_OFFSET     0x00
#define S_OFFSET     0x04
#define DLEN_OFFSET  0x08
#define A_OFFSET     0x0C
#define FIFO_OFFSET  0x10
#define DIV_OFFSET   0x14
#define DEL_OFFSET   0x18
#define CLKT_OFFSET  0x1C

// Status register bits
#define I2C_S_DONE (1 << 1)
#define I2C_S_TXW  (1 << 4)
#define I2C_S_ERR  (1 << 8)

// Control register bits
#define I2C_C_I2CEN (1 << 15)
#define I2C_C_ST    (1 << 7)

// I2C device address
#define I2C_ADDR 0x27

// LCD Constants
#define LCD_WIDTH 16
#define LCD_CHR 1
#define LCD_CMD 0
#define LCD_LINE_1 0x80
#define LCD_LINE_2 0xC0
#define LCD_BACKLIGHT 0x08
#define ENABLE 0b00000100

// Timing constants
#define E_PULSE 2000
#define E_DELAY 2000

// Function declarations
void print_binary(uint32_t value);
//void i2c_send_byte(volatile uint32_t *i2c, uint8_t byte);
void lcd_toggle_enable(volatile uint32_t *i2c, uint8_t bits);
void lcd_byte(volatile uint32_t *i2c, uint8_t bits, uint8_t mode);
void lcd_init(volatile uint32_t *i2c);
void lcd_string(volatile uint32_t *i2c, const char *message, uint8_t line);

#endif // I2C_H
