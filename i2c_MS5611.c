/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */


#include "i2c_MS5611.h"
#include <math.h>

//Private functions:
int8_t ms5611_readCalibrationData(uint16_t * C_coefficients);
int8_t ms5611_readDigitalPresAndTempData(uint32_t * D_coefficients);
int8_t ms5611_calculateTemperature(uint16_t * C_coefficients, uint32_t * D_coefficients, int32_t * dT, int32_t * TEMP);
int8_t ms5611_secondOrderTemperatureCompensation(int32_t * dT, int32_t * TEMP, int64_t * OFF2, int64_t * SENS2, int64_t * P2);
int8_t ms5611_calculateTempAndCompensatedPres(int64_t * OFF, int64_t * SENS, int32_t * P);


/**
 * Initializes the barometer.
 */
int8_t i2c_MS5611_init(void)
{
    // Reset MS5611 device
    uint8_t cmd = MS5611_RESET;
    int8_t ack = i2c_write(I2C_BUS00, MS5611_ADDRESS, &cmd, 1, 0);
    return ack;
}

int8_t ms5611_readCalibrationData(uint16_t * C_coefficients)
{
    int8_t status;

    // C1: Pressure sensitivity (C_coefficients[0])
    // C2: Pressure offset (C_coefficients[1])
    // C3: Temp. coefficient of pres. sensitivity (C_coefficients[2])
    // C4: Temp. coefficient of pres. offset (C_coefficients[3])
    // C5: Reference temp. (C_coefficients[4])
    // C6: Temp. coefficient of the temp. (C_coefficients[5])

    uint8_t coeffAddresses[6] = {MS5611_REG_C1, MS5611_REG_C2, MS5611_REG_C3, MS5611_REG_C4, MS5611_REG_C5, MS5611_REG_C6};

    uint8_t i;
    for (i = 0; i < 7; i++)
    {
        int8_t ack = i2c_write(I2C_BUS00, MS5611_ADDRESS, &coeffAddresses[i], 1, 0);
        if (ack == 0)
        {
            uint8_t coeff_parts[2];
            ack = i2c_requestFrom(I2C_BUS00, MS5611_ADDRESS, coeff_parts, 2, 0);
            C_coefficients[i] = ((uint16_t) coeff_parts[0] << 8) | ((uint16_t) coeff_parts[1]);
        }

        if (ack != 0)
            status = -1;
    }

    return status;
}

int8_t ms5611_readDigitalPresAndTempData(uint32_t * D_coefficients)
{
    // D1: Digital pressure value (D_coefficients[0])
    // D2: Digital temperature value (D_coefficients[1])

    // Ask MS5611 to convert D1
    uint8_t cmd = MS5611_CONVERT_D1;
    int8_t ack = i2c_write(I2C_BUS00, MS5611_ADDRESS, &cmd, 1, 0);

    // Wait 9 milliseconds  (conversion time is 8.22 ms according to datasheet)
    __delay_cycles(72000);

    // Read D1
    if (ack == 0)
    {
        cmd = MS5611_ADC_READ;
        ack = i2c_write(I2C_BUS00, MS5611_ADDRESS, &cmd, 1, 0);
        if (ack == 0)
        {
            uint8_t coeff_parts[3];
            ack = i2c_requestFrom(I2C_BUS00, MS5611_ADDRESS, coeff_parts, 3, 0);
            D_coefficients[0] = (((uint32_t) coeff_parts[0] << 16) | ((uint32_t) coeff_parts[1] << 8)) | ((uint32_t) coeff_parts[2]);
        }
    }

    // Ask MS5611 to convert D2
    cmd = MS5611_CONVERT_D2;
    ack = i2c_write(I2C_BUS00, MS5611_ADDRESS, &cmd, 1, 0);

    // Wait 9 milliseconds (conversion time is 8.22 ms according to datasheet)
    __delay_cycles(72000);

    // Read D2
    if (ack == 0)
    {
        cmd = MS5611_ADC_READ;
        ack = i2c_write(I2C_BUS00, MS5611_ADDRESS, &cmd, 1, 0);
        if (ack == 0)
        {
            uint8_t coeff_parts2[3];
            ack = i2c_requestFrom(I2C_BUS00, MS5611_ADDRESS, coeff_parts2, 3, 0);
            D_coefficients[1] = (((uint32_t) coeff_parts2[0] << 16) | ((uint32_t) coeff_parts2[1] << 8)) | ((uint32_t) coeff_parts2[2]);
        }
    }

    return ack;
}

int8_t ms5611_calculateTemperature(uint16_t * C_coefficients, uint32_t * D_coefficients, int32_t * dT, int32_t * TEMP)
{
    ms5611_readCalibrationData(C_coefficients);
    ms5611_readDigitalPresAndTempData(D_coefficients);

    // Difference between actual and reference temperature
    *dT = (int32_t)D_coefficients[1] - (int32_t)C_coefficients[4] * 256;

    // Actual temperature (-40 deg C ... 85 deg C with 0.01 deg C resolution)
    *TEMP = (int64_t) 2000 + (int64_t) (*dT) * (int64_t) C_coefficients[5] / 8388608;

    return 0;
}

