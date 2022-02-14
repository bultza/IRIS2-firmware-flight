/*
 * This file is part of the Supervisor on IRIS2 Flight Firmware
 * Proyecto Daedalus - 2021
 *
 */

#include "uart.h"
#include "libhal.h"

#define UART_BUS_NUM    5
#define MUX_OFF         (P2OUT |=  BIT3)
#define MUX_ON          (P2OUT &= ~BIT3)
#define MUX_CAM3        (P2OUT |=  BIT4)
#define MUX_CAM4        (P2OUT &= ~BIT4)
#define MUX_STATUS_CAM3 (P2OUT &   BIT4)

struct Port
{
    uint8_t uart_name;

    uint8_t bufferIn[UARTBUFFERLENGHT];
    uint16_t bufferInStart;
    uint16_t bufferInEnd;

    uint8_t noDisableTXInterrupt;
    uint8_t bufferOut[UARTBUFFERLENGHT];
    uint16_t bufferOutStart;
    uint16_t bufferOutEnd;

    uint8_t somethingOnQueue;
    uint8_t interruptWhenNoData;

    uint32_t timeLastReceived;
    uint16_t counterErrorSync;
    uint16_t counterErrorCRC;
    uint16_t counterErrorOverrun;
    uint16_t counterErrorOverflow;

    uint8_t isOpen;
};

//Private variables
volatile struct Port uart_device[UART_BUS_NUM] = {0};
#pragma PERSISTENT (uart_device);
static const uint16_t baseAddress[UART_BUS_NUM] = {DEBUG_BASE, CAM1_BASE, CAM2_BASE, CAM3_BASE, CAM3_BASE};

//Private functions:
int16_t bufferSize(uint8_t isOut, uint8_t uart_name);
void uart_safe_tx_byte(uint8_t uart_name);
int8_t uart_write_internal(uint8_t uart_name, uint8_t *buffer, uint16_t lenght);

/******************************************************************************
 * It configures the UART port with the selected baudrate
 *      Select UART device name: UART_DEBUG, UART_CAM1, UART_CAM2, UART_CAM3,
 *                               UART_CAM4
 *      Select BAUDRATE: BR_9600, BR_38400, BR_115200
 * Watch out!, CAM4 is multiplexed with CAM3!!!
 */
