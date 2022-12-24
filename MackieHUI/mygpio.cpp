#include "mygpio.h"
#include <QDebug>




bool Gpio::gpioInit(void)
{
    bool stat=0;

    bcm2835_gpio_fsel(SPI_SHIFT_LATCH, BCM2835_GPIO_FSEL_OUTP);

    bcm2835_gpio_fsel(PIN_SHIFT_OUT_DATA, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_SHIFT_OUT_CLK, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_SHIFT_OUT_LATCH, BCM2835_GPIO_FSEL_OUTP);

    bcm2835_gpio_fsel(PIN_DISPLAY_DC, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_DISPLAY_RS, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_DISPLAY_CS, BCM2835_GPIO_FSEL_OUTP);

    bcm2835_gpio_fsel(PIN_SHIFT_IN_CS, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_SHIFT_IN_LATCH, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PIN_SHIFT_IN_CLK, BCM2835_GPIO_FSEL_OUTP);

    bcm2835_gpio_fsel(PIN_SHIFT_IN_DATA, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(PIN_SHIFT_IN_DATA, BCM2835_GPIO_PUD_DOWN);

    stat = bcm2835_spi_begin();
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_8);

    if (!bcm2835_i2c_begin())
    {
        printf("bcm2835_i2c_begin failed. Are you running as root??\n");
        return 0;
    }
    bcm2835_i2c_set_baudrate(1000000);

    calibrateFaders();

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()),this, SLOT(update()));
    timer->setSingleShot(1);
    timer->start(30);

    return stat;
}

void Gpio::calibrateFaders() // this function is recursive
{
    int adcPrev[8]={INT_MAX};
    const int error_rate = 10;

    static bool down = 1;

    for (int i = 0; i< 8; i++)
    {
        if (down)
        {
            motorDown(i);
        }
        else
        {
            motorUp(i);
            adcPrev[i] = 0;
        }

    }
    bool ready[8] = {0};
    bool complete = 0;
    while (!complete)
    {
        for (int i = 0; i< 8; i++)
        {
            if (ready[i])
            {
                continue;
            }
            int adc = readADC(i);
            if (down)
            {
                if (adc >= adcPrev[i] - error_rate)
                {
                    stopMotor(i);
                    ready[i] = 1;
                    adcMin[i]=adc;
                }
            }
            else
            {
                if (adc <= adcPrev[i] + error_rate)
                {
                    stopMotor(i);
                    ready[i] = 1;
                    adcMax[i]=adc;
                }
            }

        }

        complete = 1;

        for (int i=1; i<8; i++)
        {
            if(ready[i]&&ready[i-1])
            {
                continue;
            }
            else
            {
                complete = 0;
                break;
            }
        }

    }

    if (down)
    {
        down = 0;
        calibrateFaders();
    }

}

