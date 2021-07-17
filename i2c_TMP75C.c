#include "i2c_TMP75C.h"

/**
 * Init sensor(s)
 */
int8_t i2c_TMP75_init(void)
{
    uint8_t buffer[2];
    buffer[0] = TMP75_REG_CONF;
    //Set resolution to 12bits. Interrupts disabled, Switched On:
    buffer[1] = 0x60;

    //Temperature on the PCB
    i2c_write(I2C_BUS00, TMP75_ADDRESS01, buffer, 2, 0);

    //Temperature on external bus
    i2c_write(I2C_BUS01, TMP75_ADDRESS01, buffer, 2, 0);
    //i2c_write(I2C_BUS01, TMP75_ADDRESS02, buffer, 2, 0);
    //i2c_write(I2C_BUS01, TMP75_ADDRESS03, buffer, 2, 0);
    //i2c_write(I2C_BUS01, TMP75_ADDRESS04, buffer, 2, 0);

    return 0;
}

/**
 * It asks a slave TMP75 its temperature
 */
int16_t i2c_TMP75_getTemperature(uint8_t selectedBus, uint8_t i2cAddress)
{
    uint8_t buffer[2];
    int16_t result = 0;

    buffer[0] = 0;
    int8_t ack = i2c_write(selectedBus, i2cAddress, buffer, 1, 0);
    if (ack == 0)
        ack = i2c_requestFrom(selectedBus, i2cAddress, buffer, 2, 0);

    if(ack == 0)
    {
        //Make conversion
        result = (0xFF00 & ((uint16_t) buffer[0] << 8))
                | ((uint16_t) (0x00FF & buffer[1]));
        result = result >> 4;   //As per datasheet
    }
    else
        result = 32767;   //Deterrent value

    return result;
}


/**
 * Save temperature(s), you should input a pointer to an array of 5 uint16_t!
 * Results are decimals of centigrade!!!, a value of 261 is 26.1ºC
 */
int8_t i2c_TMP75_getTemperatures(int16_t *temperatures)
{
    temperatures[0] = (i2c_TMP75_getTemperature(I2C_BUS00, TMP75_ADDRESS01)*10)/16;
    //temperatures[1] = (i2c_TMP75_getTemperature(I2C_BUS01, TMP75_ADDRESS02)*10)/16;
    //temperatures[2] = (i2c_TMP75_getTemperature(I2C_BUS02, TMP75_ADDRESS03)*10)/16;
    //temperatures[3] = (i2c_TMP75_getTemperature(I2C_BUS03, TMP75_ADDRESS04)*10)/16;
    //temperatures[4] = (i2c_TMP75_getTemperature(I2C_BUS04, TMP75_ADDRESS05)*10)/16;
    return 0;
}
