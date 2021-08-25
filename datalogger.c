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

// How many times has a sensor been read per Telemetry Line? --> to compute avgs
uint16_t numTimes_baroRead_[2] = {0, 0};   // FRAM, NOR
uint16_t numTimes_inaRead_[2] = {0, 0};
uint16_t numTimes_accRead_[2] = {0, 0};

//Current Telemetry Line. First index corresponds to FRAM one, second index to NOR.
struct TelemetryLine currentTelemetryLineFRAMandNOR_[2];

//History of altitudes
uint32_t lastAltitude_ = 0;

struct AltitudesHistory
{
    uint64_t time;
    uint32_t altitude;
};

const uint8_t maxHistSave_ = 10;
struct AltitudesHistory altitudeHistory_[10]; // Number must be equal to maxHistSave_
uint8_t altitudeHistoryIndex_ = 0;

// PRIVATE FUNCTIONS
void addTimeAndStateMark();

/**
 * When a sensor is read and its data put into a TM line, this function
 * updates the value of the uptime, unixtime, flight state and substate
 * of the TM line.
 */
void addTimeAndStateMark()
{
    uint32_t unixtTimeNow = i2c_RTC_unixTime_now();
    uint32_t uptime = millis_uptime();

    currentTelemetryLineFRAMandNOR_[0].unixTime = unixtTimeNow;
    currentTelemetryLineFRAMandNOR_[0].upTime = uptime;
    currentTelemetryLineFRAMandNOR_[0].state = confRegister_.flightState;
    currentTelemetryLineFRAMandNOR_[0].sub_state = confRegister_.flightSubState;

    currentTelemetryLineFRAMandNOR_[1].unixTime = unixtTimeNow;
    currentTelemetryLineFRAMandNOR_[1].upTime = uptime;
    currentTelemetryLineFRAMandNOR_[1].state = confRegister_.flightState;
    currentTelemetryLineFRAMandNOR_[1].sub_state = confRegister_.flightSubState;
}

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
 * Checks if it is time to read the sensors and saves them into memory
 */