int8_t uart_init(uint8_t uart_name, uint8_t baudrate)
{
    if(uart_name == UART_DEBUG)
    {
        //USCI_A0_
        P2SEL0 &= ~(BIT0 | BIT1);
        P2SEL1 |= (BIT0 | BIT1);
        uart_device[uart_name].uart_name = UART_DEBUG;
    }
    else if(uart_name == UART_CAM1)
    {
        //USCI_A1_
        P2SEL0 &= ~(BIT5 | BIT6);
        P2SEL1 |= (BIT5 | BIT6);
        uart_device[uart_name].uart_name = UART_CAM1;
    }
    else if(uart_name == UART_CAM2)
    {
        //USCI_A2_
        P5SEL1 &= ~(BIT4 | BIT5);
        P5SEL0 |= (BIT4 | BIT5);
        uart_device[uart_name].uart_name = UART_CAM2;
    }
    else if(uart_name == UART_CAM3)
    {
        //USCI_A3_
        P6SEL1 &= ~(BIT0 | BIT1);
        P6SEL0 |= (BIT0 | BIT1);
        uart_device[uart_name].uart_name = UART_CAM3;

        //Configure pins for Mux Enable and camera selector
        P2DIR |= BIT3;
        P2DIR |= BIT4;

        //Switch on Mux and select camera
        MUX_ON;
        MUX_CAM3;
    }
    else if(uart_name == UART_CAM4)
    {
        //This is the same as UART3!!! but multiplexed
        //USCI_A3_
        P6SEL1 &= ~(BIT0 | BIT1);
        P6SEL0 |= (BIT0 | BIT1);
        uart_device[uart_name].uart_name = UART_CAM4;

        //Configure pins for Mux Enable and camera selector
        P2DIR |= BIT3;
        P2DIR |= BIT4;

        //Switch on Mux and select camera
        MUX_ON;
        MUX_CAM4;
    }

    //Set the uart status:
    uart_device[uart_name].isOpen = 1;
    uart_device[uart_name].bufferInStart = 0;
    uart_device[uart_name].bufferInEnd = 0;
    uart_device[uart_name].bufferOutStart = 0;
    uart_device[uart_name].bufferOutEnd = 0;
    uart_device[uart_name].counterErrorSync = 0;
    uart_device[uart_name].counterErrorCRC = 0;
    uart_device[uart_name].counterErrorOverrun = 0;
    uart_device[uart_name].counterErrorOverflow = 0;

    // Put eUSCI in reset
    HWREG16(baseAddress[uart_name] + OFS_UCAxCTLW0) |= UCSWRST;
    HWREG16(baseAddress[uart_name] + OFS_UCAxCTLW0) |= UCSSEL__SMCLK;

    //Baud rates for 8 MHz clock SMCLK, see table at page 782 in users guide
    if(baudrate == BR_9600)
    {
        HWREG16(baseAddress[uart_name] + OFS_UCAxBR0) = 52;
        HWREG16(baseAddress[uart_name] + OFS_UCAxMCTLW) = 0x4900 | UCOS16 | UCBRF_1;
    }
    else if(baudrate == BR_38400)
    {
        HWREG16(baseAddress[uart_name] + OFS_UCAxBR0) = 13;
        HWREG16(baseAddress[uart_name] + OFS_UCAxMCTLW) = 0x8400 | UCOS16 | UCBRF_0;
    }
    else if(baudrate == BR_57600)
    {
        HWREG16(baseAddress[uart_name] + OFS_UCAxBR0) = 8;
        HWREG16(baseAddress[uart_name] + OFS_UCAxMCTLW) = 0xF700 | UCOS16 | UCBRF_10;
    }
    else if(baudrate == BR_115200)
    {
        HWREG16(baseAddress[uart_name] + OFS_UCAxBR0) = 4;
        HWREG16(baseAddress[uart_name] + OFS_UCAxMCTLW) = 0x5500 | UCOS16 | UCBRF_5;
    }

    //Initialize eUSCI
    HWREG16(baseAddress[uart_name] + OFS_UCAxCTLW0) &= ~UCSWRST;
    //Enable USCI_Ax RX interrupt
    HWREG16(baseAddress[uart_name] + OFS_UCAxIE) |= UCRXIE;

    return 0;
}

/**
 * It puts the GPIOs of the UARTs in high impedance, so the camera is switched off and
 * not drawing current from the pins.
 */
void uart_close(uint8_t uart_name)
{
    //Indicate that UART is closed:
    uart_device[uart_name].isOpen = 0;

    if(uart_name == UART_DEBUG)
    {
        //USCI_A0_
        P2SEL0 &= ~(BIT0 | BIT1);
        P2SEL1 &= ~(BIT0 | BIT1);
        //GPIOs as input to put in high impedance
        P2DIR  &= ~(BIT0 | BIT1);
    }
    else if(uart_name == UART_CAM1)
    {
        //USCI_A1_
        P2SEL0 &= ~(BIT5 | BIT6);
        P2SEL1 &= ~(BIT5 | BIT6);
        //GPIOs as input to put in high impedance
        P2DIR  &= ~(BIT5 | BIT6);
    }
    else if(uart_name == UART_CAM2)
    {
        //USCI_A2_
        P5SEL1 &= ~(BIT4 | BIT5);
        P5SEL0 &= ~(BIT4 | BIT5);
        //GPIOs as input to put in high impedance
        P5DIR  &= ~(BIT4 | BIT5);
    }
    else if(uart_name == UART_CAM3)
    {
        //Close port only if UART_CAM4 is also closed:
        if(uart_device[UART_CAM4].isOpen)
        {
            //Only move the switch to CAM4:
            MUX_CAM4;
        }
        else
        {
            //Both UART3 and UART4 to be closed
            //USCI_A3_
            P6SEL1 &= ~(BIT0 | BIT1);
            P6SEL0 &= ~(BIT0 | BIT1);
            //GPIOs as input to put in high impedance
            P6DIR  &= ~(BIT0 | BIT1);

            //Switch off Mux:
            MUX_OFF;
        }
    }
    else if(uart_name == UART_CAM4)
    {
        //Close port only if UART_CAM3 is also closed:
        if(uart_device[UART_CAM3].isOpen)
        {
            //Only move the switch to CAM3:
            MUX_CAM3;
        }
        else
        {
            //Both UART3 and UART4 to be closed
            //USCI_A3_
            P6SEL1 &= ~(BIT0 | BIT1);
            P6SEL0 &= ~(BIT0 | BIT1);
            //GPIOs as input to put in high impedance
            P6DIR  &= ~(BIT0 | BIT1);

            //Switch off Mux:
            MUX_OFF;
        }
    }

    //Clear buffers:
    uart_clear_buffer(uart_name);
}

