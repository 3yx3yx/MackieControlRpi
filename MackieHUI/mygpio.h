#ifndef MYGPIO_H_
#define MYGPIO_H_

#include <stdlib.h>
#include <stdbool.h>
#include "bcm2835.h"
#include <st7789.h>


// pin defines


#define PIN_SHIFT_OUT_RESET 5
#define PIN_SHIFT_OUT_LATCH 6
#define PIN_SHIFT_IN_CS 13
#define PIN_SHIFT_IN_LATCH 19

#define N_SHIFT_REG_OUT  1 // number of shift registers
#define N_SHIFT_REG_IN  1


typedef struct
{
    bool mute;
    bool recArmed;
    bool solo;
    bool selected;
    bool faderTouched;

    int vuMeter;

    float adcValue;
    int faderPosition;
    char name[6];

}channelData;

class Gpio {
public:
    Gpio(){
        gpioInit();
        char tx[N_SHIFT_REG_OUT]={0};
        shiftWriteNB(tx,N_SHIFT_REG_OUT);
        calibrateFaders();
        displays = new DisplayTFT;
    }
    ~Gpio();

    DisplayTFT* displays;
    void updateLeds (void);
    void updateFader (int);
    void shiftOut (int out, bool state);
    void shiftWriteNB (char* tx,int len);
    bool shiftRead(char* rx, int len);

private:
    unsigned int adcMin[8],adcMax[8];
    bool gpioInit (void);
    bool calibrateFaders(void);
};


#endif // GPIO_H