void sensors_read()
{
    uint64_t uptime = millis_uptime();

    //We use nested if-else in order to avoid using the I2C twice at the same time

    //Time to read barometer?
    if(lastTime_baroRead_ + confRegister_.baro_readPeriod < uptime)
    {
        //Time to read barometer
        addTimeAndStateMark();

        int32_t pressure;
        int32_t altitude;
        int8_t error = i2c_MS5611_getPressure(&pressure);
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
            return;
        }

        altitude = calculateAltitude(pressure);

        //Save historic altitude data
        if (altitudeHistoryIndex_ < maxHistSave_)
        {
            altitudeHistory_[altitudeHistoryIndex_].time = uptime;
            altitudeHistory_[altitudeHistoryIndex_].altitude = altitude;
            altitudeHistoryIndex_++;
        }
        else
        {
            uint8_t i;
            for (i = 0; i < maxHistSave_-1; i++)
            {
                altitudeHistory_[i] = altitudeHistory_[i+1];
            }
            altitudeHistory_[maxHistSave_-1].time = uptime;
            altitudeHistory_[maxHistSave_-1].altitude = altitude;
        }

        uint8_t i;
        for (i = 0; i < 2; i++)
        {
            // Save pressure
            currentTelemetryLineFRAMandNOR_[i].pressure = pressure;

            // Save altitude
            currentTelemetryLineFRAMandNOR_[i].altitude = altitude;

            // Save vertical velocity
            if (i == 0 && numTimes_baroRead_[0] == 0)
            {
                currentTelemetryLineFRAMandNOR_[0].verticalSpeed[AVG_INDEX] = 0;
                currentTelemetryLineFRAMandNOR_[0].verticalSpeed[MAX_INDEX] = -32767; //int16 minimum value
                currentTelemetryLineFRAMandNOR_[0].verticalSpeed[MIN_INDEX] = 32767; //int16 maximum value
            }
            else if (i == 1 && numTimes_baroRead_[1] == 0)
            {
                currentTelemetryLineFRAMandNOR_[1].verticalSpeed[AVG_INDEX] = 0;
                currentTelemetryLineFRAMandNOR_[1].verticalSpeed[MAX_INDEX] = -32767; //int16 minimum value
                currentTelemetryLineFRAMandNOR_[1].verticalSpeed[MIN_INDEX] = 32767; //int16 maximum value
            }
            else
            {
                uint32_t currentVerticalSpeed = (altitude-lastAltitude_)/2;
                uint32_t newAverageVerticalSpeed = (currentVerticalSpeed -
                        currentTelemetryLineFRAMandNOR_[i].verticalSpeed[AVG_INDEX])/numTimes_baroRead_[i];

                currentTelemetryLineFRAMandNOR_[i].verticalSpeed[AVG_INDEX] += newAverageVerticalSpeed;
                if (currentVerticalSpeed < currentTelemetryLineFRAMandNOR_[i].verticalSpeed[MIN_INDEX])
                    currentTelemetryLineFRAMandNOR_[i].verticalSpeed[MIN_INDEX] = currentVerticalSpeed;
                if (currentVerticalSpeed > currentTelemetryLineFRAMandNOR_[i].verticalSpeed[MAX_INDEX])
                    currentTelemetryLineFRAMandNOR_[i].verticalSpeed[MAX_INDEX] = currentVerticalSpeed;
            }

            lastAltitude_ = altitude;
            numTimes_baroRead_[i]++;
        }

        lastTime_baroRead_ = uptime;
    }
    //Time to read temperatures?
    else if(lastTime_tempRead_ + confRegister_.temp_readPeriod < uptime)
    {
        //Time to read thermometer
        addTimeAndStateMark();

        int16_t temperatures[6];
        i2c_TMP75_getTemperatures(temperatures);

        uint8_t i;
        for (i = 0; i < 2; i++)
        {
            // Save temperature
            currentTelemetryLineFRAMandNOR_[i].temperatures[0] = temperatures[0]; //PCB
            currentTelemetryLineFRAMandNOR_[i].temperatures[1] = temperatures[1]; //Baro
            currentTelemetryLineFRAMandNOR_[i].temperatures[2] = temperatures[2]; //External 01
            //currentTelemetryLineFRAMandNOR_[i].temperatures[3] = temperatures[3]; //External 02
            //currentTelemetryLineFRAMandNOR_[i].temperatures[4] = temperatures[4]; //External 03
        }

        lastTime_tempRead_ = uptime;
    }
    //Time to read voltages and currents?
    else if(lastTime_inaRead_ + confRegister_.ina_readPeriod < uptime)
    {
        //Time to read voltages and currents
        addTimeAndStateMark();

        struct INAData inaData;
        i2c_INA_read(&inaData);

        uint8_t i;
        for (i = 0; i < 2; i++)
        {
            // Save voltages
            if (i == 0 && numTimes_inaRead_[0] == 0)
            {
                currentTelemetryLineFRAMandNOR_[0].voltages[AVG_INDEX] = inaData.voltage;
                currentTelemetryLineFRAMandNOR_[0].voltages[MAX_INDEX] = inaData.voltage;
                currentTelemetryLineFRAMandNOR_[0].voltages[MIN_INDEX] = inaData.voltage;

                currentTelemetryLineFRAMandNOR_[0].currents[AVG_INDEX] = inaData.current;
                currentTelemetryLineFRAMandNOR_[0].currents[MAX_INDEX] = inaData.current;
                currentTelemetryLineFRAMandNOR_[0].currents[MIN_INDEX] = inaData.current;
            }
            else if (i == 1 && numTimes_inaRead_[1] == 0)
            {
                currentTelemetryLineFRAMandNOR_[1].voltages[AVG_INDEX] = inaData.voltage;
                currentTelemetryLineFRAMandNOR_[1].voltages[MAX_INDEX] = inaData.voltage;
                currentTelemetryLineFRAMandNOR_[1].voltages[MIN_INDEX] = inaData.voltage;

                currentTelemetryLineFRAMandNOR_[0].currents[AVG_INDEX] = inaData.current;
                currentTelemetryLineFRAMandNOR_[0].currents[MAX_INDEX] = inaData.current;
                currentTelemetryLineFRAMandNOR_[0].currents[MIN_INDEX] = inaData.current;
            }
            else
            {
                uint32_t newAverageVoltage = (inaData.voltage -
                        currentTelemetryLineFRAMandNOR_[i].voltages[AVG_INDEX])/numTimes_inaRead_[i];

                currentTelemetryLineFRAMandNOR_[i].voltages[AVG_INDEX] += newAverageVoltage;
                if (inaData.voltage < currentTelemetryLineFRAMandNOR_[i].voltages[MIN_INDEX])
                    currentTelemetryLineFRAMandNOR_[i].voltages[MIN_INDEX] = inaData.voltage;
                if (inaData.voltage > currentTelemetryLineFRAMandNOR_[i].voltages[MAX_INDEX])
                    currentTelemetryLineFRAMandNOR_[i].voltages[MAX_INDEX] = inaData.voltage;


                uint32_t newAverageCurrent = (inaData.current -
                        currentTelemetryLineFRAMandNOR_[i].currents[AVG_INDEX])/numTimes_inaRead_[i];

                currentTelemetryLineFRAMandNOR_[i].currents[AVG_INDEX] += newAverageCurrent;
                if (inaData.current < currentTelemetryLineFRAMandNOR_[i].currents[MIN_INDEX])
                    currentTelemetryLineFRAMandNOR_[i].currents[MIN_INDEX] = inaData.current;
                if (inaData.current > currentTelemetryLineFRAMandNOR_[i].currents[MAX_INDEX])
                    currentTelemetryLineFRAMandNOR_[i].currents[MAX_INDEX] = inaData.current;
            }

            numTimes_inaRead_[i]++;
        }

        lastTime_inaRead_ = uptime;
    }
    //Time to read accelerations?
    else if(lastTime_accRead_ + confRegister_.acc_readPeriod < uptime)
    {
        // COMMENTED WHILE ACCELEROMETER NOT PRESENT ON PCB

        /*
        //Time to read accelerations
        addTimeAndStateMark();

        struct Accelerations accData;
        i2c_ADXL345_getAccelerations(&accData);

        uint8_t i;
        for (i = 0; i < 2; i++)
        {
            // Save accelerations
            if (i == 0 && numTimes_accRead_[0] == 0)
            {
                currentTelemetryLineFRAMandNOR_[0].accXAxis[AVG_INDEX] = accData.XAxis;
                currentTelemetryLineFRAMandNOR_[0].accXAxis[MAX_INDEX] = accData.XAxis;
                currentTelemetryLineFRAMandNOR_[0].accXAxis[MIN_INDEX] = accData.XAxis;

                currentTelemetryLineFRAMandNOR_[0].accYAxis[AVG_INDEX] = accData.YAxis;
                currentTelemetryLineFRAMandNOR_[0].accYAxis[MAX_INDEX] = accData.YAxis;
                currentTelemetryLineFRAMandNOR_[0].accYAxis[MIN_INDEX] = accData.YAxis;

                currentTelemetryLineFRAMandNOR_[0].accZAxis[AVG_INDEX] = accData.ZAxis;
                currentTelemetryLineFRAMandNOR_[0].accZAxis[MAX_INDEX] = accData.ZAxis;
                currentTelemetryLineFRAMandNOR_[0].accZAxis[MIN_INDEX] = accData.ZAxis;
            }
            else if (i == 1 && numTimes_accRead_[1] == 0)
            {
                currentTelemetryLineFRAMandNOR_[1].accXAxis[AVG_INDEX] = accData.XAxis;
                currentTelemetryLineFRAMandNOR_[1].accXAxis[MAX_INDEX] = accData.XAxis;
                currentTelemetryLineFRAMandNOR_[1].accXAxis[MIN_INDEX] = accData.XAxis;

                currentTelemetryLineFRAMandNOR_[1].accYAxis[AVG_INDEX] = accData.YAxis;
                currentTelemetryLineFRAMandNOR_[1].accYAxis[MAX_INDEX] = accData.YAxis;
                currentTelemetryLineFRAMandNOR_[1].accYAxis[MIN_INDEX] = accData.YAxis;

                currentTelemetryLineFRAMandNOR_[1].accZAxis[AVG_INDEX] = accData.ZAxis;
                currentTelemetryLineFRAMandNOR_[1].accZAxis[MAX_INDEX] = accData.ZAxis;
                currentTelemetryLineFRAMandNOR_[1].accZAxis[MIN_INDEX] = accData.ZAxis;
            }
            else
            {
                uint32_t newAverageAccXAxis = (accData.XAxis -
                        currentTelemetryLineFRAMandNOR_[i].accXAxis[AVG_INDEX])/numTimes_accRead_[i];

                currentTelemetryLineFRAMandNOR_[i].accXAxis[AVG_INDEX] += newAverageAccXAxis;
                if (accData.XAxis < currentTelemetryLineFRAMandNOR_[i].accXAxis[MIN_INDEX])
                    currentTelemetryLineFRAMandNOR_[i].accXAxis[MIN_INDEX] = accData.XAxis;
                if (accData.XAxis > currentTelemetryLineFRAMandNOR_[i].accXAxis[MAX_INDEX])
                    currentTelemetryLineFRAMandNOR_[i].accXAxis[MAX_INDEX] = accData.XAxis;

                uint32_t newAverageAccYAxis = (accData.YAxis -
                        currentTelemetryLineFRAMandNOR_[i].accYAxis[AVG_INDEX])/numTimes_accRead_[i];

                currentTelemetryLineFRAMandNOR_[i].accYAxis[AVG_INDEX] += newAverageAccYAxis;
                if (accData.YAxis < currentTelemetryLineFRAMandNOR_[i].accYAxis[MIN_INDEX])
                    currentTelemetryLineFRAMandNOR_[i].accYAxis[MIN_INDEX] = accData.YAxis;
                if (accData.YAxis > currentTelemetryLineFRAMandNOR_[i].accYAxis[MAX_INDEX])
                    currentTelemetryLineFRAMandNOR_[i].accYAxis[MAX_INDEX] = accData.YAxis;

                uint32_t newAverageAccZAxis = (accData.ZAxis -
                        currentTelemetryLineFRAMandNOR_[i].accZAxis[AVG_INDEX])/numTimes_accRead_[i];

                currentTelemetryLineFRAMandNOR_[i].accZAxis[AVG_INDEX] += newAverageAccZAxis;
                if (accData.ZAxis < currentTelemetryLineFRAMandNOR_[i].accZAxis[MIN_INDEX])
                    currentTelemetryLineFRAMandNOR_[i].accZAxis[MIN_INDEX] = accData.ZAxis;
                if (accData.ZAxis > currentTelemetryLineFRAMandNOR_[i].accZAxis[MAX_INDEX])
                    currentTelemetryLineFRAMandNOR_[i].accZAxis[MAX_INDEX] = accData.ZAxis;
            }

            numTimes_accRead_[i]++;
        }

        lastTime_accRead_ = uptime;
        */
    }
}

