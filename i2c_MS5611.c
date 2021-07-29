/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */


#include "i2c_MS5611.h"

//Private functions:
int8_t ms5611_readCalibrationData(void);
int8_t ms5611_readDigitalPresAndTempData(uint32_t * dCoefficients);
int8_t ms5611_calculateTemperature(uint32_t * dCoefficients, int32_t * dT, int32_t * temp);
int8_t ms5611_secondOrderTemperatureCompensation(int32_t * dT, int32_t * temp, int64_t * off2, int64_t * sens2, int64_t * p2);
int8_t ms5611_calculateTempAndCompensatedPres(int64_t * off, int64_t * sens, int32_t * p);

// Constant
uint16_t C_COEFFICIENTS[6];

/**
 * Initializes the barometer.
 */
int8_t i2c_MS5611_init(void)
{
    // Reset MS5611 device
    uint8_t cmd = MS5611_RESET;
    int8_t ack = i2c_write(I2C_BUS00, MS5611_ADDRESS, &cmd, 1, 0);

    // Wait 9 milliseconds  (conversion time is 8.22 ms according to datasheet)
    sleep_ms(9);

    // Read calibration coefficients (C coefficients)
    ms5611_readCalibrationData();

    return ack;
}

int8_t ms5611_readCalibrationData(void)
{
    int8_t status;

    // C1: Pressure sensitivity (C_COEFFICIENTS[0])
    // C2: Pressure offset (C_COEFFICIENTS[1])
    // C3: Temp. coefficient of pres. sensitivity (C_COEFFICIENTS[2])
    // C4: Temp. coefficient of pres. offset (C_COEFFICIENTS[3])
    // C5: Reference temp. (C_COEFFICIENTS[4])
    // C6: Temp. coefficient of the temp. (C_COEFFICIENTS[5])

    uint8_t coeffAddresses[6] = {MS5611_REG_C1, MS5611_REG_C2, MS5611_REG_C3, MS5611_REG_C4, MS5611_REG_C5, MS5611_REG_C6};

    uint8_t i;
    for (i = 0; i < 7; i++)
    {
        int8_t ack = i2c_write(I2C_BUS00, MS5611_ADDRESS, &coeffAddresses[i], 1, 0);
        if (ack == 0)
        {
            uint8_t coeffParts[2];
            ack = i2c_requestFrom(I2C_BUS00, MS5611_ADDRESS, coeffParts, 2, 0);
            C_COEFFICIENTS[i] = ((uint16_t) coeffParts[0] << 8) | ((uint16_t) coeffParts[1]);
        }

        if (ack != 0)
            status = -1;
    }

    return status;
}

int8_t ms5611_readDigitalPresAndTempData(uint32_t * dCoefficients)
{
    // D1: Digital pressure value (D_coefficients[0])
    // D2: Digital temperature value (D_coefficients[1])

    // Ask MS5611 to convert D1
    uint8_t cmd = MS5611_CONVERT_D1;
    int8_t ack = i2c_write(I2C_BUS00, MS5611_ADDRESS, &cmd, 1, 0);

    // Wait 9 milliseconds  (conversion time is 8.22 ms according to datasheet)
    sleep_ms(9);

    // Read D1
    if (ack == 0)
    {
        cmd = MS5611_ADC_READ;
        ack = i2c_write(I2C_BUS00, MS5611_ADDRESS, &cmd, 1, 0);
        if (ack == 0)
        {
            uint8_t coeffParts[3];
            ack = i2c_requestFrom(I2C_BUS00, MS5611_ADDRESS, coeffParts, 3, 0);
            dCoefficients[0] = (((uint32_t) coeffParts[0] << 16) | ((uint32_t) coeffParts[1] << 8)) | ((uint32_t) coeffParts[2]);
        }
    }

    // Ask MS5611 to convert D2
    cmd = MS5611_CONVERT_D2;
    ack = i2c_write(I2C_BUS00, MS5611_ADDRESS, &cmd, 1, 0);

    // Wait 9 milliseconds (conversion time is 8.22 ms according to datasheet)
    sleep_ms(9);

    // Read D2
    if (ack == 0)
    {
        cmd = MS5611_ADC_READ;
        ack = i2c_write(I2C_BUS00, MS5611_ADDRESS, &cmd, 1, 0);
        if (ack == 0)
        {
            uint8_t coeffParts2[3];
            ack = i2c_requestFrom(I2C_BUS00, MS5611_ADDRESS, coeffParts2, 3, 0);
            dCoefficients[1] = (((uint32_t) coeffParts2[0] << 16) | ((uint32_t) coeffParts2[1] << 8)) | ((uint32_t) coeffParts2[2]);
        }
    }

    return ack;
}

