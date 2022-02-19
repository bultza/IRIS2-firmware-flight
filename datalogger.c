/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#include "datalogger.h"

// GLOBAL VARIABLES

// When was the last time each sensor was monitored?
uint64_t lastTime_baroRead_ = 0;
uint64_t lastTime_inaRead_ = 0;
uint64_t lastTime_accRead_ = 0;
uint64_t lastTime_tempRead_ = 0;
uint64_t lastTime_gpioSunriseRead_ = 0;

// How many times has a sensor been read per Telemetry Line? --> to compute avgs
int16_t numTimes_baroRead_[2] = {0, 0};   // FRAM, NOR
int16_t numTimes_inaRead_[2] = {0, 0};
int16_t numTimes_accRead_[2] = {0, 0};

//Current Telemetry Line. First index corresponds to FRAM one, second index to NOR.
struct TelemetryLine currentTelemetryLine_[2];

//History of altitudes
#define ALTITUDE_HISTORY 10
struct AltitudesHistory
{
    int32_t time;
    int32_t altitude;
};
struct AltitudesHistory altitudeHistory_[ALTITUDE_HISTORY];
uint8_t altitudeHistoryIndex_ = 0;

uint8_t gpioSunriseLastStatus_ = 0;

// PUBLIC FUNCTIONS

/**
 * Reads the NOR partitions for Telemetry Lines and Events Lines. Checks where
 * last lines were written, sets addresses to continue writing on NOR.
 */
void searchAddressesNOR()
{
    uint32_t telemetryLinesLastAddress = NOR_TLM_ADDRESS;
    uint32_t eventsLinesLastAddress = NOR_EVENTS_ADDRESS;

    // Check first Telemetry Lines partition
    uint8_t readByte[4];
    while(1)
    {
        //TODO probably we should have a way of escape, something reasonable
        spi_NOR_readFromAddress(telemetryLinesLastAddress,
                                readByte,
                                4,
                                confRegister_.nor_deviceSelected);
        if((uint8_t) readByte[0] == 0xFF
                && (uint8_t) readByte[1] == 0xFF
                && (uint8_t) readByte[2] == 0xFF
                && (uint8_t) readByte[3] == 0xFF)
        {
            //read and test again just in case
            spi_NOR_readFromAddress(telemetryLinesLastAddress + sizeof(struct TelemetryLine),
                                    readByte,
                                    4,
                                    confRegister_.nor_deviceSelected );
            //Check also next one just in case there was not a power off in the middle.
            if((uint8_t) readByte[0] == 0xFF
                            && (uint8_t) readByte[1] == 0xFF
                            && (uint8_t) readByte[2] == 0xFF
                            && (uint8_t) readByte[3] == 0xFF)
            {
                //read the following line and test again just in case
                break;  //found
            }
        }
        telemetryLinesLastAddress += sizeof(struct TelemetryLine);
    }

    // Then check Event Lines partition
    while(1)
    {
        //TODO probably we should have a way of escape, something reasonable
        spi_NOR_readFromAddress(eventsLinesLastAddress,
                                readByte,
                                4,
                                confRegister_.nor_deviceSelected);
        if((uint8_t) readByte[0] == 0xFF
                && (uint8_t) readByte[1] == 0xFF
                && (uint8_t) readByte[2] == 0xFF
                && (uint8_t) readByte[3] == 0xFF)
        {
            //read the following line and test again just in case
            spi_NOR_readFromAddress(eventsLinesLastAddress + sizeof(struct EventLine),
                                    readByte,
                                    4,
                                    confRegister_.nor_deviceSelected);
            if((uint8_t) readByte[0] == 0xFF
                    && (uint8_t) readByte[1] == 0xFF
                    && (uint8_t) readByte[2] == 0xFF
                    && (uint8_t) readByte[3] == 0xFF)
            {
                break;  //found
            }
        }
        eventsLinesLastAddress += sizeof(struct EventLine);

    }
    confRegister_.nor_telemetryAddress = telemetryLinesLastAddress;
    confRegister_.nor_eventAddress = eventsLinesLastAddress;
}

/**
 * It uses the current speed based on the history of 10 altitudes
 * so the speed is in reality an average of the last 10 altitudes
 * The returned value is in m/s*100 = cm/s
 */
