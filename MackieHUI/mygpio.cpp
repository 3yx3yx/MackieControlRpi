#include "mygpio.h"
#include <QDebug>
//channelData channel[8];



bool Gpio::gpioInit(void)
{
    bool stat=0;
    bcm2835_gpio_fsel(21, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_DISPLAY_CS_1, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_DISPLAY_DC, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_DISPLAY_RS, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_SHIFT_IN_CS, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_SHIFT_IN_LATCH, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_SHIFT_IN_CLK, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_SHIFT_IN_DATA, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(PIN_SHIFT_IN_DATA, BCM2835_GPIO_PUD_DOWN);

    bcm2835_gpio_fsel(PIN_SHIFT_OUT_LATCH, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_SHIFT_OUT_RESET, BCM2835_GPIO_FSEL_OUTP);

    stat = bcm2835_spi_begin();
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_8);

    if (!bcm2835_i2c_begin())
    {
        printf("bcm2835_i2c_begin failed. Are you running as root??\n");
        return 0;
    }


    return stat;
}

uint16_t Gpio::readADC(int channel)
{
    uint16_t config = ADS1X15_OS_START_SINGLE;  // bit 15     force wake up if needed
    uint16_t readmode =0;
    uint8_t addr;
    switch (channel)
    {

    default: return 0; break;
    case 1:
        readmode = ADS1X15_READ_0;
        addr = ADS_ADDR_1;
        break;
    case 2:
        readmode = ADS1X15_READ_1;
        addr = ADS_ADDR_1;
        break;
    case 3:
        readmode = ADS1X15_READ_2;
        addr = ADS_ADDR_1;
        break;
    case 4:
        readmode = ADS1X15_READ_3;
        addr = ADS_ADDR_1;
        break;

    case 5:
        readmode = ADS1X15_READ_0;
        addr = ADS_ADDR_2;
        break;
    case 6:
        readmode = ADS1X15_READ_1;
        addr = ADS_ADDR_2;
        break;
    case 7:
        readmode = ADS1X15_READ_2;
        addr = ADS_ADDR_2;
        break;
    case 8:
        readmode = ADS1X15_READ_3;
        addr = ADS_ADDR_2;
        break;

    }

    config |= readmode;                         // bit 12-14
    config |= ADS1X15_PGA_4_096V;                            // bit 9-11
    config |= ADS1X15_MODE_SINGLE;                            // bit 8
    config |= 4<<5;                        // bit 5-7 time
    // bit 4      comparator modi
    config |= ADS1X15_COMP_MODE_TRADITIONAL;
    //bit 3      ALERT active value
    config |= ADS1X15_COMP_POL_ACTIV_LOW;
    config |= ADS1X15_COMP_NON_LATCH;           // bit 2      ALERT latching
    config |= ADS1X15_COMP_QUE_NONE;
    // bit 0..1   ALERT mode
    // _writeRegister(addr, ADS1X15_REG_CONFIG, config);

    bcm2835_i2c_setSlaveAddress(addr);
    bcm2835_i2c_set_baudrate(1000000);

    char buffTx[3] = {};
    buffTx[0] = ADS1X15_REG_CONFIG;
    buffTx[1] = config>> 8;
    buffTx[2] = config & 0xFF;
    char buffRx[2] = {};
    char reg[1] = {ADS1X15_REG_CONVERT};
    bcm2835_i2c_write (buffTx,3);

    bcm2835_i2c_write (reg,1);
    bcm2835_i2c_read(buffRx,2);

    return (buffRx[0]<<8)|(buffRx[1] & 0xFF);
}

void Gpio::shiftOut(int out, int state)
{
    static uint8_t mask[N_SHIFT_REG_OUT] = {0};
    int byte=(int)(out/8);

    if (byte>=N_SHIFT_REG_OUT) {return;}

    switch (state)
    {
    case 0: mask[byte] &= ~((uint8_t)1<<out%8); break;
    case 1: mask[byte] |= (uint8_t)1<<out%8; break;
    case toggle:
        mask[byte] ^= (uint8_t)1<<out%8;
        break;
    default: return; break;
    }

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

void Gpio::shiftWriteNB (char* tx,int len)
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

int Gpio::shiftRead(char* rx)
{
    int event=0;
    const int len = N_SHIFT_REG_IN;
    static int eventPrev =0;
    char rx0[len] = {0};
    if (rx == nullptr) rx = rx0;

    bcm2835_gpio_write(PIN_SHIFT_IN_LATCH, LOW);
    bcm2835_gpio_write(PIN_SHIFT_IN_CS, LOW);
    bcm2835_gpio_write(PIN_SHIFT_IN_LATCH, HIGH);

    for (int byte=0; byte<len; byte++)
    {
        for(int bit=0; bit<9; bit++)
        {

            if(!bcm2835_gpio_lev(PIN_SHIFT_IN_DATA))
            {
                *rx |= ((char)1<<(8-bit));
                event =(8*len) -(8*byte + bit) ;
            }

            bcm2835_gpio_write(PIN_SHIFT_IN_CLK,HIGH);
            bcm2835_gpio_write(PIN_SHIFT_IN_CLK,LOW);
        }
        rx++;
    }
    bcm2835_gpio_write(PIN_SHIFT_IN_CS, HIGH);

    if (eventPrev == event) return 0;
    eventPrev = event;
    return event;
}

