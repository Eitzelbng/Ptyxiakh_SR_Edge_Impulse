#include <stdio.h>
#include <string.h>
#include "stdint.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/resets.h"
#include "hardware/clocks.h"
#include "Libraries/st7789/lcd_display.h"
#include "Libraries/sx1276/LoRa-RP2040.h"
#include "ehe.h"


uint8_t rx_buffer[256] = {0};

lcd_info_t lcd_info = 
{
    .baud = LCD_BAUDRATE,
    .tx = LCD_MOSI,
    .cs = LCD_CS,
    .sck = LCD_SCK,
    .dc = LCD_DC,
    .rst = LCD_RST,
    .spi = spi1,
    .pixels_x = 240,
    .pixels_y = 240,
};

class lcd_map
{
  public:
   char row1[16];
   char row2[16];
   char* index_start;
   char* index_split;
   char* index_end ;
};

lcd_map lcd_m;

char Sound[] = "Sound:";
char Accuracy[] = "Accuracy %:";

int main() 
{
    char lora_buffer[64];
    stdio_init_all();
    sleep_ms(3000);
    LoRa.setPins(17, 20, 21);
    if (!LoRa.begin(868E6)) {
        printf("Starting LoRa failed!\n");
     }else
     {
        printf("Starting LoRa successful!\n");
     }
    int cursor_start_x = 8;
    int cursor_start_y = 60;
    int cursor_curr_x = cursor_start_x;
    int cursor_curr_y = cursor_start_y;
    lcd_init(lcd_info);
    lcd_post_fullscreen_picture(lcd_info,&epd_bitmap_ehe[0],sizeof(epd_bitmap_ehe),true);
    sleep_ms(2000);
    
     for(int k=0; k<64;k++)
     {
        lora_buffer[k] = '0';
     }
     lcd_clear_screen(lcd_info);
     while (true) {
        
        int packetSize = LoRa.parsePacket();

        if (packetSize) {
            char lora_buffer_new[64];
            
            int i = 0;
    
            if(LoRa.available())
            {
                while (LoRa.available() && i < 63) {
                    char c = (char)LoRa.read();
                    lora_buffer_new[i++] = c;
                }
            }
            lora_buffer_new[i] = '\0'; 
            if(strcmp(lora_buffer,lora_buffer_new) != 0)
            {
                int index = 0;
                int index_end = 0;
                lcd_m.index_start = &lora_buffer_new[0];
                lcd_m.index_split = strchr(lora_buffer_new,'|');
                lcd_m.index_end = strchr(lora_buffer_new,'@');
                index = lcd_m.index_split - lcd_m.index_start+1;
                lcd_write_word(lcd_info, 0, 15, Sound, strlen(Sound));
                lcd_write_word(lcd_info, 0, 31, &lora_buffer_new[0], sizeof(uint8_t)*(index-1));
                index_end = lcd_m.index_end - (lcd_m.index_split);
                lcd_write_word(lcd_info, 0, 47,Accuracy,strlen(Accuracy));
                lcd_write_word(lcd_info, 0, 63, &lora_buffer_new[index],sizeof(uint8_t)*(index_end-1));
                strcpy(lora_buffer,lora_buffer_new);
            }
        }
        sleep_ms(250);
    }
}
    
