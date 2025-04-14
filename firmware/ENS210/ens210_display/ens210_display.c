#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ScioSense_Ens21x.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 24
#define I2C_SCL 25

void delay(uint32_t duration){
    sleep_ms(duration);
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
