#include <spi.h>

uint8_t rxdata;

void spi_init(double clockrate)
{

    //-- Setup SPI B1

    UCB1CTLW0 |= UCSWRST;       // Put B1 into SW Reset

    UCB1CTLW0 |= UCSSEL__SMCLK; // Choose SMCLK
    UCB1BRW = clockrate;        // Set prescaler to 10 to get SCLK = 1M/10 = 100 kHz

    UCB1CTLW0 |= UCSYNC;        // Put B1 into SPI mode
    UCB1CTLW0 |= UCMST;         // Put into SPI master
                                // Flash memory is slave

    //-- Configure the ports
    P5SEL1 &= ~BIT0;        // P5.0 is MOSI
    P5SEL0 |= BIT0;

    P5SEL1 &= ~BIT1;        // P5.1 is MISO
    P5SEL0 |= BIT1;

    P5SEL1 &= ~BIT2;        // P5.2 is CLK
    P5SEL0 |= BIT2;

    P5SEL1 &= ~BIT3;        // P5.3 is CS1 (Flash 1)
    P5SEL0 |= BIT3;

    P3SEL1 &= ~BIT6;        // P3.7 is CS2 (Flash 2)
    P3SEL0 = BIT6;

    PM5CTL0 &= ~LOCKLPM5;   // Disable power mode
                            // Turn on I/O

    UCB1CTLW0 &= ~UCSWRST;    // Take B1 out of SW Reset

}

void spi_send(uint8_t command)
{
    UCB1TXBUF = command;       // Send x4D out over SPI
}

uint8_t spi_receive()
{
    UCB1IE |= UCRXIE;           // Enable SPI Rx IRQ
    UCB1IFG &= ~UCRXIFG;        // Clear flag
    __enable_interrupt();
    return rxdata;
}

#pragma vector = EUSCI_B1_VECTOR    // Data is in B1 SPI buffer
__interrupt void ISR_EUSCI_B1(void)
{
    rxdata = UCB1RXBUF;
}