int8_t returnCurrentTMLines(struct TelemetryLine tmLines[2])
{
    tmLines[0] = currentTelemetryLineFRAMandNOR_[0];
    tmLines[1] = currentTelemetryLineFRAMandNOR_[1];
    return 0;
}

/**
 * It saves an Event Line on the FRAM and NOR memories.
 */
int8_t saveEvent(struct EventLine newEvent)
{
    //First save on the FRAM which is much faster:
    addEventFRAM(newEvent, &confRegister_.fram_eventAddress);

    //Now save on the NOR that it is a little bit slower
    int8_t error = addEventNOR(newEvent, &confRegister_.nor_eventAddress);

    if(error)
        uart_print(UART_DEBUG, "ERROR: NOR memory was busy, we could not write events in.");

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
    uint32_t elapsedSeconds = seconds_uptime();

    //First save on the FRAM which is much faster
    //Save it only once every x seconds, otherwise we fill it!
    if(lastTimeTelemetrySavedFRAM_ + confRegister_.fram_tlmSavePeriod < elapsedSeconds)
    {
        //Time to save
        addTelemetryFRAM(currentTelemetryLineFRAMandNOR_[0], &confRegister_.fram_telemetryAddress);
        lastTimeTelemetrySavedFRAM_ = elapsedSeconds;

        //Reset Telemetry Line
        struct TelemetryLine newVoidTMLine = {0};
        currentTelemetryLineFRAMandNOR_[0] = newVoidTMLine;
        numTimes_baroRead_[0] = 0;
        numTimes_inaRead_[0] = 0;
        numTimes_accRead_[0] = 0;
    }

    //Now save on the NOR which is a little bit slower
    //if(1) //Trick to save a lot of them altoghether
    if(lastTimeTelemetrySavedNOR_ + confRegister_.nor_tlmSavePeriod < elapsedSeconds)
    {
        //Time to save
        int8_t error = addTelemetryNOR(&currentTelemetryLineFRAMandNOR_[1], &confRegister_.nor_telemetryAddress);
        if(error)
            uart_print(UART_DEBUG, "ERROR: NOR memory was busy, we could not write telemetry in.");

        lastTimeTelemetrySavedNOR_ = elapsedSeconds;

        //Reset Telemetry Line
        struct TelemetryLine newVoidTMLine = {0};
        currentTelemetryLineFRAMandNOR_[1] = newVoidTMLine;
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
    int8_t error = spi_NOR_readFromAddress(address, (uint8_t *) savedEvent, sizeof(struct EventLine), confRegister_.nor_deviceSelected);

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
 * It returns a stored Telemetry Line from the NOR, the pointer is the TM line num.
 */
int8_t getTelemetryNOR(uint32_t pointer, struct TelemetryLine *savedTelemetry)
{
    uint32_t address = NOR_TLM_ADDRESS + pointer * sizeof(struct TelemetryLine);
    int8_t error = spi_NOR_readFromAddress(address, (uint8_t *) savedTelemetry, sizeof(struct TelemetryLine), confRegister_.nor_deviceSelected);

    return error;
}

