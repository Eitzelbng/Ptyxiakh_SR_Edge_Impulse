
#ifndef LCD_DISPLAY
#define LCD_DISPLAY
#endif

#define LCD_BAUDRATE 32000000
#define LCD_MISO 12
#define LCD_CS 13
#define LCD_SCK 10
#define LCD_MOSI 11
#define LCD_DC 9
#define LCD_RST 8

#define LCD_HEIGHT 240
#define LCD_WIDTH 240
#define LCD_HEIGHT_ROWS 10
#define LCD_WIDTH_COLLUMS 20

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "hardware/spi.h"

class lcd_info_t
{
public:
    uint32_t baud;
    uint16_t tx;
    uint16_t cs;
    uint16_t sck;
    uint16_t dc;
    uint16_t rst;
    spi_inst_t *spi;
    uint16_t pixels_x;
    uint16_t pixels_y;


};

void lcd_init(lcd_info_t);
void lcd_post_fullscreen_picture(lcd_info_t lcd_info_f, const uint16_t *data, size_t data_size, bool byte_swap);
void lcd_fill_window(lcd_info_t lcd_info_f, uint16_t color);
void lcd_write_word(lcd_info_t lcd_info_f, uint16_t x, uint16_t y, char *data, size_t data_size);
void lcd_clear_screen(lcd_info_t lcd_info_f);