int32_t getVerticalSpeed()
{
    //Check if we have valid values
    uint8_t maxIndex = 0;
    uint8_t minIndex = 9;
    uint8_t i;

    uint64_t timeNow = millis_uptime();

    //Avoid calculating speeds if uptime is very low
    if(timeNow < ALTITUDE_HISTORY * 1500L)
        return 0;

    int32_t sumOfSpeeds = 0;

    //Search for the oldest and newest values
    for(i = 0; i < ALTITUDE_HISTORY; i++)
    {
        if(altitudeHistory_[i].time == 0)
            //There are not enough saved values to calculate speed!
            return 0;
        if(altitudeHistory_[i].time > altitudeHistory_[maxIndex].time)
            maxIndex = i;
        if(altitudeHistory_[i].time < altitudeHistory_[minIndex].time)
            minIndex = i;

        if(i == 0)
        {
            if(altitudeHistory_[0].time > altitudeHistory_[ALTITUDE_HISTORY - 1].time)
            {
                //Time to calculate speed:
                int32_t timeDelta = altitudeHistory_[0].time
                        - altitudeHistory_[ALTITUDE_HISTORY - 1].time;
                int32_t distanceDelta = altitudeHistory_[0].altitude
                        - altitudeHistory_[ALTITUDE_HISTORY - 1].altitude;

                sumOfSpeeds += (distanceDelta * 1000/ timeDelta);
            }
        }
        else
        {
            if(altitudeHistory_[i].time > altitudeHistory_[i - 1].time)
            {
                //Time to calculate speed:
                int32_t timeDelta = altitudeHistory_[i].time
                        - altitudeHistory_[i - 1].time;
                int32_t distanceDelta = altitudeHistory_[i].altitude
                        - altitudeHistory_[i - 1].altitude;

                sumOfSpeeds += (distanceDelta * 1000/ timeDelta);
            }
        }
    }

    //Sanity check, if no altitudes for 20s, then return 0
    if(altitudeHistory_[maxIndex].time + 20000 < timeNow)
        return 0;

    /*

    //Time to calculate speed:
    int32_t timeDelta = altitudeHistory_[maxIndex].time
            - altitudeHistory_[minIndex].time;
    int32_t distanceDelta = altitudeHistory_[maxIndex].altitude
            - altitudeHistory_[minIndex].altitude;

    //Sanity check, timeDelta should be of only 15s
    if(timeDelta > ALTITUDE_HISTORY * 1500L)
        return 0;

    //timedelta is in ms, so we divide by 1000
    //distance is in cm, and remains like that
    return (distanceDelta / (timeDelta/1000);

    */

    return sumOfSpeeds / ((ALTITUDE_HISTORY - 1) );
}

/**
 * It returns the last recorded altitude in cm
 */
int32_t getAltitude()
{
    if(confRegister_.sim_enabled)
    {
        return confRegister_.sim_altitude * 100;
    }
    //REturn the last recorded altitude:
    uint8_t previousAltitudeIndex;
    if(altitudeHistoryIndex_ == 0)
        previousAltitudeIndex = ALTITUDE_HISTORY - 1;
    else
        previousAltitudeIndex = altitudeHistoryIndex_ - 1;

    return altitudeHistory_[previousAltitudeIndex].altitude;
}


/**
 * It returns 1 if the signal has been detected, or a 0 if not detected
 */
uint8_t getSunriseSignalActivated()
{
    uint32_t uptime = seconds_uptime();
    if(uptime < 15)
        return 0;   //If uptime is <15s, return always zero

    //Signal is low, it means that the signal has been actually sent!
    if(gpioSunriseLastStatus_ == 0)
        return 1;

    //Signal is high, so no signal yet detected
    return 0;
}


//uint32_t altDebug_ = 0;
float inaCurrentAverages_[2];
float inaVoltageAverages_[2];
float verticalSpeedAverages_[2];
float accXAverages_[2];
float accYAverages_[2];
float accZAverages_[2];

/**
 * Checks if it is time to read the sensors and saves them into memory
 */
