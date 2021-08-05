#include "datalogger.h"

/**
 * It saves an event on the FRAM and NOR memories
 */
int8_t saveEvent(struct EventLine newEvent)
{
    //First save on the FRAM which is much faster:
    addEventFRAM(newEvent, &confRegister_.fram_eventAddress);

    //Now save on the NOR that it is a little bit slower
    //TODO
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




