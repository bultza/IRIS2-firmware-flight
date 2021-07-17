/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#ifndef I2C_H_
#define I2C_H_


#include <msp430.h>
#include <stdint.h>
//#include "configuration.h"
#include "clock.h"
#include "libhal.h"

volatile extern uint8_t confRegWrite_flg_;
volatile extern uint8_t loadDefault_flg_;

#define I2C_BUS00 0
#define I2C_BUS01 1

#define I2CTIMEOUTCYCLES    10000UL   //30000 for 8MHz at 400kHz

//INA226 Registers (further information on the INA226 datasheet)
//#define INA_ADDRESS 0x40
#define INA_CONF_REG 0x00       //INA226 Configuration register address (page 22)
#define INA_VSHUNT 0x01         //INA226 Shunt Voltage address (page 24)
#define INA_VBUS 0x02           //INA226 Bus Voltage address (page 24)
#define INA_PWR 0x03            //INA226 Power adress  (page 24)
#define INA_CURRENT 0x04        //INA226 Current register address (page 25)
#define INA_CAL_REG 0x05        //INA226 Calibration register (page 25)
#define INA_MASK_REG 0x06       //INA226 Alarm configuration (page 25)
#define INA_ALIMIT_REG 0x07     //INA226 Alarm limit register (Page 26)
#define INA_ID 0xFE             //INA226 Manufacturer register (Page26)



struct I2cPort
{
    uint16_t baseAddress;
    uint8_t name;
    uint8_t isOnError;
    uint8_t inRepeatedStartCondition;
    uint8_t pointerAddress;
    uint8_t repeatedStart;
};


int8_t i2c_begin_transmission(uint8_t busSelect, uint8_t address,
                              uint8_t asReceiver);
int8_t i2c_write_firstbyte(uint8_t busSelect, uint8_t *buffer);
int8_t i2c_write(uint8_t busSelect, uint8_t address, uint8_t *buffer,
                     uint16_t length, uint8_t repeatedStart);
int8_t i2c_requestFrom(uint8_t busSelect, uint8_t address, uint8_t *buffer,
                           uint16_t length, uint8_t repeatedStart);

void i2c_master_init();
//void i2c_TMP75_getTemperatures();
//void i2c_inaRead();
//void i2c_inaConfig();


#endif /* I2C_H_ */