void sensorsRead()
{
    uint64_t uptime = millis_uptime();

    //Read GPIOs only once per second (like baro)
    if(lastTime_gpioSunriseRead_ + confRegister_.baro_readPeriod < uptime)
    {
        uint8_t gpioStatus = sunrise_GPIO_Read();
        //Do not listen to the GPIO if less than 10s of uptime
        if(gpioStatus != gpioSunriseLastStatus_ && uptime > 10000)
        {
            uint8_t payload[5] = {0};
            payload[0] = gpioStatus;
            saveEventSimple(EVENT_SUNRISE_GPIO_CHANGE, payload);
            //TODO implement here something to detect the change!
        }

        if(gpioStatus)
        {
            currentTelemetryLine_[0].switches_status |= BIT6;
            currentTelemetryLine_[1].switches_status |= BIT6;
        }
        else
        {
            currentTelemetryLine_[0].switches_status &= ~BIT6;
            currentTelemetryLine_[1].switches_status &= ~BIT6;
        }

        gpioSunriseLastStatus_ = gpioStatus;
        lastTime_gpioSunriseRead_ = uptime;
    }

    //Add the rest of the thing of the switches:
    //CAM1
    if(P4OUT & BIT6)
    {
        currentTelemetryLine_[0].switches_status &= ~BIT0;
        currentTelemetryLine_[1].switches_status &= ~BIT0;
    }
    else
    {
        currentTelemetryLine_[0].switches_status |= BIT0;
        currentTelemetryLine_[1].switches_status |= BIT0;
    }

    //CAM2
    if(P4OUT & BIT5)
    {
        currentTelemetryLine_[0].switches_status &= ~BIT1;
        currentTelemetryLine_[1].switches_status &= ~BIT1;
    }
    else
    {
        currentTelemetryLine_[0].switches_status |= BIT1;
        currentTelemetryLine_[1].switches_status |= BIT1;

    }

    //CAM3
    if(P4OUT & BIT4)
    {
        currentTelemetryLine_[0].switches_status &= ~BIT2;
        currentTelemetryLine_[1].switches_status &= ~BIT2;
    }
    else
    {
        currentTelemetryLine_[0].switches_status |= BIT2;
        currentTelemetryLine_[1].switches_status |= BIT2;
    }

    //CAM4
    if(P2OUT & BIT7)
    {
        currentTelemetryLine_[0].switches_status &= ~BIT3;
        currentTelemetryLine_[1].switches_status &= ~BIT3;
    }
    else
    {
        currentTelemetryLine_[0].switches_status |= BIT3;
        currentTelemetryLine_[1].switches_status |= BIT3;
    }

    //MUX Power
    if(P2OUT & BIT3)
    {
        currentTelemetryLine_[0].switches_status &= ~BIT4;
        currentTelemetryLine_[1].switches_status &= ~BIT4;
    }
    else
    {
        currentTelemetryLine_[0].switches_status |= BIT4;
        currentTelemetryLine_[1].switches_status |= BIT4;
    }

    //MUX Status
    if(P2OUT & BIT4)
    {
        currentTelemetryLine_[0].switches_status &= ~BIT5;
        currentTelemetryLine_[1].switches_status &= ~BIT5;
    }
    else
    {
        currentTelemetryLine_[0].switches_status |= BIT5;
        currentTelemetryLine_[1].switches_status |= BIT5;
    }

    //Acc interrupt
    if(P3OUT & BIT7)
    {
        currentTelemetryLine_[0].switches_status |= BIT7;
        currentTelemetryLine_[1].switches_status |= BIT7;
    }
    else
    {
        currentTelemetryLine_[0].switches_status &= ~BIT7;
        currentTelemetryLine_[1].switches_status &= ~BIT7;
    }

    currentTelemetryLine_[0].state = confRegister_.flightState;
    currentTelemetryLine_[1].state = confRegister_.flightState;

    currentTelemetryLine_[0].sub_state = confRegister_.flightSubState;
    currentTelemetryLine_[1].sub_state = confRegister_.flightSubState;

    //Time to read barometer?
    if(lastTime_baroRead_ + confRegister_.baro_readPeriod < uptime)
    {
        //Time to read barometer
        int32_t pressure;
        int32_t altitude;
        int32_t speed;
        int32_t temperature;
        int8_t error = i2c_MS5611_getPressure(&pressure, &temperature);
        if (error != 0)
        {
            //Try to reboot the i2c!!!
            uart_print(UART_DEBUG, "\r\nERROR: I2C seems to be not responding, rebooting the I2C...\r\n");
            i2c_master_init();
            //Init Barometer
            i2c_MS5611_init();
            //Register that an error ocurred today
            uint8_t payload[5] = {0};
            saveEventSimple(EVENT_I2C_ERROR_RESET, payload);

            //Reset time to read baro
            lastTime_baroRead_ = uptime;
            return;
        }

        //Convert baro to altitude
        altitude = calculateAltitude(pressure);
        //currentTelemetryLine_[0].temperatures[1] = temperature/10; //Baro
        //currentTelemetryLine_[1].temperatures[1] = temperature/10; //Baro

        //altitude = altDebug_;
        //altDebug_ = altDebug_ + 210;

        //add altitude to altitude history
        altitudeHistory_[altitudeHistoryIndex_].time = (int32_t)uptime;
        altitudeHistory_[altitudeHistoryIndex_].altitude = altitude;
        altitudeHistoryIndex_++;
        if(altitudeHistoryIndex_ >= ALTITUDE_HISTORY)
            altitudeHistoryIndex_ = 0;

        //Calculate speed
        speed = getVerticalSpeed();

        uint8_t i;
        for (i = 0; i < 2; i++)
        {
            currentTelemetryLine_[i].pressure = pressure;
            currentTelemetryLine_[i].altitude = altitude;
            //currentTelemetryLine_[i]. = altitude;

            // Save vertical velocity
            if(numTimes_baroRead_[i] == 0)
            {
                currentTelemetryLine_[i].verticalSpeed[AVG_INDEX] = 0;
                currentTelemetryLine_[i].verticalSpeed[MAX_INDEX] = -32767; //int16 minimum value
                currentTelemetryLine_[i].verticalSpeed[MIN_INDEX] = 32767; //int16 maximum value
                verticalSpeedAverages_[i] = 0.0;
            }
            else
            {
                //media = media + (newValue-media)/numberOfValues

                //Wrong way of doing it because numTime_inaRead becomes very high
                //int16_t newAverageVerticalSpeed = (speed -
                //        currentTelemetryLine_[i].verticalSpeed[AVG_INDEX])/numTimes_baroRead_[i];
                //currentTelemetryLine_[i].verticalSpeed[AVG_INDEX] += newAverageVerticalSpeed;

                float newAverageVerticalSpeed = ((float)speed - verticalSpeedAverages_[i]) / (float)numTimes_baroRead_[i];
                verticalSpeedAverages_[i] += newAverageVerticalSpeed;
                currentTelemetryLine_[i].verticalSpeed[AVG_INDEX] = verticalSpeedAverages_[i];

                if (speed < currentTelemetryLine_[i].verticalSpeed[MIN_INDEX])
                    currentTelemetryLine_[i].verticalSpeed[MIN_INDEX] = speed;
                if (speed > currentTelemetryLine_[i].verticalSpeed[MAX_INDEX])
                    currentTelemetryLine_[i].verticalSpeed[MAX_INDEX] = speed;
            }
            numTimes_baroRead_[i]++;
        }

        lastTime_baroRead_ = uptime;
    }

    //Time to read temperatures?
    if(lastTime_tempRead_ + confRegister_.temp_readPeriod < uptime)
    {
        int16_t temperatures[6];
        int8_t error = i2c_TMP75_getTemperatures(temperatures);

        uint8_t i;
        for (i = 0; i < 2; i++)
        {
            // Save temperature
            currentTelemetryLine_[i].temperatures[0] = temperatures[0]; //PCB
            currentTelemetryLine_[i].temperatures[1] = temperatures[1]; //External 01
            currentTelemetryLine_[i].temperatures[2] = temperatures[2]; //External 02
            //currentTelemetryLine_[i].temperatures[3] = temperatures[3]; //External 02
            //currentTelemetryLineFRAMandNOR_[i].temperatures[4] = temperatures[4]; //External 03
        }

        lastTime_tempRead_ = uptime;
    }

    //Time to read voltages and currents?
    if(lastTime_inaRead_ + confRegister_.ina_readPeriod < uptime)
    {
        struct INAData inaData;
        int8_t error = i2c_INA_read(&inaData);

        if(inaData.current < 0)
        {
            //Negative current means that power supply has been disconnected
            uint8_t payload[5] = {0};
            memcpy(&payload[0], &inaData.current, sizeof(inaData.current));
            memcpy(&payload[2], &inaData.voltage, sizeof(inaData.voltage));
            saveEventSimple(EVENT_BATTERY_CUTOUT, payload);
        }

        uint8_t i;
        for (i = 0; i < 2; i++)
        {
            // Save voltages
            if (numTimes_inaRead_[i] == 0)
            {
                currentTelemetryLine_[i].voltage[AVG_INDEX] = 0;
                currentTelemetryLine_[i].voltage[MAX_INDEX] = -32767;
                currentTelemetryLine_[i].voltage[MIN_INDEX] = 32767;

                currentTelemetryLine_[i].current[AVG_INDEX] = 0;
                currentTelemetryLine_[i].current[MAX_INDEX] = -32767;
                currentTelemetryLine_[i].current[MIN_INDEX] = 32767;
                inaCurrentAverages_[i] = 0.0;
                inaVoltageAverages_[i] = 0.0;
            }
            else
            {
                //media = media + (newValue-media)/numberOfValues

                //Wrong way of doing it because numTime_inaRead becomes very high
                //int16_t newAverageVoltage = (inaData.voltage -
                //        currentTelemetryLine_[i].voltage[AVG_INDEX])/numTimes_inaRead_[i];
                //currentTelemetryLine_[i].voltage[AVG_INDEX] += newAverageVoltage;

                //We can only do it with floats accurately
                float newAverageVoltage = ((float)inaData.voltage - inaVoltageAverages_[i])
                                        / (float)numTimes_inaRead_[i];
                inaVoltageAverages_[i] += newAverageVoltage;
                currentTelemetryLine_[i].voltage[AVG_INDEX] = inaVoltageAverages_[i];

                if (inaData.voltage < currentTelemetryLine_[i].voltage[MIN_INDEX])
                    currentTelemetryLine_[i].voltage[MIN_INDEX] = inaData.voltage;
                if (inaData.voltage > currentTelemetryLine_[i].voltage[MAX_INDEX])
                    currentTelemetryLine_[i].voltage[MAX_INDEX] = inaData.voltage;

                float newAverageCurrent = ((float)inaData.current -
                        inaCurrentAverages_[i])/(float)numTimes_inaRead_[i];
                inaCurrentAverages_[i] += newAverageCurrent;
                currentTelemetryLine_[i].current[AVG_INDEX] = inaCurrentAverages_[i];

                if (inaData.current < currentTelemetryLine_[i].current[MIN_INDEX])
                    currentTelemetryLine_[i].current[MIN_INDEX] = inaData.current;
                if (inaData.current > currentTelemetryLine_[i].current[MAX_INDEX])
                    currentTelemetryLine_[i].current[MAX_INDEX] = inaData.current;
            }

            numTimes_inaRead_[i]++;
        }

        lastTime_inaRead_ = uptime;
    }

    //Time to read accelerations?
    if(lastTime_accRead_ + confRegister_.acc_readPeriod < uptime)
    {
        struct ACCData accData;
        int8_t error = i2c_ADXL345_getAccelerations(&accData);

        uint8_t i;
        for (i = 0; i < 2; i++)
        {
            // Save accs
            if (numTimes_accRead_[i] == 0)
            {
                currentTelemetryLine_[i].accXAxis[AVG_INDEX] = 0;
                currentTelemetryLine_[i].accXAxis[MAX_INDEX] = -32767;
                currentTelemetryLine_[i].accXAxis[MIN_INDEX] = 32767;

                currentTelemetryLine_[i].accYAxis[AVG_INDEX] = 0;
                currentTelemetryLine_[i].accYAxis[MAX_INDEX] = -32767;
                currentTelemetryLine_[i].accYAxis[MIN_INDEX] = 32767;

                currentTelemetryLine_[i].accZAxis[AVG_INDEX] = 0;
                currentTelemetryLine_[i].accZAxis[MAX_INDEX] = -32767;
                currentTelemetryLine_[i].accZAxis[MIN_INDEX] = 32767;

                accXAverages_[i] = 0.0;
                accYAverages_[i] = 0.0;
                accZAverages_[i] = 0.0;
            }
            else
            {
                //media = media + (newValue-media)/numberOfValues

                float newAverageAcc = ((float)accData.x - accXAverages_[i])
                                        / (float)numTimes_accRead_[i];
                accXAverages_[i] += newAverageAcc;
                currentTelemetryLine_[i].accXAxis[AVG_INDEX] = accXAverages_[i];

                if (accData.x < currentTelemetryLine_[i].accXAxis[MIN_INDEX])
                    currentTelemetryLine_[i].accXAxis[MIN_INDEX] = accData.x;
                if (accData.x > currentTelemetryLine_[i].accXAxis[MAX_INDEX])
                    currentTelemetryLine_[i].accXAxis[MAX_INDEX] = accData.x;

                //Y Axys

                newAverageAcc = ((float)accData.y - accYAverages_[i])
                                        / (float)numTimes_accRead_[i];
                accYAverages_[i] += newAverageAcc;
                currentTelemetryLine_[i].accYAxis[AVG_INDEX] = accYAverages_[i];

                if (accData.y < currentTelemetryLine_[i].accYAxis[MIN_INDEX])
                    currentTelemetryLine_[i].accYAxis[MIN_INDEX] = accData.y;
                if (accData.y > currentTelemetryLine_[i].accYAxis[MAX_INDEX])
                    currentTelemetryLine_[i].accYAxis[MAX_INDEX] = accData.y;

                //Z Axys

                newAverageAcc = ((float)accData.z - accZAverages_[i])
                                        / (float)numTimes_accRead_[i];
                accZAverages_[i] += newAverageAcc;
                currentTelemetryLine_[i].accZAxis[AVG_INDEX] = accZAverages_[i];

                if (accData.z < currentTelemetryLine_[i].accZAxis[MIN_INDEX])
                    currentTelemetryLine_[i].accZAxis[MIN_INDEX] = accData.z;
                if (accData.z > currentTelemetryLine_[i].accZAxis[MAX_INDEX])
                    currentTelemetryLine_[i].accZAxis[MAX_INDEX] = accData.z;

            }

            numTimes_accRead_[i]++;
        }
        lastTime_accRead_ = uptime;
    }


}

