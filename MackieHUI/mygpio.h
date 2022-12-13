#ifndef MYGPIO_H_
#define MYGPIO_H_

#include <stdlib.h>
#include <stdbool.h>
#include "bcm2835.h"
#include <st7789.h>


// pin defines


#define PIN_SHIFT_OUT_RESET 5
#define PIN_SHIFT_OUT_LATCH 6
#define PIN_SHIFT_IN_CS 26
#define PIN_SHIFT_IN_LATCH 19
#define PIN_SHIFT_IN_DATA 20
#define PIN_SHIFT_IN_CLK 16
#define N_SHIFT_REG_OUT  1 // number of shift registers
#define N_SHIFT_REG_IN  1


#define ADS1015_CONVERSION_DELAY    1
#define ADS1115_CONVERSION_DELAY    8


// Kept #defines a bit in line with Adafruit library.

// REGISTERS
#define ADS1X15_REG_CONVERT         0x00
#define ADS1X15_REG_CONFIG          0x01
#define ADS1X15_REG_LOW_THRESHOLD   0x02
#define ADS1X15_REG_HIGH_THRESHOLD  0x03


// CONFIG REGISTER

// BIT 15       Operational Status         // 1 << 15
#define ADS1X15_OS_BUSY             0x0000
#define ADS1X15_OS_NOT_BUSY         0x8000
#define ADS1X15_OS_START_SINGLE     0x8000

// BIT 12-14    read differential
#define ADS1X15_MUX_DIFF_0_1        0x0000
#define ADS1X15_MUX_DIFF_0_3        0x1000
#define ADS1X15_MUX_DIFF_1_3        0x2000
#define ADS1X15_MUX_DIFF_2_3        0x3000
//              read single
#define ADS1X15_READ_0              0x4000  // pin << 12
#define ADS1X15_READ_1              0x5000  // pin = 0..3
#define ADS1X15_READ_2              0x6000
#define ADS1X15_READ_3              0x7000


// BIT 9-11     gain                        // (0..5) << 9
#define ADS1X15_PGA_6_144V          0x0000  // voltage
#define ADS1X15_PGA_4_096V          0x0200  //
#define ADS1X15_PGA_2_048V          0x0400  // default
#define ADS1X15_PGA_1_024V          0x0600
#define ADS1X15_PGA_0_512V          0x0800
#define ADS1X15_PGA_0_256V          0x0A00

// BIT 8        mode                        // 1 << 8
#define ADS1X15_MODE_CONTINUE       0x0000
#define ADS1X15_MODE_SINGLE         0x0100

// BIT 5-7      data rate sample per second  // (0..7) << 5
/*
differs for different devices, check datasheet or readme.md
| data rate | ADS101x | ADS 111x | Notes   |
|:---------:|--------:|---------:|:-------:|
|     0     |   128   |   8      | slowest |
|     1     |   250   |   16     |         |
|     2     |   490   |   32     |         |
|     3     |   920   |   64     |         |
|     4     |   1600  |   128    | default |
|     5     |   2400  |   250    |         |
|     6     |   3300  |   475    |         |
|     7     |   3300  |   860    | fastest |
*/

// BIT 4 comparator modi                    // 1 << 4
#define ADS1X15_COMP_MODE_TRADITIONAL   0x0000
#define ADS1X15_COMP_MODE_WINDOW        0x0010

// BIT 3 ALERT active value                 // 1 << 3
#define ADS1X15_COMP_POL_ACTIV_LOW      0x0000
#define ADS1X15_COMP_POL_ACTIV_HIGH     0x0008

// BIT 2 ALERT latching                     // 1 << 2
#define ADS1X15_COMP_NON_LATCH          0x0000
#define ADS1X15_COMP_LATCH              0x0004

// BIT 0-1 ALERT mode                       // (0..3)
#define ADS1X15_COMP_QUE_1_CONV         0x0000  // trigger alert after 1 convert
#define ADS1X15_COMP_QUE_2_CONV         0x0001  // trigger alert after 2 converts
#define ADS1X15_COMP_QUE_4_CONV         0x0002  // trigger alert after 4 converts
#define ADS1X15_COMP_QUE_NONE           0x0003  // disable comparator


// _CONFIG masks
//
// | bit  | description          |
// |:----:|:---------------------|
// |  0   | # channels           |
// |  1   | -                    |
// |  2   | resolution           |
// |  3   | -                    |
// |  4   | GAIN supported       |
// |  5   | COMPARATOR supported |
// |  6   | -                    |
// |  7   | -                    |
//
#define ADS_CONF_CHAN_1  0x00
#define ADS_CONF_CHAN_4  0x01
#define ADS_CONF_RES_12  0x00
#define ADS_CONF_RES_16  0x04
#define ADS_CONF_NOGAIN  0x00
#define ADS_CONF_GAIN    0x10
#define ADS_CONF_NOCOMP  0x00
#define ADS_CONF_COMP    0x20

#define ADS_ADDR_1 0x48
#define ADS_ADDR_2 0x49


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
    }

    ~Gpio();

    DisplayTFT* displays;
    void updateLeds (void);
    void updateFader (int);
    void shiftOut (int out, int state);
    void shiftWriteNB (char* tx,int len);
    int shiftRead(char* rx = nullptr);
    static const int toggle = 2;
    uint16_t readADC (int channel);

private:
    char rxPrev[N_SHIFT_REG_IN];
    unsigned int adcMin[8],adcMax[8];
    bool gpioInit (void);
    bool calibrateFaders(void);
};


#endif // GPIO_H
