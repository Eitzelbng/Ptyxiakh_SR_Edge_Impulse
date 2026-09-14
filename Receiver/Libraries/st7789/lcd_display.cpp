#define GPIO_HIGH 1
#define GPIO_LOW 0

#include "stdint.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "font.h"
#include "lcd_display.h"
#include <hardware/spi.h>

void lcd_write_cmd(lcd_info_t lcd_info_f, uint8_t cmd,uint8_t cmd_size)
{
    gpio_put(lcd_info_f.dc,GPIO_LOW);
    gpio_put(lcd_info_f.cs,GPIO_LOW);
    spi_write_blocking(lcd_info_f.spi,&cmd,cmd_size);
    gpio_put(lcd_info_f.cs,GPIO_HIGH);
    sleep_us(1);
}

void lcd_write_data(lcd_info_t lcd_info_f, uint8_t data,uint8_t data_size)
{
    gpio_put(lcd_info_f.dc,GPIO_HIGH);
    gpio_put(lcd_info_f.cs,GPIO_LOW);
    spi_write_blocking(lcd_info_f.spi,&data,data_size);
    gpio_put(lcd_info_f.cs,GPIO_HIGH);
    sleep_us(1);
}

void lcd_write_data_raw(lcd_info_t lcd_info_f, uint8_t data,uint8_t data_size)
{
    spi_write_blocking(lcd_info_f.spi,&data,data_size);

}

void init_serial(lcd_info_t lcd_info_f)
{
    spi_init(lcd_info_f.spi,lcd_info_f.baud);
    gpio_set_function(lcd_info_f.sck,GPIO_FUNC_SPI);
    gpio_set_function(lcd_info_f.tx,GPIO_FUNC_SPI);
    gpio_init(lcd_info_f.cs);
    gpio_init(lcd_info_f.dc);
    gpio_init(lcd_info_f.rst);
    gpio_set_dir(lcd_info_f.cs,GPIO_OUT);
    gpio_set_dir(lcd_info_f.dc,GPIO_OUT);
    gpio_set_dir(lcd_info_f.rst,GPIO_OUT);
    gpio_put(lcd_info_f.cs,GPIO_HIGH);
    gpio_put(lcd_info_f.dc,GPIO_HIGH);
    gpio_put(lcd_info_f.rst,GPIO_HIGH);
}

void init_st7789(lcd_info_t lcd_info_f)
{
    gpio_put(lcd_info_f.rst, GPIO_LOW);
    sleep_ms(20); 
    gpio_put(lcd_info_f.rst, GPIO_HIGH);
    sleep_ms(150);

    // 1. Software Reset
    lcd_write_cmd(lcd_info_f, 0x01, 1);
    sleep_ms(150);

    // 2. Sleep Out (CRITICAL - won't work without this)
    lcd_write_cmd(lcd_info_f, 0x11, 1); 
    sleep_ms(150); // Wait for power supply to stabilize

    // 3. Interface Pixel Format COLMOD
    lcd_write_cmd(lcd_info_f, 0x3A, 1);
    lcd_write_data(lcd_info_f, 0x05, 1); // 16-bit RGB565

    // 4. Memory Data Access Control
    lcd_write_cmd(lcd_info_f, 0x36, 1);
    lcd_write_data(lcd_info_f, 0x10, 1);

    // 5. Inversion On (Standard for most ST7789 modules)
    lcd_write_cmd(lcd_info_f, 0x21, 1);

    // 6. Display ON (CRITICAL - turns the screen on)
    lcd_write_cmd(lcd_info_f, 0x29, 1);
    sleep_ms(100);
}

void lcd_select_window(lcd_info_t lcd_info_f, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // Column Address Set (X-axis)
    lcd_write_cmd(lcd_info_f, 0x2A, 1);
    lcd_write_data(lcd_info_f, x0 >> 8, 1);    // X-start High
    lcd_write_data(lcd_info_f, x0 & 0xFF, 1);  // X-start Low
    lcd_write_data(lcd_info_f, x1 >> 8, 1);    // X-end High
    lcd_write_data(lcd_info_f, x1 & 0xFF, 1);  // X-end Low

    // Row Address Set (Y-axis)
    lcd_write_cmd(lcd_info_f, 0x2B, 1);
    lcd_write_data(lcd_info_f, y0 >> 8, 1);    // Y-start High
    lcd_write_data(lcd_info_f, y0 & 0xFF, 1);  // Y-start Low
    lcd_write_data(lcd_info_f, y1 >> 8, 1);    // Y-end High
    lcd_write_data(lcd_info_f, y1 & 0xFF, 1);  // Y-end Low

    // Command to start writing to RAM
    lcd_write_cmd(lcd_info_f, 0x2C, 1); 
}

