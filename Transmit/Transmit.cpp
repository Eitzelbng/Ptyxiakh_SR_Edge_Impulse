#define LORA_MODE_TRANSMITER
//#define SampleMode
// #define PRINT

// LORA DEFINES

#define SPI_BAUDRATE 8000000
#define LORA_PORT spi0
#define LORA_MISO 16
#define LORA_NSS 17
#define LORA_SCK 18
#define LORA_MOSI 19
#define LORA_RST 20
#define LORA_DIO0 21

// AI   
#define AVG_SAMPLE_RATE 14

// MICROPHONE DEFINES

#define STATE_MACHINE 0
#define PIO_WS_PIN 12
#define PIO_SCK_PIN 11
#define PIO_SEL_PIN 13
#define PIO_DO_PIN 10

// #define PIO_WS_CLOCK_TICKS 30 // 16 khz
#define PIO_WS_CLOCK_TICKS 15 // 16 khz

#define DMA_CHANNEL 0           // Choose an available DMA channel
#define AUDIO_BUFFER_MINI_SIZE 4000//3200
#define AUDIO_BUFFER_COLLECTOR0_ID 0
#define AUDIO_BUFFER_COLLECTOR1_ID 1

#define I2S_PIO_FRQ_INT 36  //  } TOTAL AT 16khz WS and x64 AT SCK
#define I2S_PIO_FRQ_DIV 140 //  }

// OUTPUT AUDIO
#define AUDIO_SM 1
#define AUDIO_PWM_PIN 26
#define AUDIO_PIO_FRQ_INT 1172 // 4660
#define AUDIO_PIO_FRQ_DIV 0    // 190

// #define PIO_SCK_CLOCK_FREQ 460 // 3.07 Mhz

#define ATTENUATION (0.000316227f) //! decent -70db attenuation


#define run_cycle 4

#include <stdio.h>
#include <cstdint>
#include "string.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/pio.h"
#include "pico/multicore.h"

#include "hardware/pwm.h"
#include "hardware/timer.h"
#include "hardware/sync.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include <cstdlib>
#include <cstdint>
#include <string>

#include "i2s.pio.h"
#include "Libraries/Edge_Impulse/edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "Libraries/Feature_samples.h"

#include "Libraries/Edge_Impulse/tflite-model/tflite_learn_794595_164.h"

#include "Libraries/sx1276/LoRa-RP2040.h"
#include "Libraries/sx1276/Print.h"

using namespace std;

class MIC_MAP
{
public:
    uint16_t sck_pin = PIO_SCK_PIN;
    uint16_t ws_pin = PIO_WS_PIN;
    uint16_t sel_pin = PIO_SEL_PIN;
    uint16_t do_pin = PIO_DO_PIN;
    uint dma_channel = DMA_CHANNEL;
    uint dma_irq = DMA_IRQ_0;
    uint32_t buffer_mini_0[AUDIO_BUFFER_MINI_SIZE];
    uint32_t buffer_mini_1[AUDIO_BUFFER_MINI_SIZE];
    //int32_t buffer_copy[AUDIO_BUFFER_MINI_SIZE];
    uint32_t* ai_processing_ptr;
    uint8_t current_dma_idx;
};

static MIC_MAP mic_map;

PIO i2s_pio = pio0;
PIO audio_pio = pio1;


volatile bool new_data_ready = false;
bool buffer_ready = false;
bool copy_buffer = false;
bool buffer_copied = false;
uint8_t part = 0;

bool pack_ready = false;

//#define SampleMode



volatile char classification_name[64];
volatile float classification_val;
// LORA_MODULE Lora;

static ei_impulse_result_t result = {0};

void __isr dma_handler()
{

    dma_channel_acknowledge_irq0(mic_map.dma_channel);
    if (mic_map.current_dma_idx == 0) {
        mic_map.ai_processing_ptr = mic_map.buffer_mini_0; // Give buffer 0 to AI
        dma_channel_set_write_addr(mic_map.dma_channel, mic_map.buffer_mini_1, false); // DMA fills 1
        dma_hw->ch[mic_map.dma_channel].al1_transfer_count_trig = AUDIO_BUFFER_MINI_SIZE;
        mic_map.current_dma_idx = 1;

    } else {
        mic_map.ai_processing_ptr = mic_map.buffer_mini_1; // Give buffer 1 to AI
        dma_channel_set_write_addr(mic_map.dma_channel, mic_map.buffer_mini_0, false); // DMA fills 0
        dma_hw->ch[mic_map.dma_channel].al1_transfer_count_trig = AUDIO_BUFFER_MINI_SIZE;
        mic_map.current_dma_idx = 0;
    }
    if(copy_buffer == false)copy_buffer = true;
    buffer_ready  = true;
}

