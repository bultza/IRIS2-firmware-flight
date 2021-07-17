#include "i2c.h"

struct I2cPort i2cPorts[2];


/**
 * It configures the GPIOs for the two I2C ports, internal and external
 * I2C Bus00(Ports 1.6 [SDA] and 1.7[SCL], UCB0)
 * I2C Bus01(Ports 7.0 [SDA] and 7.1[SCL], UCB2) -> External
 * On the Internal bus we have the following connected:
 *  - Temperature sensor
 *  - Barometer
 *  - Accelerometer
 *  - INA
 *  - RTC
 */
void i2c_master_init()
{
    //Init variables:
    i2cPorts[I2C_BUS00].name = I2C_BUS00;
    i2cPorts[I2C_BUS00].baseAddress = 0x0640; //From page 128 of Datasheet:
    i2cPorts[I2C_BUS00].isOnError = 0;
    i2cPorts[I2C_BUS00].pointerAddress = 0;
    i2cPorts[I2C_BUS00].repeatedStart = 0;
    i2cPorts[I2C_BUS00].inRepeatedStartCondition = 0;

    i2cPorts[I2C_BUS01].name = I2C_BUS01;
    i2cPorts[I2C_BUS01].baseAddress = 0x06C0; //From page 128 of Datasheet:
    i2cPorts[I2C_BUS01].isOnError = 0;
    i2cPorts[I2C_BUS01].pointerAddress = 0;
    i2cPorts[I2C_BUS01].repeatedStart = 0;
    i2cPorts[I2C_BUS01].inRepeatedStartCondition = 0;


    //Configure GPIO for BUS00, UCB0
    P1SEL0 &= ~(BIT6 | BIT7);
    P1SEL1 |= BIT6 | BIT7;

    //Configure GPIO for BUS01, UCB2
    P7SEL0 |= BIT0 | BIT1;
    P7SEL1 &= ~(BIT0 | BIT1);

    uint8_t i;
    //Configure both I2C Buses:
    for(i = 0; i < 2; i++)
    {
        //SW reset enabled
        HWREG16(i2cPorts[i].baseAddress + OFS_UCBxCTLW0) |= UCSWRST;
        // I2C mode, Master mode, sync
        HWREG16(i2cPorts[i].baseAddress + OFS_UCBxCTLW0) |= UCMODE_3 | UCMST | UCSYNC;
        // baudrate = SMCLK / 20 = 400k
        HWREG16(i2cPorts[i].baseAddress + OFS_UCBxBRW) = 0x0014;
        // baudrate = SMCLK / 40 = 200k
        //HWREG16(i2cPorts[i].baseAddress + OFS_UCBxBRW) = 0x0028;
        HWREG16(i2cPorts[i].baseAddress + OFS_UCBxCTLW0) &= ~UCSWRST;
        //Clock low timeout, approximately 28 ms
        HWREG16(i2cPorts[i].baseAddress + OFS_UCBxCTLW1) |= UCCLTO_1;
    }
}

/**
 * It starts i2c transmission on selected bus.
 */
int8_t i2c_begin_transmission(uint8_t busSelect,
                              uint8_t address,
                              uint8_t asReceiver)
{
    if(busSelect > 1)
        return -4;  //Bus does not exist

    uint16_t baseAddress = i2cPorts[busSelect].baseAddress;

    //Make sure stop condition got sent:
    uint32_t counter = 0;
    while (HWREG16(baseAddress + OFS_UCBxCTL1) & UCTXSTP)
    {
        counter++;
        if (counter > I2CTIMEOUTCYCLES)
        {
            i2cPorts[busSelect].isOnError = 1;
            //I2C stop condition
            HWREG16(baseAddress + OFS_UCBxCTL1) |= UCTXSTP;
            return -2;  //Very error
        }
    }

    //Set the address:
    HWREG16(baseAddress + OFS_UCBxI2CSA) = address;

    if (asReceiver == 0)
        HWREG16(baseAddress + OFS_UCBxCTL1) |= UCTR;   //As TX
    else
    {
        //Flush any previous byte:
        uint8_t foo = HWREG16(baseAddress + OFS_UCBxRXBUF);
        HWREG16(baseAddress + OFS_UCBxCTL1) &= ~UCTR;  //As Rx
    }

    //Generate START condition on bus
    HWREG16(baseAddress + OFS_UCBxCTL1) |= UCTXSTT;

    return 0;
}

/**
 * Write a byte in the selected bus
 */
