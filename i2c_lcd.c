#include "i2clcd.h"

// Pin mappings matching your working configuration
#define PIN_RS        0x01  // Bit 0
#define PIN_RW        0x02  // Bit 1
#define PIN_EN        0x04  // Bit 2
#define LCD_BACKLIGHT 0x08  // Bit 3

static void I2C_Write_Nibble(I2C_LCD_HandleTypeDef *lcd, uint8_t nibble, uint8_t rs_state)
{
    uint8_t byte_high;
    uint8_t byte_low;
    uint16_t dev_addr = (uint16_t)(lcd->address << 1);

    // Prepare the two distinct I2C payloads
    byte_high = nibble | PIN_EN | rs_state | LCD_BACKLIGHT; // Enable HIGH
    byte_low  = nibble | rs_state | LCD_BACKLIGHT;          // Enable LOW

    // 1. Send Enable HIGH transaction
    HAL_I2C_Master_Transmit(lcd->hi2c, dev_addr, &byte_high, 1, 50);
    HAL_Delay(1); // Forces a minimum 1ms setup delay (Arduino uses delayMicroseconds)

    // 2. Send Enable LOW transaction (The falling edge that latches data)
    HAL_I2C_Master_Transmit(lcd->hi2c, dev_addr, &byte_low, 1, 50);
    HAL_Delay(1); // Give the LCD controller micro-execution headroom
}

void lcd_send_cmd(I2C_LCD_HandleTypeDef *lcd, char cmd)
{
    uint8_t upper = (cmd & 0xF0);
    uint8_t lower = ((cmd << 4) & 0xF0);

    I2C_Write_Nibble(lcd, upper, 0); // RS = 0 for command
    I2C_Write_Nibble(lcd, lower, 0);
}

void lcd_send_data(I2C_LCD_HandleTypeDef *lcd, char data)
{
    uint8_t upper = (data & 0xF0);
    uint8_t lower = ((data << 4) & 0xF0);

    I2C_Write_Nibble(lcd, upper, PIN_RS); // RS = PIN_RS for data
    I2C_Write_Nibble(lcd, lower, PIN_RS);
}

void lcd_init(I2C_LCD_HandleTypeDef *lcd)
{
    HAL_Delay(50); // Power-on stabilization delay

    // Attention synchronization sequence
    I2C_Write_Nibble(lcd, 0x30, 0);
    HAL_Delay(5);
    I2C_Write_Nibble(lcd, 0x30, 0);
    HAL_Delay(1);
    I2C_Write_Nibble(lcd, 0x30, 0);
    HAL_Delay(1);
    
    // Switch to 4-bit interface mode
    I2C_Write_Nibble(lcd, 0x20, 0);
    HAL_Delay(1);

    // Operational configuration commands
    lcd_send_cmd(lcd, 0x28); // 4-bit mode, 2 display lines, 5x8 font matrix
    lcd_send_cmd(lcd, 0x0C); // Display ON, Cursor OFF, Blink OFF
    lcd_send_cmd(lcd, 0x06); // Entry Mode: Increment cursor automatically
    lcd_send_cmd(lcd, 0x01); // Clear Display completely
    HAL_Delay(2);            // Clearing displays requires extra processing time
}

void lcd_clear(I2C_LCD_HandleTypeDef *lcd)
{
    lcd_send_cmd(lcd, 0x01);
    HAL_Delay(2);
}

void lcd_gotoxy(I2C_LCD_HandleTypeDef *lcd, int col, int row)
{
    static const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    if (row > 3) row = 0;
    
    uint8_t address = 0x80 + row_offsets[row] + col;
    lcd_send_cmd(lcd, address);
}

void lcd_puts(I2C_LCD_HandleTypeDef *lcd, char *str)
{
    while (*str) lcd_send_data(lcd, *str++);
}

void lcd_putchar(I2C_LCD_HandleTypeDef *lcd, char ch)
{
    lcd_send_data(lcd, ch);
}
