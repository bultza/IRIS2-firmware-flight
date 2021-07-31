#include <spi.h>

uint8_t rxdata;

void spi_init(uint8_t clockrate)
{

    //-- Setup SPI B1

    UCB1CTLW0 = UCSWRST; //UCB1CTLW0 |= UCSWRST;       // Put B1 into SW Reset

    //UCB1CTLW0 |= UCSSEL__SMCLK; // Choose SMCLK
    //UCB1BRW = clockrate;        // Set prescaler

    //UCB1CTLW0 |= UCSYNC;        // Put B1 into SPI mode
    //UCB1CTLW0 |= UCMST;         // Put into SPI master

    // 3-pin, 8-bit SPI master
    UCB1CTLW0 |= UCMST | UCSYNC | UCCKPH | UCMSB;
    // Clock polarity high, MSB

    // SMCLK
    UCB1CTLW0 |= UCSSEL__SMCLK;

    UCB1BRW = clockrate;
    // No modulation

    //-- Configure the ports
    P5SEL1 &= ~(BIT0 | BIT1 | BIT2);
    P5SEL0 |= (BIT0 | BIT1 | BIT2);

    //CS1
    FLASH_CS1_OFF;
    P5OUT |= BIT3;
    //CS2
    FLASH_CS2_OFF;
    P3OUT |= BIT6;

    UCB1CTLW0 &= ~UCSWRST;    // Take B1 out of SW Reset

}

/**
 * Send a single byte and do not expect any answer
 */
int8_t spi_write_instruction(uint8_t instruction)
{
    // Wait for TXBUF ready
    while (!(UCB1IFG & UCTXIFG));
    UCB1TXBUF = instruction;

    // Wait for TX complete
    while (UCB1STAT & UCBUSY);

    return 0;
}

/**
 * Write x bytes and read x bytes consecutively
 */
int8_t spi_write_read(uint8_t *bufferOut,
                      unsigned int bufferOutLenght,
                      uint8_t *bufferIn,
                      unsigned int bufferInLenght)
{
    //First we send:
    while(bufferOutLenght)
    {
        // Wait for TXBUF ready
        while (!(UCB1IFG & UCTXIFG));
        //Fill buffer
        UCB1TXBUF = *bufferOut;

        bufferOutLenght--;
        if(bufferOutLenght != 0)
            *bufferOut++;
    }
    // Wait for TX complete
    while (UCB1STAT & UCBUSY);

    //Empty the RX buffer:
    uint8_t dummy = UCB1RXBUF;

    //Then we read
    while(bufferInLenght)
    {
        //Send dummy byte to keep the clock running
        UCB1TXBUF = 0;

        // Wait for RX to finish
        while (!(UCB1IFG & UCRXIFG));

        // Store data from last data RX
        *bufferIn = UCB1RXBUF;

        bufferInLenght--;
        if(bufferInLenght != 0)
            *bufferIn++;
    }

    //Return without errors
    return 0;
}