int8_t ms5611_calculateTemperature(uint32_t * dCoefficients, int32_t * dT, int32_t * temp)
{
    ms5611_readDigitalPresAndTempData(dCoefficients);

    // Difference between actual and reference temperature
    *dT = (int32_t)dCoefficients[1] - (int32_t)C_COEFFICIENTS[4] * 256;

    // Actual temperature (-40 deg C ... 85 deg C with 0.01 deg C resolution)
    *temp = (int64_t) 2000 + (int64_t) (*dT) * (int64_t) C_COEFFICIENTS[5] / 8388608;

    return 0;
}

int8_t ms5611_secondOrderTemperatureCompensation(int32_t * dT, int32_t * temp, int64_t * t2, int64_t * off2, int64_t * sens2)
{
    if (*temp < 2000)
    {
        *t2 = ((int64_t)(*dT) * (int64_t)(*dT)) / 2147483648;
        *off2 = 5 * (((int64_t)(*temp) - 2000)*((int64_t)(*temp) - 2000)) / 2;
        *sens2 = 5 * (((int64_t)(*temp) - 2000)*((int64_t)(*temp) - 2000)) / 4;

        if (*temp < -1500)
        {
            *off2 = *off2 + 7 * (((int64_t)(*temp) + 1500)*((int64_t)(*temp) + 1500));
            *sens2 = *sens2 + 11 * (((int64_t)(*temp) + 1500)*((int64_t)(*temp) + 1500)) / 2;
        }
    }
    else
    {
        *t2 = 0;
        *off2 = 0;
        *sens2 = 0;
    }

    return 0;
}

int8_t ms5611_calculateTempAndCompensatedPres(int64_t * off, int64_t * sens, int32_t * p)
{
    uint32_t dCoefficients[2];
    int32_t dT, temp;
    ms5611_calculateTemperature(dCoefficients, &dT, &temp);

    int64_t t2, off2, sens2;
    ms5611_secondOrderTemperatureCompensation(&dT, &temp, &t2, &off2, &sens2);

    temp = temp - t2;

    // Offset at actual temperature
    *off = (int64_t)C_COEFFICIENTS[1] * 65536 + ((int64_t)C_COEFFICIENTS[3] * dT) / 128;
    *off = *off - off2;

    // Sensitivity at actual temperature
    *sens = (int64_t)C_COEFFICIENTS[0] * 32768 + ((int64_t)C_COEFFICIENTS[2] * dT) / 256;
    *sens = *sens - sens2;

    // Temperature compensated pressure (10 ... 1200 mbar with 0.01 mbar resolution)
    *p = ((int64_t)dCoefficients[0] * (int64_t)(*sens) / 2097152 - (int64_t)(*off)) / 32768;

    return 0;
}

/**
 * Returns the pressure in hundredths of mbar (10^-2 mbar).
 * Function takes between 27-28 ms to compute pressure.
 */
int8_t i2c_MS5611_getPressure(int32_t * pressure)
{
    int64_t off, sens;
    int32_t p;

    ms5611_calculateTempAndCompensatedPres(&off, &sens, &p);
    *pressure = p;

    return 0;
}

/**
 * Returns the altitude in cm.
 */
int8_t i2c_MS5611_getAltitude(int32_t * pressure, int32_t * altitude)
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

    if(*pressure > 22632)  //11km
    {
      //We are at troposphere
      //Z=(T0/L)*[(P/P0)^(-R*L/g)-1];
      *altitude = (-4433080 * (pow(*pressure / 101325., 0.190163) - 1.));
    }
    else if(*pressure > 2481)  //25km??
    {
      //We are at stratosphere
      //Z=11000-(R*T11k/g)*ln(P/P11k)
      *altitude = (1100000 - (6338.282 * log(*pressure / 22552.)));
    }
    else if(*pressure > 111)  //47km
    {
      //We are at high stratosphere
      //Z=25000+(T25k/L)*[(P/P25k)^(-R*L/g)-1]
      *altitude = (2500000 + (-33330.8 * (pow(*pressure/2481., 0.190163) - 1.)));
    }
    else
      *altitude = 5000000;  //Return a veeery high altitude

    return 0;
}
