/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */
#include "i2c_MS5611.h"

//Calibration constants inside the ROM of the pressure sensor
uint16_t baro_ROM_C[6];

/**
 * It reads the calibration Data from the ROM of the barometer:
 *   C1: Pressure sensitivity (C_COEFFICIENTS[0])
 *   C2: Pressure offset (C_COEFFICIENTS[1])
 *   C3: Temp. coefficient of pres. sensitivity (C_COEFFICIENTS[2])
 *   C4: Temp. coefficient of pres. offset (C_COEFFICIENTS[3])
 *   C5: Reference temp. (C_COEFFICIENTS[4])
 *   C6: Temp. coefficient of the temp. (C_COEFFICIENTS[5])
 */
int8_t ms5611_readCalibrationData(void)
{
    int8_t status = 0;



    uint8_t coeffAddresses[6] = {MS5611_REG_C1,
                                 MS5611_REG_C2,
                                 MS5611_REG_C3,
                                 MS5611_REG_C4,
                                 MS5611_REG_C5,
                                 MS5611_REG_C6};

    uint8_t i;
    for (i = 0; i < 6; i++)
    {
        int8_t ack = i2c_write(I2C_BUS00,
                               MS5611_ADDRESS,
                               &coeffAddresses[i],
                               1,
                               0);
        if (ack == 0)
        {
            uint8_t coeffParts[2];
            ack = i2c_requestFrom(I2C_BUS00,
                                  MS5611_ADDRESS,
                                  coeffParts,
                                  2,
                                  0);

            baro_ROM_C[i] = ((uint16_t) coeffParts[0] << 8)
                              | ((uint16_t) coeffParts[1]);
        }

        if (ack != 0)
            status = -1;
    }

    return status;
}

/**
 * Initializes the barometer.
 */
int8_t i2c_MS5611_init(void)
{
    // Reset MS5611 device
    uint8_t cmd = MS5611_RESET;
    int8_t ack = i2c_write(I2C_BUS00, MS5611_ADDRESS, &cmd, 1, 0);

    // Wait 9 milliseconds (conversion time is 8.22 ms according to datasheet)
    sleep_ms(9);

    // Read calibration coefficients (C coefficients)
    ms5611_readCalibrationData();

    return ack;
}

/**
 * It reads the temperature and pressure from the ADC of the MS5611. After each
 * command it needs to wait for 8.22ms so this function is blocking for at
 * least 18ms!!
 * It returns:
 *  D1: Digital pressure value (D_coefficients[0])
 *  D2: Digital temperature value (D_coefficients[1])
 */
int8_t ms5611_readDigitalPresAndTempData(uint32_t * rawPressure, uint32_t * rawTemperature)
{
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
            uint8_t buffer[3];
            ack = i2c_requestFrom(I2C_BUS00, MS5611_ADDRESS, buffer, 3, 0);
            *rawPressure = ((uint32_t) buffer[0] << 16)
                         | ((uint32_t) buffer[1] <<  8)
                         | ((uint32_t) buffer[2]      );
        }
    }
    else
        return ack;

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
            uint8_t buffer[3];
            ack = i2c_requestFrom(I2C_BUS00, MS5611_ADDRESS, buffer, 3, 0);
            *rawTemperature = ((uint32_t) buffer[0] << 16)
                            | ((uint32_t) buffer[1] <<  8)
                            | ((uint32_t) buffer[2]      );
        }
        else
            return ack;
    }

    return ack;
}

/**
 * Returns the pressure in hundredths of mbar (10^-2 mbar).
 * Function takes between 27-28 ms to compute pressure.
 *
 * Consider changing the code to follow https://github.com/rgalarcia/iris2/blob/main/IRIS1/01_design/C%C3%B3digo_fuente/IRIS_src/IRIS_037_Pressure.ino
 */