void mic_dma_setup(PIO i2s_pio, uint sm)
{
    mic_map.dma_channel = dma_claim_unused_channel(true);
    irq_set_exclusive_handler(mic_map.dma_irq, dma_handler);
    irq_set_enabled(mic_map.dma_irq, true);
    dma_channel_config c = dma_channel_get_default_config(mic_map.dma_channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    //channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_dreq(&c, pio_get_dreq(i2s_pio, sm, false));
    dma_channel_configure(mic_map.dma_channel, &c, &mic_map.buffer_mini_0[0], &i2s_pio->rxf[sm], AUDIO_BUFFER_MINI_SIZE, false);
    mic_map.current_dma_idx = 0;
    dma_channel_set_irq0_enabled(mic_map.dma_channel, true);
    dma_channel_start(mic_map.dma_channel);
}

void mic_pio_setup(PIO i2s_pio, uint offset)
{
    pio_gpio_init(i2s_pio, mic_map.ws_pin);
    pio_gpio_init(i2s_pio, mic_map.sck_pin);
    pio_gpio_init(i2s_pio, mic_map.sel_pin);
    pio_sm_set_consecutive_pindirs(i2s_pio, STATE_MACHINE, PIO_SCK_PIN, 3, true);
    pio_sm_set_consecutive_pindirs(i2s_pio, STATE_MACHINE, PIO_DO_PIN, 1, false);
    pio_gpio_init(i2s_pio, mic_map.do_pin);
    pio_sm_config pio_config = i2s_program_get_default_config(offset);
    sm_config_set_in_pins(&pio_config, mic_map.do_pin);
    sm_config_set_in_shift(&pio_config, false, true , 32);
    sm_config_set_sideset_pins(&pio_config, mic_map.sck_pin);
    pio_sm_init(i2s_pio, STATE_MACHINE, offset, &pio_config);
    //pio_sm_set_clkdiv_int_frac(pio0, STATE_MACHINE, 36, 159); for 128 ticks per channel
    pio_sm_set_clkdiv_int_frac(pio0, STATE_MACHINE, 18, 79);

    //pio_sm_set_clkdiv_int_frac(pio0, STATE_MACHINE, I2S_PIO_FRQ_INT, I2S_PIO_FRQ_DIV);
    pio_sm_set_enabled(i2s_pio, STATE_MACHINE, true);
    i2s_pio->txf[0] = 29 ; //PIO_WS_CLOCK_TICKS;
    mic_dma_setup(i2s_pio, STATE_MACHINE);
}

void init_audio_hardware()
{
    gpio_set_function(AUDIO_PWM_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(AUDIO_PWM_PIN);
    pwm_set_wrap(slice_num, 6553);
    pwm_set_clkdiv(slice_num, 1.0f);
    pwm_set_enabled(slice_num, true);
}

void prog_init()
{
    int mic_offset = pio_add_program(i2s_pio, &i2s_program);
    mic_pio_setup(i2s_pio, mic_offset);
    init_audio_hardware();
}


int samples_func(size_t offset, size_t length, float *out_ptr)
{
    for (size_t i = 0; i < length; i++)
    {

        uint32_t raw32 = mic_map.ai_processing_ptr[offset + i]<<1;
        int32_t base_val = (int32_t)(raw32 >> 15);
        int16_t bv_16 = (int16_t)base_val;
        float amplified = (float)bv_16*1.5f;
        if (amplified > 32767.0f)  amplified = 32767.0f;
        if (amplified < -32768.0f) amplified = -32768.0f;

        out_ptr[i] = amplified;
    }
    return 0;
}


volatile static bool classification_finished = false;
static signal_t samples;
volatile static int runs = 0;
static float max_classified_val[EI_CLASSIFIER_LABEL_COUNT];
int max_num = 0;
int max_c = -1;
static int class_counter[EI_CLASSIFIER_LABEL_COUNT] = {};
static float class_max_val_found[EI_CLASSIFIER_LABEL_COUNT] = {};
volatile static int classification_result;






void core1_entry()
{
    while (true)
    {
        if (multicore_fifo_rvalid()) 
        {
            uint32_t msg = multicore_fifo_pop_blocking(); 
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "%s|%.4f@", classification_name, classification_val);
            LoRa.beginPacket();
            LoRa.print(buffer);
            LoRa.endPacket();
            memset(class_counter, 0, sizeof(int) * EI_CLASSIFIER_LABEL_COUNT);
            memset(class_max_val_found, 0, sizeof(float) * EI_CLASSIFIER_LABEL_COUNT);
            runs = 0;
            multicore_fifo_push_blocking(1);
        }
        sleep_ms(1);
    }
}

int main()
{
    // ... your initialization code stays exactly the same ...
    stdio_init_all();
    sleep_ms(5000);
    
    samples.total_length = AUDIO_BUFFER_MINI_SIZE;
    samples.get_data = &samples_func;
    prog_init();
    
    LoRa.setPins(17, 20, 21);
    if (!LoRa.begin(868E6)) {
        printf("Starting LoRa failed!\n");
    }
    
    #ifndef SampleMode
    multicore_launch_core1(&core1_entry);
    #endif
    run_classifier_init();

    while (true)
    {
        if (buffer_ready && runs < run_cycle)
        {
            runs++;
            EI_IMPULSE_ERROR ret = run_classifier_continuous(&samples, &result, false, true);
            buffer_ready = false;
            for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++)
            {
                if (result.classification[i].value > 0.77f)
                {
                   class_counter[i]++;
                   if(result.classification[i].value > class_max_val_found[i])
                   {
                        class_max_val_found[i] = result.classification[i].value;
                   } }}
            if(runs >= run_cycle)
            { 
                max_num = 0;
                max_c = -1;
                for(int k = 0; k < EI_CLASSIFIER_LABEL_COUNT; k++)
                {
                    if(class_counter[k] > max_num)
                    {
                        max_num = class_counter[k];
                        max_c = k;
                    }
                }
                if(max_c != -1)
                {
                    if(class_counter[max_c] >= 2)
                    {
                        classification_val = class_max_val_found[max_c];
                        strncpy((char*)classification_name, result.classification[max_c].label, sizeof(classification_name));
                    }
                    else
                    {
                        strcpy((char*)classification_name, "Noise");
                        classification_val = 1.0f; 
                    }
                }
                else
                {
                    strcpy((char*)classification_name, "Noise");
                    classification_val = 1.0f - result.classification[5].value; 
                }
                multicore_fifo_push_blocking(100); 
                multicore_fifo_pop_blocking(); 
            }
        }
        else
        {
            sleep_ms(1); 
        }
    }
}