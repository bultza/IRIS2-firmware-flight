/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#ifndef I2C_INA_H_
#define I2C_INA_H_

#include <msp430.h>
#include <stdint.h>
#include "i2c.h"

//INA226 Registers (further information on the INA226 datasheet)
#define INA_ADDRESS 0x40
#define INA_CONF_REG 0x00       //INA226 Configuration register address (page 22)
#define INA_VSHUNT 0x01         //INA226 Shunt Voltage address (page 24)
#define INA_VBUS 0x02           //INA226 Bus Voltage address (page 24)
#define INA_PWR 0x03            //INA226 Power adress  (page 24)
#define INA_CURRENT 0x04        //INA226 Current register address (page 25)
#define INA_CAL_REG 0x05        //INA226 Calibration register (page 25)
#define INA_MASK_REG 0x06       //INA226 Alarm configuration (page 25)
#define INA_ALIMIT_REG 0x07     //INA226 Alarm limit register (Page 26)
#define INA_ID 0xFE             //INA226 Manufacturer register (Page26)

struct INAData
{
    int16_t current;
    int16_t voltage;
    int8_t error;
};


//******************************************************************************
//* PUBLIC FUNCTION DECLARATIONS :                                             *
//******************************************************************************
int8_t i2c_INA_init();
int8_t i2c_INA_read(struct INAData *data);



#endif /* I2C_INA_H_ */