int8_t returnCurrentTMLines(struct TelemetryLine tmLines[2])
{
    tmLines[0] = currentTelemetryLine_[0];
    tmLines[1] = currentTelemetryLine_[1];
    return 0;
}

struct EventLine lastEvent = {0};

/**
 * It saves an Event Line on the FRAM and NOR memories.
 */
int8_t saveEvent(struct EventLine newEvent)
{
    if(lastEvent.event == newEvent.event
            && lastEvent.unixTime + 5 > newEvent.unixTime)
    {
        //Ignore repetitive event if it is in less than 5s
        return 0;
    }

    //Copy last event
    lastEvent = newEvent;

    //First save on the FRAM which is much faster:
    addEventFRAM(newEvent, &confRegister_.fram_eventAddress);

    //Now save on the NOR that it is a little bit slower
    int8_t error = addEventNOR(newEvent, &confRegister_.nor_eventAddress);

    if(error)
        uart_print(UART_DEBUG, "ERROR: NOR memory was busy, we could not write events in.\r\n");

    return 0;
}

/**
 * It adds an event in the simplest possible way
 */
int8_t saveEventSimple(uint8_t code, uint8_t payload[5])
{
    //Register that a boot happened just now:
    struct EventLine newEvent = {0};
    newEvent.upTime = (uint32_t) millis_uptime();
    newEvent.unixTime = i2c_RTC_unixTime_now();
    newEvent.state = confRegister_.flightState;
    newEvent.sub_state = confRegister_.flightSubState;
    newEvent.event = code;
    newEvent.payload[0] = payload[0];
    newEvent.payload[1] = payload[1];
    newEvent.payload[2] = payload[2];
    newEvent.payload[3] = payload[3];
    newEvent.payload[4] = payload[4];
    return saveEvent(newEvent);
}

