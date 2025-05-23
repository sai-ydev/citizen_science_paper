#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ScioSense_Ens21x.h"
#include "string.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 24
#define I2C_SCL 25
#define I2C_TIMEOUT make_timeout_time_ms(10)
#define ENS210_ADDR 0x43

void delay(uint32_t duration){
    sleep_ms(duration);
}

//Result  (*read)     (void* config, const uint16_t address, uint8_t* data, const size_t size);
Result i2c_read(void* config, const uint16_t address, uint8_t* data, const size_t size)
{
    uint8_t ret;
    uint8_t write_buf = address;

    ret = i2c_write_blocking_until(config, ENS210_ADDR, &write_buf, 1, true, I2C_TIMEOUT);
    if(ret != 1)
    {
        return RESULT_IO_ERROR;
    }

    ret = i2c_read_blocking_until(config, ENS210_ADDR, data, size, false, I2C_TIMEOUT);
    if(ret != size)
    {
        return RESULT_IO_ERROR;
    }
    return RESULT_OK;
}
//Result  (*write)    (void* config, const uint16_t address, uint8_t* data, const size_t size);
Result i2c_write(void* config, uint16_t address, uint8_t* data, const size_t size)
{
    uint8_t ret;
    uint8_t write_buffer[size + 1];

    write_buffer[0] = address;
    memcpy(&write_buffer[1], data, size);

    ret = i2c_write_blocking_until(config, ENS210_ADDR, write_buffer, size + 1, true, I2C_TIMEOUT);
    if(ret != (size + 1))
    {
        printf("Failed to write to i2c bus.\n");
        return RESULT_IO_ERROR;
    }

    return RESULT_OK;
}

int main()
{
    ScioSense_Ens21x_IO temp_sensor;
    temp_sensor.wait = &delay;

    stdio_init_all();

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
;