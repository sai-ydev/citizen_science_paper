#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "ws2812.pio.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 24
#define I2C_SCL 25

#define PDIS_EN 8
#define SOIL_INT 1
#define SOILTEMP_INT 2
#define WS2812_PIN 16

#define PH_REGISTER 4
#define PH_BYTES 4
#define PH_ADDRESS 0x65
#define RTD_ADDRESS 0x68

#define PH_BYTES 4
#define PH_ADDRESS 0x65
#define RTD_ADDRESS 0x68

#define I2C_TIMEOUT 4

#define VERSION_REG     0x00
#define INTERRUPT_REG   0x04
#define ACTIVE_REG      0x06
#define READING_REG     0x16

#define INT_INVERT      0x08
#define ACTIVATE        0x01

#define IS_RGBW false
#define NUM_PIXELS 32

bool new_pH_reading = false;
uint8_t i2c_buf_write[2];
uint8_t i2c_buf_read[4];

const uint8_t seven_seg_lookup[] = {
    0x3F,//0 
    0x06,//1 
    0x5B,//2 
    0x4F,//3 
    0x66,//4 
    0x6D,//5 
    0x7D,//6
    0x07,//7 
    0x7F,//8
    0x6F//9
};

union pH_buffer                
{
	uint8_t i2c_data[4];      //i2c data is written into this buffer          
	uint32_t intermittent_data;	 // this variable is cast to float to read pH
};

void pH_callback(uint gpio, uint32_t events){
    new_pH_reading = true;
}

void i2c_read(uint8_t dev_addr, uint8_t reg, uint8_t len){
    i2c_buf_write[0] = reg;
    i2c_write_blocking_until(I2C_PORT, dev_addr, i2c_buf_write, 1, true, make_timeout_time_ms(I2C_TIMEOUT));
    
    i2c_read_blocking_until(I2C_PORT, dev_addr, i2c_buf_read, len, true, make_timeout_time_ms(I2C_TIMEOUT));
}

void i2c_write(uint8_t dev_addr, uint8_t reg, uint8_t command, uint8_t length){
    i2c_buf_write[0] = reg;
    i2c_buf_write[1] = command;
    i2c_write_blocking_until(I2C_PORT, PH_ADDRESS, i2c_buf_write, length, true, make_timeout_time_ms(100));
}

static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
            ((uint32_t) (r) << 8) |
            ((uint32_t) (g) << 16) |
            (uint32_t) (b);
}

int main()
{
    union pH_buffer pH_data;
    float pH_value;
    uint16_t pH_number;
    bool reading_enabled = false;

    PIO pio;
    uint sm;
    uint offset;
    int counter = 0;
    uint8_t pos_nums[4] = {0, 0, 0, 0};

    stdio_init_all();
    /* Wait for serial port to connect*/

    printf("\n pH Meter Display\n");

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 100*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

    gpio_init(PDIS_EN);
    gpio_init(SOIL_INT);
    gpio_init(SOILTEMP_INT);

    gpio_set_dir(PDIS_EN, true);
    gpio_set_dir(SOIL_INT, false);
    gpio_set_dir(SOILTEMP_INT, false);
    
    // Enable I2C Isolator
    gpio_put(PDIS_EN, true);
    gpio_set_irq_enabled_with_callback(SOIL_INT, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &pH_callback);
    
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(I2C_SDA, I2C_SCL, GPIO_FUNC_I2C));
    
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, WS2812_PIN, 1, true);
    hard_assert(success);

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
    for(uint8_t i = 0; i < NUM_PIXELS; i++){
        put_pixel(pio, sm, urgb_u32(0, 0, 0));
    }
    
    while (true) {
        while(!reading_enabled){
            
            i2c_read(PH_ADDRESS, VERSION_REG, 2);

            if(i2c_buf_read[0] == 1 && !reading_enabled){
                reading_enabled = true;
                printf("Setup complete\n");
            } else{
                continue;
            }

            i2c_write(PH_ADDRESS, INTERRUPT_REG, INT_INVERT, 2);
            i2c_write(PH_ADDRESS, ACTIVE_REG, ACTIVATE, 2);
            
            i2c_read(PH_ADDRESS, READING_REG, 4);

            pH_data.i2c_data[3] = i2c_buf_read[0];
            pH_data.i2c_data[2] = i2c_buf_read[1];
            pH_data.i2c_data[1] = i2c_buf_read[2];
            pH_data.i2c_data[0] = i2c_buf_read[3];

            pH_value = (float)pH_data.intermittent_data;
            pH_value /= 1000.0;
            printf("pH value = %2.2f\n", pH_value);
        }

        if(new_pH_reading){
            i2c_read(PH_ADDRESS, READING_REG, 4);

            pH_data.i2c_data[3] = i2c_buf_read[0];
            pH_data.i2c_data[2] = i2c_buf_read[1];
            pH_data.i2c_data[1] = i2c_buf_read[2];
            pH_data.i2c_data[0] = i2c_buf_read[3];

            pH_value = (float)pH_data.intermittent_data;
            pH_value /= 1000.0;
            printf("pH value = %3.2f\n", pH_value);
            pH_number = (uint16_t)(pH_value * 100);

            for(uint8_t i = 0; i < 4; i++){
                pos_nums[i] = pH_number % 10;
                pH_number = pH_number / 10;
            }

            for(uint8_t i = 0; i < 4; i++){
                uint8_t digit = seven_seg_lookup[pos_nums[i]];
                if(i == 2){
                    digit |= 0x80;
                }
                for(uint8_t j = 0; j < 8; j++){
                    if((1 << j) & digit){
                        put_pixel(pio, sm, urgb_u32(0, 0, 0xFF));
                    } else{
                        put_pixel(pio, sm, urgb_u32(0, 0, 0));
                    }
                }
            }
            new_pH_reading = false;
        }
    }
}