/**
 * It clears all the pointers and counters of a specific port
 */
void uart_clear_buffer(uint8_t uart_name)
{
    uart_device[uart_name].bufferInStart = 0;
    uart_device[uart_name].bufferInEnd = 0;
    uart_device[uart_name].bufferOutStart = 0;
    uart_device[uart_name].bufferOutEnd = 0;
    uart_device[uart_name].counterErrorSync = 0;
    uart_device[uart_name].counterErrorCRC = 0;
    uart_device[uart_name].counterErrorOverrun = 0;
    uart_device[uart_name].counterErrorOverflow = 0;
}


/*
 * It counts the size of the buffer
 * This function must not be called outside of this file (it is not available
 * in the uart.h file.
 */
int16_t bufferSize(uint8_t isOut, uint8_t uart_name)
{
    if(isOut == 0)
    {
        if(uart_device[uart_name].bufferInEnd > uart_device[uart_name].bufferInStart)
            return ((uart_device[uart_name].bufferInEnd) - (uart_device[uart_name].bufferInStart));
        else if(uart_device[uart_name].bufferInEnd < uart_device[uart_name].bufferInStart)
            return (UARTBUFFERLENGHT - uart_device[uart_name].bufferInStart + uart_device[uart_name].bufferInEnd);
        else
            uart_device[uart_name].somethingOnQueue = 0;
    }
    else
    {
        if(uart_device[uart_name].bufferOutEnd > uart_device[uart_name].bufferOutStart)
            return (uart_device[uart_name].bufferOutEnd - uart_device[uart_name].bufferOutStart);
        else if(uart_device[uart_name].bufferOutEnd < uart_device[uart_name].bufferOutStart)
            return (UARTBUFFERLENGHT - uart_device[uart_name].bufferOutStart + uart_device[uart_name].bufferOutEnd);
    }
    return 0;
}


/*
 * Returns the number of available bytes in the buffer
 */
int16_t uart_available(uint8_t uart_name)
{
    return bufferSize(0, uart_name);
}

/*
 * Returns the number of bytes that have not yet been sent
 */
int16_t uart_tx_onWait(uint8_t uart_name)
{
    return bufferSize(1, uart_name);
}

uint8_t pointerTimes = 0;
uint32_t timesDebug[30];

/*
 * It blocks execution until uart is empty, or after 20ms
 */
void uart_flush(uint8_t uart_name)
{
    uint32_t timeStart = (uint32_t)millis_uptime();
    while(uart_tx_onWait(uart_name))
    {
        uint32_t timeNow = (uint32_t)millis_uptime();
        if(timeStart + 20 < timeNow)
        {
            timesDebug[pointerTimes] = timeNow - timeStart;
            pointerTimes++;
            if(pointerTimes >= 30)
                pointerTimes = 0;
            return;//break execution just in case
        }
    }

    uint32_t timeNow = (uint32_t)millis_uptime();
    timesDebug[pointerTimes] = timeNow - timeStart;
    pointerTimes++;
    if(pointerTimes >= 30)
        pointerTimes = 0;

    sleep_ms(5);
}