uint32_t lastTimeTelemetrySavedNOR_ = 0;
uint32_t lastTimeTelemetrySavedFRAM_ = 0;

/**
 * It saves a Telemetry Line on the FRAM and on the NOR memories.
 */
int8_t saveTelemetry()
{
    uint64_t uptime = millis_uptime();
    uint32_t elapsedSeconds = seconds_uptime();
    uint32_t unixtTimeNow = i2c_RTC_unixTime_now();

    //First save on the FRAM which is much faster
    //Save it only once every x seconds, otherwise we fill it!
    if(lastTimeTelemetrySavedFRAM_ + confRegister_.fram_tlmSavePeriod < elapsedSeconds)
    {
        currentTelemetryLine_[0].unixTime = unixtTimeNow;
        currentTelemetryLine_[0].upTime = uptime;
        currentTelemetryLine_[0].state = confRegister_.flightState;
        currentTelemetryLine_[0].sub_state = confRegister_.flightSubState;
        //Time to save
        addTelemetryFRAM(currentTelemetryLine_[0], &confRegister_.fram_telemetryAddress);
        lastTimeTelemetrySavedFRAM_ = elapsedSeconds;

        //Reset Telemetry Line
        struct TelemetryLine newVoidTMLine = {0};
        currentTelemetryLine_[0] = newVoidTMLine;
        numTimes_baroRead_[0] = 0;
        numTimes_inaRead_[0] = 0;
        numTimes_accRead_[0] = 0;
    }

    //Now save on the NOR which is a little bit slower
    //if(1) //Trick to save a lot of them altoghether
    if(lastTimeTelemetrySavedNOR_ + confRegister_.nor_tlmSavePeriod < elapsedSeconds)
    {
        currentTelemetryLine_[1].unixTime = unixtTimeNow;
        currentTelemetryLine_[1].upTime = uptime;
        currentTelemetryLine_[1].state = confRegister_.flightState;
        currentTelemetryLine_[1].sub_state = confRegister_.flightSubState;

        //Time to save
        int8_t error = addTelemetryNOR(&currentTelemetryLine_[1], &confRegister_.nor_telemetryAddress);
        if(error)
            uart_print(UART_DEBUG, "ERROR: NOR memory was busy, we could not write telemetry in.");

        lastTimeTelemetrySavedNOR_ = elapsedSeconds;

        //Reset Telemetry Line
        struct TelemetryLine newVoidTMLine = {0};
        currentTelemetryLine_[1] = newVoidTMLine;
        numTimes_baroRead_[1] = 0;
        numTimes_inaRead_[1] = 0;
        numTimes_accRead_[1] = 0;
    }

    return 0;
}

