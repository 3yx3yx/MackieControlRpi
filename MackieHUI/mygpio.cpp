#include "mygpio.h"
#include <QDebug>
#include <unistd.h>

int startup_key_combo_detected = 0;

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

    timer = new QTimer(this);
    for(int i = 0; i<N_CHANNELS; i++)
    {
        for (int j = 0; j<4; j++)
        {
            shiftWriteToBuf(i,j,1);
            shiftLoadToRegisters();
            usleep(25000);
            shiftWriteToBuf(i,j,0);
            shiftLoadToRegisters();
        }
    }



    char rx[8] = {};
    shiftRead(rx);
    if (rx[0] == 0x0F)  startup_key_combo_detected = 1;
    if (rx[0] == 0x0E)  startup_key_combo_detected = 2;

    if (startup_key_combo_detected)
    {
        for (int i = 0; i<8; i++)
        {
            shiftWriteToBuf(i,0,1);
            shiftLoadToRegisters();
            return 0;
        }
    }
#ifdef TEST
    adcMin[0] = 15;
    adcMax[0] = 24500;
#else
    calibrateFaders();
#endif
    connect(timer, SIGNAL(timeout()),this, SLOT(update()));
    timer->setSingleShot(1);
    timer->start(30);

    return stat;
}

void Gpio::calibrateFaders()
{   
    qDebug()<<"calibrating faders";
    for (int i=0; i<N_CHANNELS; i++) setFader(i,faderMax);
    do
    {
        qDebug()<<"calibrating faders max";
        update();
        usleep(1500*1000);
    }while (faderTouchEvent);

    for (int i=0; i<N_CHANNELS; i++) stopMotor(i);
    usleep(200*1000);

    for (int i=0; i<N_CHANNELS; i++)
    {
        adcMax[i] = readADC(i);
        qDebug()<<"adc max "<< adcMax[i];
        setFader(i,faderMin);
    }

    do
    {
        qDebug()<<"calibrating faders min";
        update();
        usleep(1500*1000);
    }while (faderTouchEvent);

    for (int i=0; i<N_CHANNELS; i++) stopMotor(i);

    usleep(200*1000);

    for (int i=0; i<N_CHANNELS; i++)
    {
        adcMin[i] = readADC(i);
        qDebug()<<"adc min "<< adcMin[i];
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
    case 0:readmode = ADS1X15_READ_0;addr = ADS_ADDR_1; break;
    case 1:readmode = ADS1X15_READ_1;addr = ADS_ADDR_1; break;
    case 2:readmode = ADS1X15_READ_2;addr = ADS_ADDR_1; break;
    case 3:readmode = ADS1X15_READ_3;addr = ADS_ADDR_1; break;
    case 4:readmode = ADS1X15_READ_0;addr = ADS_ADDR_2; break;
    case 5:readmode = ADS1X15_READ_1;addr = ADS_ADDR_2; break;
    case 6:readmode = ADS1X15_READ_2;addr = ADS_ADDR_2; break;
    case 7:readmode = ADS1X15_READ_3;addr = ADS_ADDR_2; break;
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

void Gpio::stopMotor(int channel)
{
    shiftWriteToBuf(channel, 5, 0);
    shiftWriteToBuf(channel, 6, 0);
    shiftLoadToRegisters();
    //qDebug()<< "stopped motor "<< channel;
}

void Gpio::motorUp(int channel)
{
    shiftWriteToBuf(channel, 6, 0);
    shiftWriteToBuf(channel, 5, 1);
    shiftLoadToRegisters();
    //qDebug()<< "moving motor up "<< channel;

}

void Gpio::motorDown(int channel)
{
    shiftWriteToBuf(channel, 5, 0);
    shiftWriteToBuf(channel, 6, 1);
    shiftLoadToRegisters();
    //qDebug()<< "moving motor down "<< channel;
}

void Gpio::shiftWriteToBuf(int byte, int bit, int state)
{
    if (byte>=N_SHIFT_REG_OUT || bit> 7) {return;}

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
    for (int byte=N_SHIFT_REG_OUT-1; byte>=0; byte--)
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
            usleep(1);
            bcm2835_gpio_write(PIN_SHIFT_OUT_CLK,LOW);
        }
    }
    bcm2835_gpio_write(PIN_SHIFT_OUT_LATCH, LOW);
    usleep(10);
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
                rx[byte] |= ((char)1<<(bit-1));
                event =(8*len) -(8*byte + bit) ;
            }
            bcm2835_gpio_write(PIN_SHIFT_IN_CLK,HIGH);
            usleep(1);
            bcm2835_gpio_write(PIN_SHIFT_IN_CLK,LOW);
        }
    }
    bcm2835_gpio_write(PIN_SHIFT_IN_CS, HIGH);


    return event;
}


int Gpio::getFaderPos(int channel, int adc)
{
    if (adc< adcMin[channel]) adc = adcMin[channel];

    int pos =
            (faderMax- faderMin)
            *(adc-adcMin[channel])
            /(adcMax[channel]-adcMin[channel])
            +faderMin;

    if (pos> faderMax) pos = faderMax;

    return pos;
}

void Gpio::buttonPressedEmmiter(int channel, int type)
{
    qDebug()<< "btn pressed "<< channel << type;
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
    QMutexLocker locker (&mutexAdc);
    adcTarget[channel] =
            (adcMax[channel] - adcMin[channel])
            *(pos-0)
            /(faderMax - faderMin)
            +adcMin[channel];
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
    static const int faderTouch = 4;
    char shiftReadBuf[8] = {0};
    static char shiftReadBufPrev[8] = {0};
    faderTouchEvent = 0;

    shiftRead(shiftReadBuf);

    for (int channel = 0; channel < N_CHANNELS; channel++)
    {
        uint16_t adc = readADC(channel);
        QMutexLocker locker (&mutexAdc);
        uint16_t target = adcTarget[channel];
        locker.unlock();
        bool faderIsTouched=0;

        for(int type = 0; type<8; type++)
        {
            bool stateNow = shiftReadBuf[channel] & (uint8_t)1<<(7-type);
            bool statePrev = shiftReadBufPrev[channel] & (uint8_t)1<<(7-type);

            if ((stateNow==statePrev && type!=faderTouch)
                    || !stateNow)
            {
                continue;
            }

            //qDebug()<<"type "<<type<< "state now "<< stateNow<<"state prev "<< statePrev;

            if (type!=faderTouch)
            {
                buttonPressedEmmiter(channel, type);
            }
            else
            {
                emit faderMoved(channel, getFaderPos(channel, adc));
                stopMotor(channel);
                shiftWriteToBuf(channel, faderTouch, 1);
                faderIsTouched = 1;
                faderTouchEvent = 1;
            }
        }

        shiftReadBufPrev[channel] = shiftReadBuf[channel] ;


        if (faderIsTouched)
        {
            continue;
        }
        else
        {
            shiftWriteToBuf(channel, faderTouch, 0);
        }


        if (adc>= target-400 && adc<= target+400)
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
    for (int i=0; i<N_SHIFT_SPI; i++)
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