/*
 * Returns the last byte in buffer
 */
uint8_t uart_read(uint8_t uart_name)
{
    //Buffer was empty. Send last byte as safe
    if(uart_device[uart_name].bufferInEnd == uart_device[uart_name].bufferInStart)
        return (uart_device[uart_name].bufferIn[(uart_device[uart_name].bufferInStart)]);

    uint8_t byteToSend;
    byteToSend = uart_device[uart_name].bufferIn[(uart_device[uart_name].bufferInStart)];

    //Increment buffer pointer
    uart_device[uart_name].bufferInStart++;

    //Sanity check
    if(uart_device[uart_name].bufferInStart >= UARTBUFFERLENGHT)
        uart_device[uart_name].bufferInStart = 0;

    return byteToSend;
}


void uart_safe_tx_byte(uint8_t uart_name)
{
    if(uart_device[uart_name].interruptWhenNoData == 0)
        return;

    uart_device[uart_name].interruptWhenNoData = 0;
    //Watch out, is not that the buffer is now empty, isn't it?
    if(uart_device[uart_name].bufferOutEnd == uart_device[uart_name].bufferOutStart)
        return ;

    uint8_t byte2write = uart_device[uart_name].bufferOut[(uart_device[uart_name].bufferOutStart)];
    uart_device[uart_name].bufferOutStart++;
    if(uart_device[uart_name].bufferOutStart >= UARTBUFFERLENGHT)
        uart_device[uart_name].bufferOutStart = 0;

    //We need to refill the buffer

    while((HWREG16(baseAddress[uart_name]+ OFS_UCAxSTATW)) & UCBUSY);
    HWREG16(baseAddress[uart_name]+ OFS_UCAxTXBUF) = byte2write;
    //Reenable interrupt
    HWREG16(baseAddress[uart_name]+ OFS_UCAxIE) |= UCTXIE;

}

/*
 * This function sends a buffer to the selected port.
 * This function must not be called outside of this file (it is not available
 * in the g_uart.h file.
 */
int8_t uart_write_internal(uint8_t uart_name, uint8_t *buffer, uint16_t lenght)
{
    //Copy next byte to buffer
    uint32_t position = 0;

    uint8_t isBusy = 0;

    if((bufferSize(1, uart_name) == 0) && (isBusy == 0))
    {
        while((HWREG16(baseAddress[uart_name]+ OFS_UCAxSTATW)) & UCBUSY);
        HWREG16(baseAddress[uart_name]+ OFS_UCAxTXBUF) = *buffer;

        buffer++;
        position++;
    }

    while (position != lenght)
    {
        uint16_t wait = 1000;
        //Block code to avoid buffer overflow
        //TODO, we need to add a failsafe to overwrite this.
        while(bufferSize(1, uart_name) == (UARTBUFFERLENGHT - 1))
        {
            //It can happen that with multiple interrupts,
            //TX is not triggered so we have to force the TX...
            if(uart_device[uart_name].interruptWhenNoData == 1)
                uart_safe_tx_byte(uart_name);

            wait--;
            if(wait == 0)
                break;
        }

        //Ok there is space in the buffer, lets go:
        uart_device[uart_name].bufferOut[(uart_device[uart_name].bufferOutEnd)] = *buffer;
        position++;

        uart_device[uart_name].bufferOutEnd++;
        if(uart_device[uart_name].bufferOutEnd >= UARTBUFFERLENGHT)
            uart_device[uart_name].bufferOutEnd = 0;

        if(position >= lenght)
            break;

        //Do it as the last thing to avoid buffer overflow
        buffer++;
    }

    //It can happen that the interrupt started while buffer was still empty
    //so now that has been filled, lets trigger it again:
    if(uart_device[uart_name].interruptWhenNoData == 1)
        uart_safe_tx_byte(uart_name);

    return 0;
}


/*
 * Writes a buffer to the port.
 */