/**
 * It saves a new event in the FRAM memory
 */
int8_t addEventFRAM(struct EventLine newEvent, uint32_t *address)
{
    //Avoid saving the timelapse pictures because of lack of space
    if(newEvent.event == EVENT_CAMERA_TIMELAPSE_PIC)
        return 0;

    //Sanity check:
    if(*address < FRAM_EVENTS_ADDRESS ||
            (*address + sizeof(newEvent)) > (FRAM_EVENTS_ADDRESS + FRAM_EVENTS_SIZE))
        //memory overflow, protect exiting from here!
        return -1;

    volatile uint8_t *framPointerWrite;
    volatile uint8_t *ramPointerRead;

    framPointerWrite = (uint8_t *)*address;
    ramPointerRead = (uint8_t *)&newEvent;

    *address += sizeof(newEvent);

    //Enable write on FRAM
    MPUCTL0 = MPUPW;

    uint8_t i;
    //Copy each byte to the FRAM
    for(i = 0; i < sizeof(newEvent); i++)
    {
        *framPointerWrite = *ramPointerRead;
        framPointerWrite++;
        ramPointerRead++;
    }
    //Put back protection flags to FRAM code area
    MPUCTL0 = MPUPW | MPUENA;

    return 0;
}

/**
 * It returns a stored event line from the FRAM, the pointer is the event
 * number, being 0 the first event and 255 the last one.
 */
