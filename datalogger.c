#include "datalogger.h"

//When was the last time each sensors was monitored?
uint64_t lastTime_baroRead_ = 0;
uint64_t lastTime_inaRead_ = 0;
uint64_t lastTime_accRead_ = 0;
uint64_t lastTime_tempRead_ = 0;

uint32_t lastAltitude_ = 0;

//Current status of the sensors
struct TelemetryLine currentTelemetryLineFRAM_;
uint16_t currentTelemetryLineIterationsFRAM_ = 0;
struct TelemetryLine currentTelemetryLineNOR_;
uint16_t currentTelemetryLineIterationsNOR_ = 0;

//History of altitudes
struct AltitudesHistory
{
    uint64_t time;
    uint32_t altitude;
};
struct AltitudesHistory altitudeHistory_[5];
uint8_t altitudeHistoryIndex_ = 0;

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
        int32_t pressure;
        int32_t altitude;
        i2c_MS5611_getPressure(&pressure);
        i2c_MS5611_getAltitude(&pressure, &altitude);
        currentTelemetryLineFRAM_.altitude = altitude;
        currentTelemetryLineFRAM_.pressure = pressure;
        /*
        //For FRAM line
        if(currentTelemetryLineIterationsFRAM_ == 0)
        {
            //First time, so restart everything:
            lastAltitude_ = altitude;
            currentTelemetryLineFRAM_.verticalSpeed[AVG_INDEX] = 0;     //Average 0
            currentTelemetryLineFRAM_.verticalSpeed[MAX_INDEX] = 0;     //Maximum 0
            currentTelemetryLineFRAM_.verticalSpeed[MIN_INDEX] = 0;   //Minimum 99
        }

        //Calculate Speed:
        int16_t speed = (altitude - lastAltitude_) / (uptime - lastTime_baroRead_);
        if(speed > currentTelemetryLineFRAM_.verticalSpeed[MAX_INDEX])
            currentTelemetryLineFRAM_.verticalSpeed[MAX_INDEX] = speed;
        if(speed < currentTelemetryLineFRAM_.verticalSpeed[MIN_INDEX])
            currentTelemetryLineFRAM_.verticalSpeed[MIN_INDEX] = speed;

        currentTelemetryLineFRAM_.verticalSpeed[AVG_INDEX] =
                currentTelemetryLineFRAM_.verticalSpeed[AVG_INDEX]
                + (speed - currentTelemetryLineFRAM_.verticalSpeed[AVG_INDEX]) / currentTelemetryLineIterationsFRAM_;

        //For NOR line
        //TODO

        lastAltitude_ = currentTelemetryLineFRAM_.altitude;
        currentTelemetryLineIterationsFRAM_++;
        currentTelemetryLineIterationsNOR_++;*/
        lastTime_baroRead_ = uptime;
    }
    //Time to read temperatures?
    else if(lastTime_tempRead_ + confRegister_.temp_readPeriod < uptime)
    {
        //Time to read barometer
        //TODO
        lastTime_tempRead_ = uptime;
    }
    //Time to read voltages and currents?
    else if(lastTime_inaRead_ + confRegister_.ina_readPeriod < uptime)
    {
        //Time to read barometer
        //TODO
        lastTime_inaRead_ = uptime;
    }
    //Time to read voltages and currents?
    else if(lastTime_accRead_ + confRegister_.acc_readPeriod < uptime)
    {
        //Time to read barometer
        //TODO
        lastTime_accRead_ = uptime;
    }
}

/**
 * It saves an event on the FRAM and NOR memories
 */
int8_t saveEvent(struct EventLine newEvent)
{
    //First save on the FRAM which is much faster:
    addEventFRAM(newEvent, &confRegister_.fram_eventAddress);

    //Now save on the NOR that it is a little bit slower
    //TODO

    return 0;
}

uint32_t lastTimeTelemetrySavedNOR_ = 0;
uint32_t lastTimeTelemetrySavedFRAM_ = 0;

/**
 * It saves a telemetry on the FRAM and on the NOR memories
 */
int8_t saveTelemetry(struct TelemetryLine newTelemetry)
{
    uint32_t elapsedSeconds = seconds_uptime();
    //First save on the FRAM which is much faster
    //Save it only once every x seconds, otherwise we fill it!
    if(lastTimeTelemetrySavedFRAM_ + confRegister_.fram_tlmSavePeriod < elapsedSeconds)
    {
        //Time to save
        addTelemetryFRAM(newTelemetry, &confRegister_.fram_telemetryAddress);
        lastTimeTelemetrySavedFRAM_ = elapsedSeconds;
    }

    //Now save on the NOR which is a little bit slower
    if(lastTimeTelemetrySavedNOR_ + confRegister_.nor_tlmSavePeriod < elapsedSeconds)
    {
        //Time to save
        //TODO
        lastTimeTelemetrySavedNOR_ = elapsedSeconds;
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
            (*address + sizeof(newTelemetry)) > (FRAM_TLM_ADDRESS + FRAM_TLM_SIZE))
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

    uint32_t address = FRAM_TLM_ADDRESS + pointer * sizeof(struct TelemetryLine);
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