int8_t uart_write(uint8_t uart_name, uint8_t *buffer, uint16_t length)
{
    //Is the port open??
    if(uart_device[uart_name].isOpen == 0)
        return -1;  //No! very much error

    uart_device[uart_name].noDisableTXInterrupt = 1;

    //Should we multiplex?
    if(uart_name == UART_CAM3)
        MUX_CAM3;
    else if(uart_name == UART_CAM4)
        MUX_CAM4;

    //Enable TX interrupt
    HWREG16(baseAddress[uart_name]+ OFS_UCAxIE) |= UCTXIE;

    uart_write_internal(uart_name, buffer, length);


    uart_device[uart_name].noDisableTXInterrupt = 0;

    return 0;
}

/**
 * It prints something on the UART :-D
 */
int8_t uart_print(uint8_t uart_name, char *buffer)
{
    uint16_t i, lenght;
    for(i = 0; i < UARTBUFFERLENGHT; i++)
        if(buffer[i] == 0)
        {
            lenght = i;
            break;
        }

    return uart_write(uart_name, (uint8_t *) buffer, lenght);
}

//////////////////////////////////////////////////////////////////////////////
/**
 * Interrupt for UART_DEBUG
 */
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
    switch(__even_in_range(UCA0IV,USCI_UART_UCTXCPTIFG))
    {
        case USCI_NONE: break;
        case USCI_UART_UCRXIFG:
        {
            UCA0IFG &=~ UCRXIFG;            // Clear interrupt
            //UCA0IE &=~UCRXIE;   //Disable RX interrupt

            //Check RX Overrun flag
            if((UCA0STATW & UCOE) != 0)
                uart_device[UART_DEBUG].counterErrorOverrun++;

            uart_device[UART_DEBUG].bufferIn[(uart_device[UART_DEBUG].bufferInEnd)] = UCA0RXBUF;
            uart_device[UART_DEBUG].bufferInEnd++;
            if((uart_device[UART_DEBUG].bufferInEnd) >= UARTBUFFERLENGHT)
                uart_device[UART_DEBUG].bufferInEnd = 0;

            //Buffer overflow? delete last character
            if((uart_device[UART_DEBUG].bufferInEnd) == (uart_device[UART_DEBUG].bufferInStart))
            {
                uart_device[UART_DEBUG].counterErrorOverflow++;
                uart_device[UART_DEBUG].bufferInStart++;
            }

            if((uart_device[UART_DEBUG].bufferInStart) >= UARTBUFFERLENGHT)
              uart_device[UART_DEBUG].bufferInStart = 0;


            uart_device[UART_DEBUG].somethingOnQueue = 1;

            //UCA0IE |= UCRXIE; //Enable RX interrupt

        }break;

        case USCI_UART_UCTXIFG:
        {

            if((uart_device[UART_DEBUG].bufferOutEnd) == (uart_device[UART_DEBUG].bufferOutStart))
            {
                //Finish sending, disable TX interrupt and bye bye
                if((uart_device[UART_DEBUG].noDisableTXInterrupt) == 0)
                    UCA0IE &= ~UCTXIE;
                else
                {
                    UCA0IE &= ~UCTXIE;
                    uart_device[UART_DEBUG].interruptWhenNoData = 1;
                }
                break;
            }

            //We have to send the next byte in the buffer
            UCA0TXBUF = uart_device[UART_DEBUG].bufferOut[(uart_device[UART_DEBUG].bufferOutStart)];

            uart_device[UART_DEBUG].bufferOutStart++;
            if(uart_device[UART_DEBUG].bufferOutStart >= UARTBUFFERLENGHT)
                uart_device[UART_DEBUG].bufferOutStart = 0;

            uart_device[UART_DEBUG].interruptWhenNoData = 0;


        }break;

        case USCI_UART_UCSTTIFG: break;
        case USCI_UART_UCTXCPTIFG: break;
    }
}