uint16_t Gpio::readADC(int channel)
{
    uint16_t config = ADS1X15_OS_START_SINGLE;  // bit 15     force wake up if needed
    uint16_t readmode =0;
    uint8_t addr;
    switch (channel)
    {
    default: return 0; break;
    case 1:readmode = ADS1X15_READ_0;addr = ADS_ADDR_1; break;
    case 2:readmode = ADS1X15_READ_1;addr = ADS_ADDR_1; break;
    case 3:readmode = ADS1X15_READ_2;addr = ADS_ADDR_1; break;
    case 4:readmode = ADS1X15_READ_3;addr = ADS_ADDR_1; break;
    case 5:readmode = ADS1X15_READ_0;addr = ADS_ADDR_2; break;
    case 6:readmode = ADS1X15_READ_1;addr = ADS_ADDR_2; break;
    case 7:readmode = ADS1X15_READ_2;addr = ADS_ADDR_2; break;
    case 8:readmode = ADS1X15_READ_3;addr = ADS_ADDR_2; break;
    }

    config |= readmode;                         // bit 12-14
    config |= ADS1X15_PGA_4_096V;                            // bit 9-11
    config |= ADS1X15_MODE_SINGLE;                            // bit 8
    config |= 4<<5;                        // bit 5-7 time
    config |= ADS1X15_COMP_MODE_TRADITIONAL;// bit 4      comparator modi
    config |= ADS1X15_COMP_POL_ACTIV_LOW;//bit 3      ALERT active value
    config |= ADS1X15_COMP_NON_LATCH;           // bit 2      ALERT latching
    config |= ADS1X15_COMP_QUE_NONE; // bit 0..1   ALERT mode

    bcm2835_i2c_setSlaveAddress(addr);

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

void Gpio::stopMotor(int channel)
{
    shiftWriteToBuf(channel, 5, 0);
    shiftWriteToBuf(channel, 6, 0);
    shiftLoadToRegisters();
}

void Gpio::motorUp(int channel)
{
    shiftWriteToBuf(channel, 5, 0);
    shiftLoadToRegisters();
}

void Gpio::motorDown(int channel)
{
    shiftWriteToBuf(channel, 6, 0);
    shiftLoadToRegisters();
}

void Gpio::shiftWriteToBuf(int byte, int bit, int state)
{
    if (byte>=N_SHIFT_REG_OUT || bit> 8) {return;}

    lockShiftReg.lockForWrite();

    switch (state)
    {
    case 0: ShiftBuffer[byte] &= ~((uint8_t)1<<bit); break;
    case 1: ShiftBuffer[byte] |= (uint8_t)1<<bit; break;
    case shiftToggle:
        ShiftBuffer[byte] ^= (uint8_t)1<<bit;
        break;
    default: return; break;
    }

    lockShiftReg.unlock();
}

void Gpio::shiftLoadToRegisters()
{
    lockShiftReg.lockForRead();

    bcm2835_gpio_write(PIN_SHIFT_OUT_LATCH, HIGH);
    for (int byte=0; byte<N_SHIFT_REG_OUT; byte++)
    {
        for(int bit=1; bit<9; bit++)
        {
            if (ShiftBuffer[byte] & ((char)1<<(8-bit)))
            {
                bcm2835_gpio_write(PIN_SHIFT_OUT_DATA,HIGH);
            }
            else
            {
                bcm2835_gpio_write(PIN_SHIFT_OUT_DATA,LOW);
            }
            bcm2835_gpio_write(PIN_SHIFT_OUT_CLK,HIGH);
            bcm2835_gpio_write(PIN_SHIFT_OUT_CLK,LOW);
        }
    }
    bcm2835_gpio_write(PIN_SHIFT_OUT_LATCH, LOW);
    bcm2835_gpio_write(PIN_SHIFT_OUT_LATCH, HIGH);

    lockShiftReg.unlock();
}

int Gpio::shiftRead(char* rx)
{
    int event=0;
    const int len = N_SHIFT_REG_IN;

    char rx0[len] = {0};
    if (rx == nullptr) rx = rx0;

    bcm2835_gpio_write(PIN_SHIFT_IN_LATCH, LOW);
    bcm2835_gpio_write(PIN_SHIFT_IN_CS, LOW);
    bcm2835_gpio_write(PIN_SHIFT_IN_LATCH, HIGH);

    for (int byte=0; byte<len; byte++)
    {
        for(int bit=1; bit<9; bit++)
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


    return event;
}


int Gpio::getFaderPos(int channel, int adc)
{
    int pos =
            (faderMax- faderMin)
            *(adc-adcMin[channel])
            /(adcMax[channel]-adcMin[channel])
            +0;
    pos = std::abs(pos);
    return pos;
}

void Gpio::buttonPressedEmmiter(int channel, int type)
{
    switch (type)
    {
    case 0: emit recArmPressed(channel); break;
    case 1: emit soloPressed(channel); break;
    case 2: emit mutePressed(channel); break;
    case 3: emit selectPressed(channel); break;
    }
}

void Gpio::setFader(int channel, int pos)
{
    lockTargetADC.lockForWrite();

    adcTarget[channel] =
            (adcMax[channel] - adcMin[channel])
            *(pos-0)
            /(faderMax - faderMin)
            +adcMin[channel];

    lockTargetADC.unlock();
}



void Gpio::onRecArmed(int channel, bool state)
{
    shiftWriteToBuf(channel, 0, state);
}

void Gpio::onSoloed(int channel, bool state)
{
    shiftWriteToBuf(channel, 1, state);
}

void Gpio::onMuted(int channel, bool state)
{
    shiftWriteToBuf(channel, 2, state);
}

void Gpio::onSelected(int channel, bool state)
{
    shiftWriteToBuf(channel, 3, state);
}

void Gpio::update()
{
    const int faderTouch = 4;
    char shiftReadBuf[8];
    static char shiftReadBufPrev[8];

    shiftRead(shiftReadBuf);

    for (int channel = 0; channel < 8; channel++)
    {
        uint16_t adc = readADC(channel);
        lockTargetADC.lockForRead();
        uint16_t target = adcTarget[channel];
        lockTargetADC.unlock();
        bool faderIsTouched=0;

        for(int type = 0; type<5; type++)
        {
            bool stateNow = shiftReadBuf[channel] & (char)1<<(7-type);
            bool statePrev = shiftReadBufPrev[channel] & (char)1<<(7-type);
            if (stateNow==statePrev && type!=faderTouch)
            {
                continue;
            }

            if (type!=faderTouch)
            {
                buttonPressedEmmiter(channel, type);
                shiftWriteToBuf(channel, faderTouch, 0);
            }
            else
            {
                emit faderMoved(channel, getFaderPos(channel, adc));
                stopMotor(channel);
                shiftWriteToBuf(channel, faderTouch, 1);
                faderIsTouched = 1;
            }
        }

        shiftReadBufPrev[channel] = shiftReadBuf[channel] ;

        if (faderIsTouched)
        {
            continue;
        }

        if (adc>= target-30 && adc<= target+30)
        {
            stopMotor(channel);
        }
        else
            if(adc> target)
            {
                motorDown(channel);
            }
            else
                if(adc<target)
                {
                    motorUp(channel);
                }
    }

    shiftLoadToRegisters();
    timer->start(30);
}

void Gpio::SPIshiftOut(int out, int state)
{
    static uint8_t mask[N_SHIFT_SPI] = {0};
    int byte=(int)(out/8);

    if (byte>=N_SHIFT_SPI) {return;}

    if (out<0 || state<0)
    {
        for (int i=0; i<N_SHIFT_SPI; ++i)
        {
            mask[i]=0;
        }
    }

    switch (state)
    {
    case 0: mask[byte] &= ~((uint8_t)1<<out%8); break;
    case 1: mask[byte] |= (uint8_t)1<<out%8; break;
    case shiftToggle:
        mask[byte] ^= (uint8_t)1<<out%8;
        break;
    default: return; break;
    }


    bcm2835_gpio_write(SPI_SHIFT_LATCH, HIGH);
    for (int i=0; i<N_SHIFT_REG_OUT; i++)
    {
        bcm2835_spi_transfer(mask[i]);
    }
    bcm2835_gpio_write(SPI_SHIFT_LATCH, LOW);
    bcm2835_gpio_write(SPI_SHIFT_LATCH, HIGH);
}

void Gpio::SPIshiftReset()
{
    SPIshiftOut(-1,-1);
}