int8_t getEventFRAM(uint16_t pointer, struct EventLine *savedEvent)
{
    uint32_t address = FRAM_EVENTS_ADDRESS + pointer * sizeof(struct EventLine);
    volatile uint8_t *framPointerRead;
    volatile uint8_t *ramPointerWrite;

    framPointerRead = (uint8_t *)address;
    ramPointerWrite = (uint8_t *)savedEvent;

    uint8_t i;
   //Copy each byte to the FRAM
   for(i = 0; i < sizeof(struct EventLine); i++)
   {
       *ramPointerWrite = *framPointerRead;
       framPointerRead++;
       ramPointerWrite++;
   }

   return 0;
}

/**
 * It saves the telemetry struct in the selected address in the FRAM.
 */
int8_t addTelemetryFRAM(struct TelemetryLine newTelemetry, uint32_t *address)
{
    //Sanity check:
    if(*address < FRAM_TLM_ADDRESS ||
            (*address + (uint32_t)sizeof(newTelemetry)) > (FRAM_TLM_ADDRESS + FRAM_TLM_SIZE))
        //memory overflow, protect exiting from here!
        return -1;

    volatile uint8_t *framPointerWrite;
    volatile uint8_t *ramPointerRead;

    framPointerWrite = (uint8_t *)*address;
    ramPointerRead = (uint8_t *)&newTelemetry;

    *address += sizeof(newTelemetry);

    //Enable write on FRAM
    MPUCTL0 = MPUPW;

    uint8_t i;
    //Copy each byte to the FRAM
    for(i = 0; i < sizeof(newTelemetry); i++)
    {
        *framPointerWrite = *ramPointerRead;
        framPointerWrite++;
        ramPointerRead++;
    }
    //Put back protection flags to FRAM code area
    MPUCTL0 = MPUPW | MPUENA;

    return 0;
}