//////////////////////////////////////////////////////////////////////////////
/**
 * Interrupt for UART_CAM1
 */
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
    switch(__even_in_range(UCA1IV,USCI_UART_UCTXCPTIFG))
    {
        case USCI_NONE: break;
        case USCI_UART_UCRXIFG:
        {
            UCA1IFG &=~ UCRXIFG;            // Clear interrupt

            //Check RX Overrun flag
            if((UCA1STATW & UCOE) != 0)
                uart_device[UART_CAM1].counterErrorOverrun++;

            uint8_t buffer = UCA1RXBUF;
            uart_device[UART_CAM1].bufferIn[(uart_device[UART_CAM1].bufferInEnd)] = buffer;
            uart_device[UART_CAM1].bufferInEnd++;
            if((uart_device[UART_CAM1].bufferInEnd) >= UARTBUFFERLENGHT)
                uart_device[UART_CAM1].bufferInEnd = 0;

            //Buffer overflow? delete last character
            if((uart_device[UART_CAM1].bufferInEnd) == (uart_device[UART_CAM1].bufferInStart))
            {
                uart_device[UART_CAM1].counterErrorOverflow++;
                uart_device[UART_CAM1].bufferInStart++;
            }


            if((uart_device[UART_CAM1].bufferInStart) >= UARTBUFFERLENGHT)
              uart_device[UART_CAM1].bufferInStart = 0;


            uart_device[UART_CAM1].somethingOnQueue = 1;

            if(confRegister_.debugUART == 1)
                UCA0TXBUF = buffer;
        }break;

        case USCI_UART_UCTXIFG:
        {
            if((uart_device[UART_CAM1].bufferOutEnd) == (uart_device[UART_CAM1].bufferOutStart))
            {
                //Finish sending, disable TX interrupt and bye bye
                if((uart_device[UART_CAM1].noDisableTXInterrupt) == 0)
                    UCA1IE &= ~UCTXIE;
                else
                {
                    UCA1IE &= ~UCTXIE;
                    uart_device[UART_CAM1].interruptWhenNoData = 1;
                }
                break;
            }

            //We have to send the next byte in the buffer
            UCA1TXBUF = uart_device[UART_CAM1].bufferOut[(uart_device[UART_CAM1].bufferOutStart)];

            uart_device[UART_CAM1].bufferOutStart++;
            if(uart_device[UART_CAM1].bufferOutStart >= UARTBUFFERLENGHT)
                uart_device[UART_CAM1].bufferOutStart = 0;

            uart_device[UART_CAM1].interruptWhenNoData = 0;
        }break;
        case USCI_UART_UCSTTIFG: break;
        case USCI_UART_UCTXCPTIFG: break;
    }
}

//////////////////////////////////////////////////////////////////////////////
/**
 * Interrupt for UART_CAM2
 */
#pragma vector=USCI_A2_VECTOR
__interrupt void USCI_A2_ISR(void)
{
    switch(__even_in_range(UCA2IV,USCI_UART_UCTXCPTIFG))
    {
        case USCI_NONE: break;
        case USCI_UART_UCRXIFG:
        {
            UCA2IFG &=~ UCRXIFG;            // Clear interrupt

            //Check RX Overrun flag
            if((UCA2STATW & UCOE) != 0)
                uart_device[UART_CAM2].counterErrorOverrun++;

            uint8_t buffer = UCA2RXBUF;
            uart_device[UART_CAM2].bufferIn[(uart_device[UART_CAM2].bufferInEnd)] = buffer;
            uart_device[UART_CAM2].bufferInEnd++;
            if((uart_device[UART_CAM2].bufferInEnd) >= UARTBUFFERLENGHT)
                uart_device[UART_CAM2].bufferInEnd = 0;

            //Buffer overflow? delete last character
            if((uart_device[UART_CAM2].bufferInEnd) == (uart_device[UART_CAM2].bufferInStart))
            {
                uart_device[UART_CAM2].counterErrorOverflow++;
                uart_device[UART_CAM2].bufferInStart++;
            }


            if((uart_device[UART_CAM2].bufferInStart) >= UARTBUFFERLENGHT)
              uart_device[UART_CAM2].bufferInStart = 0;


            uart_device[UART_CAM2].somethingOnQueue = 1;

            if(confRegister_.debugUART == 2)
                UCA0TXBUF = buffer;
        }break;

        case USCI_UART_UCTXIFG:
        {
            if((uart_device[UART_CAM2].bufferOutEnd) == (uart_device[UART_CAM2].bufferOutStart))
            {
                //Finish sending, disable TX interrupt and bye bye
                if((uart_device[UART_CAM2].noDisableTXInterrupt) == 0)
                    UCA2IE &= ~UCTXIE;
                else
                {
                    UCA2IE &= ~UCTXIE;
                    uart_device[UART_CAM2].interruptWhenNoData = 1;
                }
                break;
            }

            //We have to send the next byte in the buffer
            UCA2TXBUF = uart_device[UART_CAM2].bufferOut[(uart_device[UART_CAM2].bufferOutStart)];

            uart_device[UART_CAM2].bufferOutStart++;
            if(uart_device[UART_CAM2].bufferOutStart >= UARTBUFFERLENGHT)
                uart_device[UART_CAM2].bufferOutStart = 0;

            uart_device[UART_CAM2].interruptWhenNoData = 0;
        }break;
        case USCI_UART_UCSTTIFG: break;
        case USCI_UART_UCTXCPTIFG: break;
    }
}