int8_t ms5611_secondOrderTemperatureCompensation(int32_t * dT, int32_t * TEMP, int64_t * T2, int64_t * OFF2, int64_t * SENS2)
{
    if (*TEMP < 2000)
    {
        *T2 = ((int64_t)(*dT) * (int64_t)(*dT)) / 2147483648;
        *OFF2 = 5 * (((int64_t)(*TEMP) - 2000)*((int64_t)(*TEMP) - 2000)) / 2;
        *SENS2 = 5 * (((int64_t)(*TEMP) - 2000)*((int64_t)(*TEMP) - 2000)) / 4;

        if (*TEMP < -1500)
        {
            *OFF2 = *OFF2 + 7 * (((int64_t)(*TEMP) + 1500)*((int64_t)(*TEMP) + 1500));
            *SENS2 = *SENS2 + 11 * (((int64_t)(*TEMP) + 1500)*((int64_t)(*TEMP) + 1500)) / 2;
        }
    }
    else
    {
        *T2 = 0;
        *OFF2 = 0;
        *SENS2 = 0;
    }

    return 0;
}

int8_t ms5611_calculateTempAndCompensatedPres(int64_t * OFF, int64_t * SENS, int32_t * P)
{
    uint16_t C_coefficients[6];
    uint32_t D_coefficients[2];
    int32_t dT, TEMP;
    ms5611_calculateTemperature(C_coefficients, D_coefficients, &dT, &TEMP);

    int64_t T2, OFF2, SENS2;
    ms5611_secondOrderTemperatureCompensation(&dT, &TEMP, &T2, &OFF2, &SENS2);

    TEMP = TEMP - T2;

    // Offset at actual temperature
    *OFF = (int64_t)C_coefficients[1] * 65536 + ((int64_t)C_coefficients[3] * dT) / 128;
    *OFF = *OFF - OFF2;

    // Sensitivity at actual temperature
    *SENS = (int64_t)C_coefficients[0] * 32768 + ((int64_t)C_coefficients[2] * dT) / 256;
    *SENS = *SENS - SENS2;

    // Temperature compensated pressure (10 ... 1200 mbar with 0.01 mbar resolution)
    *P = ((int64_t)D_coefficients[0] * (int64_t)(*SENS) / 2097152 - (int64_t)(*OFF)) / 32768;

    return 0;
}

/**
 * Returns the pressure in hundredths of mbar (10^-2 mbar).
 */
int8_t i2c_MS5611_getPressure(int32_t * pressure)
{

    uint32_t pressTotal = 0;

    uint8_t i = 0;
    uint8_t numOfSamples = 10;
    for (i; i < numOfSamples; i++)
    {
        int64_t OFF, SENS;
        int32_t P;

        ms5611_calculateTempAndCompensatedPres(&OFF, &SENS, &P);
        pressTotal += P;
    }

    *pressure = pressTotal / numOfSamples;

    return 0;
}

/**
 * Returns the altitude in cm.
 */
int8_t i2c_MS5611_getAltitude(int32_t *altitude)
{
    /*
     - Troposfera: Z=(T0/L)*[(P/P0)^(-R*L/g)-1]
     - Baja Estratosfera: Z=11000-(R*T11k/g)*ln(P/P11k)
     - Alta Estratosfera: Z=25000+(T25k/L)*[(P/P25k)^(-R*L/g)-1]

    Donde:

         T0=288,15
         L=-0,0065
         P0=101325
         R=287
         g=9,81
         T11k=216,65
         P11k=22552
         T25k=216,65
         P25k=2481

     Con ln=Logaritmo Neperiano, las presiones en Pascales, las Temperaturas en Kelvin, y P la presion medida.*/

    int32_t pressure;

    i2c_MS5611_getPressure(&pressure);

    if(pressure > 22632)  //11km
    {
      //We are at troposphere
      //Z=(T0/L)*[(P/P0)^(-R*L/g)-1];
      *altitude = (-4433080 * (pow(pressure / 101325., 0.190163) - 1.));
    }
    else if(pressure > 2481)  //25km??
    {
      //We are at stratosphere
      //Z=11000-(R*T11k/g)*ln(P/P11k)
      *altitude = (1100000 - (6338.282 * log(pressure / 22552.)));
    }
    else if(pressure > 111)  //47km
    {
      //We are at high stratosphere
      //Z=25000+(T25k/L)*[(P/P25k)^(-R*L/g)-1]
      *altitude = (2500000 + (-33330.8 * (pow(pressure/2481., 0.190163) - 1.)));
    }
    else
      *altitude = 5000000;  //Return a veeery high altitude

    return 0;
}