int8_t i2c_MS5611_getPressure(int32_t * pressure, int32_t * temperature)
{
    int64_t off, sens;
    uint32_t rawTemperature;
    uint32_t rawPressure;
    int32_t dT;

    //Read DAC values from the sensor
    int8_t error = ms5611_readDigitalPresAndTempData(&rawPressure, &rawTemperature);

    //Error in I2C?
    if(error != 0)
        return error;

    // Difference between actual and reference temperature
    dT = (int32_t)rawTemperature - (int32_t)baro_ROM_C[4] * 256;

    // Actual temperature (-40 deg C ... 85 deg C with 0.01 deg C resolution)
    *temperature = (int64_t) 2000 + (int64_t) (dT) * (int64_t) baro_ROM_C[5] / 8388608;

    int64_t t2 = 0;
    int64_t off2 = 0;
    int64_t sens2 = 0;

    //Temperature compensation if low temperatures! (<20ºC and <-15ºC)
    if (*temperature < 2000)
    {
        t2 = ((int64_t)(dT) * (int64_t)(dT)) / 2147483648;
        off2 = 5 * (((int64_t)(*temperature) - 2000)*((int64_t)(*temperature) - 2000)) / 2;
        sens2 = 5 * (((int64_t)(*temperature) - 2000)*((int64_t)(*temperature) - 2000)) / 4;

        if (*temperature < -1500)
        {
            off2 = off2 + 7 * (((int64_t)(*temperature) + 1500)*((int64_t)(*temperature) + 1500));
            sens2 = sens2 + 11 * (((int64_t)(*temperature) + 1500)*((int64_t)(*temperature) + 1500)) / 2;
        }
    }

    *temperature = *temperature - t2;

    // Offset at actual temperature
    off = (int64_t)baro_ROM_C[1] * 65536 + ((int64_t)baro_ROM_C[3] * dT) / 128;
    off = off - off2;

    // Sensitivity at actual temperature
    sens = (int64_t)baro_ROM_C[0] * 32768 + ((int64_t)baro_ROM_C[2] * dT) / 256;
    sens = sens - sens2;

    // Temperature compensated pressure (10 ... 1200 mbar with 0.01 mbar resolution)
    *pressure = ((int64_t)rawPressure * (int64_t)(sens) / 2097152 - (int64_t)(off)) / 32768;

    return error;
}

/**
 * Returns the altitude in cm. Input is pressure in thousands of millibars.
 * 102400 is sea level.
 *
 * - Troposfera: Z=(T0/L)*[(P/P0)^(-R*L/g)-1]
 *   - Baja Estratosfera: Z=11000-(R*T11k/g)*ln(P/P11k)
 *   - Alta Estratosfera: Z=25000+(T25k/L)*[(P/P25k)^(-R*L/g)-1]
 *  Donde:
 *       T0=288,15
 *       L=-0,0065
 *       P0=101325
 *       R=287
 *       g=9,81
 *       T11k=216,65
 *       P11k=22552
 *       T25k=216,65
 *       P25k=2481
 *   Con ln=Logaritmo Neperiano, las presiones en Pascales,
 *   las Temperaturas en Kelvin, y P la presion medida.
 */
int32_t calculateAltitude(int32_t pressureInt)
{
    float pressure = (float)pressureInt / 100.0;
    float altitude;
    if(pressure > 226.32)  //11km
    {
        //We are at troposphere
        //Z=(T0/L)*[(P/P0)^(-R*L/g)-1];
        altitude = (-44330.8 * (pow(pressure / 1013.25, 0.190163) - 1.0));
    }
    else if(pressure > 24.81)  //25km??
    {
        //We are at stratosphere
        //Z=11000-(R*T11k/g)*ln(P/P11k)
        altitude = (11000.0 - (6338.282 * log(pressure / 225.52)));
    }
    else if(pressure > 1.1091)  //39.869km
    {
        //We are at high stratosphere
        //Z=25000+(T25k/L)*[(P/P25k)^(-R*L/g)-1]
        altitude = (25000.0 + (-33330.8 * (pow(pressure/24.81, 0.190163) - 1.0)));
    }
    else
        altitude = 39870.0;  //Clip at that hight

    return (int32_t)(altitude * 100.0);
}