//////////////////////////////////////////////////////////////////////////////
/**
 * Interrupt for UART_CAM3 and UART_CAM4
 */
#pragma vector=USCI_A3_VECTOR
__interrupt void USCI_A3_ISR(void)
{
    //Which UART is it?
    uint8_t uartName = UART_CAM4;
    if(MUX_STATUS_CAM3)
        uartName = UART_CAM3;

    switch(__even_in_range(UCA3IV,USCI_UART_UCTXCPTIFG))
    {
        case USCI_NONE: break;
        case USCI_UART_UCRXIFG:
        {
            UCA3IFG &=~ UCRXIFG;            // Clear interrupt

            //Check RX Overrun flag
            if((UCA3STATW & UCOE) != 0)
                uart_device[uartName].counterErrorOverrun++;

            uint8_t buffer = UCA3RXBUF;
            uart_device[uartName].bufferIn[(uart_device[uartName].bufferInEnd)] = buffer;
            uart_device[uartName].bufferInEnd++;
            if((uart_device[uartName].bufferInEnd) >= UARTBUFFERLENGHT)
              uart_device[uartName].bufferInEnd = 0;

            //Buffer overflow? delete last character
            if((uart_device[uartName].bufferInEnd) == (uart_device[uartName].bufferInStart))
            {
                uart_device[uartName].counterErrorOverflow++;
                uart_device[uartName].bufferInStart++;
            }

            if((uart_device[uartName].bufferInStart) >= UARTBUFFERLENGHT)
              uart_device[uartName].bufferInStart = 0;


            uart_device[uartName].somethingOnQueue = 1;
            if(confRegister_.debugUART == uartName)
                UCA0TXBUF = buffer;
        }break;

        case USCI_UART_UCTXIFG:
        {
            if((uart_device[uartName].bufferOutEnd) == (uart_device[uartName].bufferOutStart))
            {
                //Finish sending, disable TX interrupt and bye bye
                if(uart_device[uartName].noDisableTXInterrupt == 0)
                    UCA3IE &= ~UCTXIE;
                else
                {
                    UCA3IE &= ~UCTXIE;
                    uart_device[uartName].interruptWhenNoData = 1;
                }
                break;
            }

            //We have to send the next byte in the buffer
            UCA3TXBUF = uart_device[uartName].bufferOut[(uart_device[uartName].bufferOutStart)];

            uart_device[uartName].bufferOutStart++;
            if((uart_device[uartName].bufferOutStart) >= UARTBUFFERLENGHT)
                uart_device[uartName].bufferOutStart = 0;

            uart_device[uartName].interruptWhenNoData = 0;
        }break;
        case USCI_UART_UCSTTIFG: break;
        case USCI_UART_UCTXCPTIFG: break;
    }
}