int8_t i2c_write_firstbyte(uint8_t busSelect, uint8_t *buffer)
{
    if(busSelect > 1)
        return -4;  //Bus does not exist

    uint16_t baseAddress = i2cPorts[busSelect].baseAddress;

    //address is being transmitted as this time, we have to write something
    //in buffer now in order to prevent bus being stalled:

    // Wait for UCTXIF
    while (HWREG16(baseAddress + OFS_UCBxIFG) & !UCTXIFG);

    //Now wait for start condition flag is set to 0. This happens when address
    //Is finished being sent. It might hang if there is a malfunction on the i2c
    //bus like a short-circuit
    uint32_t counter = 0;
    while (HWREG16(baseAddress + OFS_UCBxCTL1) & UCTXSTT)
    {
        counter++;
        if (counter > I2CTIMEOUTCYCLES)
        {
            i2cPorts[busSelect].isOnError = 1;
            return -2;  //Very error
        }
    }

    //Fill the tx buffer
    HWREG16(baseAddress + OFS_UCBxTXBUF) = *buffer;

    //Ok, we need to check if there was acknowledge
    if ((HWREG16(baseAddress + OFS_UCBxIFG) & UCNACKIFG))
    {
        // We got NACK!
        while (HWREG16(baseAddress + OFS_UCBxIFG) & !UCTXIFG);
        //I2C stop condition
        HWREG16(baseAddress + OFS_UCBxCTL1) |= UCTXSTP;
        //Clear Nack Flag
        HWREG16(baseAddress + OFS_UCBxIFG) &= ~UCNACKIFG;
        i2cPorts[busSelect].isOnError = 1;
        return -1;
    }

    //Ok, everything worked well, at this point first byte is being transmitted
    i2cPorts[busSelect].isOnError = 0;
    return 0;
}

/**
 * function to begin the transmission to the selected bus and write a byte and receive ACK or NACK
 */
int8_t sat_i2c_write(uint8_t busSelect, uint8_t address, uint8_t *buffer,
                     uint16_t length, uint8_t repeatedStart)
{
    if(busSelect > 1)
        return -4;  //Bus does not exist

    uint16_t baseAddress = i2cPorts[busSelect].baseAddress;

    //Going into a repeated start?
    if (i2cPorts[busSelect].inRepeatedStartCondition == 1)
    {
        //Repeated start
        //As TX
        HWREG16(baseAddress + OFS_UCBxCTL1) |= UCTR;
        //Repeated start condition
        HWREG16(baseAddress + OFS_UCBxCTL1) |= UCTXSTT;
    }
    else
    {
        //Starting from scratch
        //Begin transmission
        if (i2c_begin_transmission(busSelect, address, 0) != 0)
            return -2;

        //Send the first byte and return with error if unack
        if (i2c_write_firstbyte(busSelect, buffer) != 0)
            return -1;

        //First byte was already sent
        buffer++;
        length--;

        //First byte is being transmitted at this point. We need to stop
        //sending bytes or continue if buffer is not yet empty
    }

    //Now we send the bytes:
    while (length)
    {
        //UCTXIFG is set again as soon as the data is transferred
        //from the buffer into the shift register
        while (!(HWREG16(baseAddress + OFS_UCBxIFG) & UCTXIFG))
        {
            if ((HWREG16(baseAddress + OFS_UCBxIFG) & UCNACKIFG))
            {
                // We got NACK!
                //I2C stop condition
                HWREG16(baseAddress + OFS_UCBxCTL1) |= UCTXSTP;
                //Clear Nack Flag
                HWREG16(baseAddress + OFS_UCBxIFG) &= ~UCNACKIFG;
                i2cPorts[busSelect].isOnError = 1;
                i2cPorts[busSelect].inRepeatedStartCondition = 0;
                return -1;
            }
        }

        //Fill again the tx buffer
        HWREG16(baseAddress + OFS_UCBxTXBUF) = *buffer;

        length--;
        if (length != 0)
            buffer++;

    }

    // Wait for TX buffer to empty
    while (!(HWREG16(baseAddress + OFS_UCBxIFG) & UCTXIFG))
        if ((HWREG16(baseAddress + OFS_UCBxIFG) & UCNACKIFG))
        {
            // We got NACK!
            //I2C stop condition
            HWREG16(baseAddress + OFS_UCBxCTL1) |= UCTXSTP;
            //Clear Nack Flag
            HWREG16(baseAddress + OFS_UCBxIFG) &= ~UCNACKIFG;
            i2cPorts[busSelect].isOnError = 1;
            i2cPorts[busSelect].inRepeatedStartCondition = 0;
            return -1;
        }

    if (repeatedStart == 0)
    {
        //I2C stop condition
        HWREG16(baseAddress + OFS_UCBxCTL1) |= UCTXSTP;

        //Set flag by software:
        HWREG16(baseAddress + OFS_UCBxIFG) &= ~UCTXIFG;

        //Clear bus errors
        i2cPorts[busSelect].isOnError = 0;
        i2cPorts[busSelect].inRepeatedStartCondition = 0;

        while (HWREG16(baseAddress + OFS_UCBxCTL1) & UCTXSTP);
        return 0;
    }

    //A repeated start is needed in next transaction!
    i2cPorts[busSelect].inRepeatedStartCondition = 1;
    return 0;
}

