#include "mygpio.h"

//channelData channel[8];

bool gpioInit(void)
{
    bool stat=0;
    bcm2835_gpio_fsel(21, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_DISPLAY_CS_1, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_DISPLAY_DC, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_DISPLAY_RS, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_SHIFT_IN_CS, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_SHIFT_IN_LATCH, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_SHIFT_OUT_LATCH, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_SHIFT_OUT_RESET, BCM2835_GPIO_FSEL_OUTP);

    stat = bcm2835_spi_begin();
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_8);
    bcm2835_gpio_write(PIN_DISPLAY_CS_1,HIGH);

    return stat;
}

void shiftOut(int out, bool state)
{
    static uint8_t mask[N_SHIFT_REG_OUT] = {0};
    int byte=(int)(out/8);

    if (byte>=N_SHIFT_REG_OUT) {return;}

    if(state){mask[byte] |= (uint8_t)1<<out%8;}
    else {mask[byte] ^= (uint8_t)1<<out%8;}

    bcm2835_gpio_write(PIN_SHIFT_OUT_RESET, HIGH);
    bcm2835_gpio_write(PIN_SHIFT_OUT_LATCH, HIGH);
    for (int i=0; i<N_SHIFT_REG_OUT; i++)
    {
        bcm2835_spi_transfer(mask[i]);
    }
    bcm2835_gpio_write(PIN_SHIFT_OUT_LATCH, LOW);
    bcm2835_delayMicroseconds(1);
    bcm2835_gpio_write(PIN_SHIFT_OUT_LATCH, HIGH);
}

void shiftWriteNB (char* tx,int len)
{
    if(len>N_SHIFT_REG_OUT) {return;}
    bcm2835_gpio_write(PIN_SHIFT_OUT_RESET, HIGH);
    bcm2835_gpio_write(PIN_SHIFT_OUT_LATCH, HIGH);

    char rx[N_SHIFT_REG_IN]={0};
    bcm2835_spi_transfernb(tx,rx,N_SHIFT_REG_OUT);

    bcm2835_gpio_write(PIN_SHIFT_OUT_LATCH, LOW);
    bcm2835_delayMicroseconds(1);
    bcm2835_gpio_write(PIN_SHIFT_OUT_LATCH, HIGH);
}

bool shiftRead(char* rx, int len)
{
    bool event=0;
    if(len>N_SHIFT_REG_IN) {return 0;}

    bcm2835_gpio_write(PIN_SHIFT_IN_CS, HIGH);
    bcm2835_gpio_write(PIN_SHIFT_IN_LATCH, LOW);
    bcm2835_delayMicroseconds(1);
    bcm2835_gpio_write(PIN_SHIFT_IN_CS, LOW);
    bcm2835_gpio_write(PIN_SHIFT_IN_LATCH, HIGH);

    char tx[N_SHIFT_REG_IN]={0};
    bcm2835_spi_transfernb(tx,rx,N_SHIFT_REG_IN);
    for (int i=0; i<N_SHIFT_REG_IN; i++)
    {
        if(rx[i]!=0) {
            event=1;
            break;
        }
    }
    return event;
}