void lcd_fill_window(lcd_info_t lcd_info_f, uint16_t color,uint16_t x0, uint16_t y0,uint16_t x1, uint16_t y1) {
    // 1. Set the window to full screen
    // Note: Use your actual resolution (usually 240x240 or 240x320)
    lcd_select_window(lcd_info_f, x0, y0, (x1)-1, (y1)-1);

    // 2. Prepare the color bytes
    uint8_t color_buf[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };

    // 3. Manually control CS/DC for high speed
    gpio_put(lcd_info_f.dc, GPIO_HIGH);
    gpio_put(lcd_info_f.cs, GPIO_LOW);

    for (int i = 0; i < x1 * y1; i++) {
        spi_write_blocking(lcd_info_f.spi, color_buf, 2);
    }

  
     gpio_put(lcd_info_f.cs, GPIO_HIGH);
}

void lcd_post_fullscreen_picture(lcd_info_t lcd_info_f,const uint16_t* data,size_t data_size,bool byte_swap)
{
    lcd_select_window(lcd_info_f, 0, 0, (lcd_info_f.pixels_x)-1, (lcd_info_f.pixels_y)-1);
    gpio_put(lcd_info_f.dc, GPIO_HIGH);
    gpio_put(lcd_info_f.cs, GPIO_LOW);
    
    if(byte_swap)
    {
        uint16_t* temp = NULL;
        temp = new uint16_t [(lcd_info_f.pixels_x*lcd_info_f.pixels_y)];
        uint16_t byte_buffer = 0;
        for (int i = 0; i < (lcd_info_f.pixels_x*lcd_info_f.pixels_y); i++)
        {
            byte_buffer = (data[i]>>8) | (data[i]<<8);
            temp[i] = byte_buffer; 
        }
        spi_write_blocking(lcd_info_f.spi, (uint8_t *)temp, (data_size));
        delete[] temp;
    }else
    {
        spi_write_blocking(lcd_info_f.spi, (uint8_t *)data, (data_size));
    }
    gpio_put(lcd_info_f.cs, GPIO_HIGH);
}

void lcd_write_word(lcd_info_t lcd_info_f, uint16_t x, uint16_t y, char *data, size_t data_size)
{
    if(data_size>15)
    {
        data_size = 15;
    }
    lcd_fill_window(lcd_info_f,0xffff,x,y,LCD_WIDTH,16);
    uint32_t x_max = x+15 ;
    uint8_t* letter_color_buffer = new uint8_t [512];
    uint8_t letter;
    for(int u=0; u<data_size; u++)
    {
        uint16_t ctr = 0;
        char letter_to_int = (uint8_t)data[u];
        for(int i=0; i<32; i++)
        {
            for(int k=0; k<8; k++)
            {
                if((font[letter_to_int][i]>>k)&0x01)
                {
                    letter_color_buffer[ctr+1]= 0x00;
                    letter_color_buffer[ctr]= 0x00;
                }else
                {
                    letter_color_buffer[ctr+1]= 0xff;
                    letter_color_buffer[ctr]= 0xff;
                }
                ctr+=2;
            }
        }
        lcd_select_window(lcd_info_f, x, y, x_max, y+15);
        gpio_put(lcd_info_f.dc, GPIO_HIGH);
        gpio_put(lcd_info_f.cs, GPIO_LOW);
        spi_write_blocking(lcd_info_f.spi, letter_color_buffer, 512);
        x = x_max+1;
        x_max = x+15;
        gpio_put(lcd_info_f.cs, GPIO_HIGH);
        
    }
    
    delete[] letter_color_buffer;
    gpio_put(lcd_info_f.cs, GPIO_HIGH);

}


void lcd_init(lcd_info_t lcd_info_f) 
{
    init_serial(lcd_info_f);
    init_st7789(lcd_info_f);
    lcd_fill_window(lcd_info_f,0xffff,0,0,LCD_WIDTH,LCD_HEIGHT);
}

void lcd_clear_screen(lcd_info_t lcd_info_f)
{
    lcd_fill_window(lcd_info_f,0xffff,0,0,LCD_WIDTH,LCD_HEIGHT);
}