/**
 * It returns a stored telemetry line from the FRAM, the pointer is the telemetry
 * number, being 0 the first telemetry and 1600 the last one.
 */
int8_t getTelemetryFRAM(uint16_t pointer, struct TelemetryLine *savedTelemetry)
{

    uint32_t address = FRAM_TLM_ADDRESS + (uint32_t)pointer * (uint32_t)sizeof(struct TelemetryLine);

    volatile uint8_t *framPointerRead;
    volatile uint8_t *ramPointerWrite;

    framPointerRead = (uint8_t *)address;
    ramPointerWrite = (uint8_t *)savedTelemetry;

    uint8_t i;
   //Copy each byte to the FRAM
   for(i = 0; i < sizeof(struct TelemetryLine); i++)
   {
       *ramPointerWrite = *framPointerRead;
       framPointerRead++;
       ramPointerWrite++;
   }

   return 0;
}

/**
 * It saves a new Event Line in the NOR memory.
 */
int8_t addEventNOR(struct EventLine newEvent, uint32_t *address)
{
    //Sanity check:
    if(*address < NOR_EVENTS_ADDRESS)
        return -5;

    int8_t error = spi_NOR_writeToAddress(*address,
                                          (uint8_t *) &newEvent,
                                          sizeof(struct EventLine),
                                          confRegister_.nor_deviceSelected);
    //Give time to the NOR memory to become available again (takes 1 to 2 ms),
    //this solves many problems
    sleep_ms(2);

    if(error == 0)  //Advance only if no error was detected
        *address += sizeof(struct EventLine);

    return error;
}

/**
 * It returns a stored Event Line from the NOR, the pointer is the EV line num.
 */
int8_t getEventNOR(uint32_t pointer, struct EventLine *savedEvent)
{
    uint32_t address = NOR_EVENTS_ADDRESS + pointer * sizeof(struct EventLine);
    int8_t error = spi_NOR_readFromAddress(address,
                                           (uint8_t *) savedEvent,
                                           sizeof(struct EventLine),
                                           confRegister_.nor_deviceSelected);

    return error;
}

/**
 * It saves a new Telemetry Line in the NOR memory.
 */
int8_t addTelemetryNOR(struct TelemetryLine *newTelemetry, uint32_t *address)
{
    //Sanity check:
    if(*address >= NOR_EVENTS_ADDRESS)
        return -1;

    int8_t error = spi_NOR_writeToAddress(*address,
                                          (uint8_t *) newTelemetry,
                                          sizeof(struct TelemetryLine),
                                          confRegister_.nor_deviceSelected);
    //Give time to the NOR memory to become available again (takes 1 to 2 ms),
    //this solves many problems
    sleep_ms(2);

    if(error == 0)  //Advance only if no error was detected
        *address += sizeof(struct TelemetryLine);

    return error;
}

/**
 * It returns a stored Telemetry Line from the NOR, the pointer is the TM
 * line num.
 */
int8_t getTelemetryNOR(uint32_t pointer,
                       struct TelemetryLine *savedTelemetry)
{
    uint32_t address = NOR_TLM_ADDRESS + pointer * sizeof(struct TelemetryLine);
    int8_t error = spi_NOR_readFromAddress(address,
                                           (uint8_t *) savedTelemetry,
                                           sizeof(struct TelemetryLine),
                                           confRegister_.nor_deviceSelected);

    return error;
}

/**
 * It prints on UART_DEBUG the history of altitudes and calculated vertical
 * speed
 */
void printAltitudeHistory()
{
    char strToPrint[200];
    uart_print(UART_DEBUG, "Altitude History:\r\n");
    uint8_t i;
    for(i = 0; i < ALTITUDE_HISTORY; i++)
    {
        sprintf(strToPrint, "  [%d]:\t%.3fs\t %.2fm\r\n",
                i,
                (float)altitudeHistory_[i].time / 1000.0,
                (float)altitudeHistory_[i].altitude / 100.0);
        uart_print(UART_DEBUG, strToPrint);
    }

    int32_t speed = getVerticalSpeed();
    sprintf(strToPrint, "Current Speed:   %.3fm/s\r\n", (float)speed / 100.0);
    uart_print(UART_DEBUG, strToPrint);
}