/**
 * Read two bytes of the selected bus and address with the temperature read in the sensor
 */
int8_t sat_i2c_requestFrom(uint8_t busSelect, uint8_t address, uint8_t *buffer,
                           uint16_t length, uint8_t repeatedStart)
{
    if(busSelect > 1)
        return -4;  //Bus does not exist

    uint16_t baseAddress = i2cPorts[busSelect].baseAddress;

    //Going into a repeated start?
    if (i2cPorts[busSelect].inRepeatedStartCondition == 1)
    {
        //Flush any previous byte:
        uint8_t foo = HWREG16(baseAddress + OFS_UCBxRXBUF);
        //As Rx
        HWREG16(baseAddress + OFS_UCBxCTL1) &= ~UCTR;
        //Repeated start condition
        HWREG16(baseAddress + OFS_UCBxCTL1) |= UCTXSTT;
    }
    else
    {
        //Begin transmission as reader and return if slave does not answer
        if (i2c_begin_transmission(busSelect, address, 1) != 0)
            return -2;  //Bus was in error
    }

    uint32_t wait = I2CTIMEOUTCYCLES;


    //Wait for the start condition to be send:
    uint32_t counter = 0;
    while (HWREG16(baseAddress + OFS_UCBxCTL1) & UCTXSTT)
    {
        counter++;
        if (counter > I2CTIMEOUTCYCLES)
        {
            i2cPorts[busSelect].isOnError = 1;
            i2cPorts[busSelect].inRepeatedStartCondition = 0;
            HWREG16(baseAddress + OFS_UCBxCTL1) |= UCTXSTP;
            return -2;  //Very error
        }
    }

    if ((HWREG16(baseAddress + OFS_UCBxIFG) & UCNACKIFG))
    {
        // We got NACK!
        //I2C stop condition
        HWREG16(baseAddress + OFS_UCBxCTL1) |= UCTXSTP;
        //Clear Nack Flag
        HWREG16(baseAddress + OFS_UCBxIFG) &= ~UCNACKIFG;
        i2cPorts[busSelect].isOnError = 1;
        i2cPorts[busSelect].inRepeatedStartCondition = 0;
        return -1;
    }

    // Only one byte to be received?
    if (length == 1)
    {
        if (repeatedStart == 0)
            HWREG16(baseAddress + OFS_UCBxCTL1) |= UCTXSTP + UCTXNACK; // Generate I2C stop condition and NACK
        else
        {
            HWREG16(baseAddress + OFS_UCBxCTL1) |= UCTXNACK;  // Send NACK
            i2cPorts[busSelect].inRepeatedStartCondition = 1;
        }
    }

    //Receive each byte only when something available
    while (length)
    {
        //wait for receive flag to be set
        if ((HWREG16(baseAddress + OFS_UCBxIFG) & UCRXIFG))
        {
            //Move byte from hardware buffer
            *buffer = HWREG16(baseAddress + OFS_UCBxRXBUF);
            length--;

            //Move pointer buffer
            if (length != 0)
                buffer++;

            // Only one byte left?
            if (length == 1)
            {
                if (repeatedStart == 0)
                    HWREG16(baseAddress + OFS_UCBxCTL1) |= UCTXSTP + UCTXNACK; // Generate I2C stop condition and NACK
                else
                {
                    HWREG16(baseAddress + OFS_UCBxCTL1) |= UCTXNACK;  // Send NACK
                    i2cPorts[busSelect].inRepeatedStartCondition = 1;
                }
            }
        }
        else
        {
            if (--wait == 0)
            {
                //if this transaction took too long, bail
                length = 0;
                HWREG16(baseAddress + OFS_UCBxCTL1) |= UCTXSTP;
                i2cPorts[busSelect].isOnError = 1;
                i2cPorts[busSelect].inRepeatedStartCondition = 0;
                //Exit with errors
                return -1;
            }
        }
    }

    i2cPorts[busSelect].isOnError = 0;

    if (repeatedStart == 0)
    {
        //Wait for repeated start to be sent
        while (HWREG16(baseAddress + OFS_UCBxCTL1) & UCTXSTP);
        i2cPorts[busSelect].inRepeatedStartCondition = 0;

        //Exit without errors
        return 0;
    }

    return 0;   //Repeated start! :-D
}

