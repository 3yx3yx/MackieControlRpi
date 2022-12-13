#ifndef _ST7789_H
#define _ST7789_H
#include "bcm2835.h"

#include <stdint.h>





#define ST7789_ColorMode_65K    0x50
#define ST7789_ColorMode_262K   0x60
#define ST7789_ColorMode_12bit  0x03
#define ST7789_ColorMode_16bit  0x05
#define ST7789_ColorMode_18bit  0x06
#define ST7789_ColorMode_16M    0x07

#define ST7789_Cmd_SWRESET      0x01
#define ST7789_Cmd_SLPIN        0x10
#define ST7789_Cmd_SLPOUT       0x11
#define ST7789_Cmd_COLMOD       0x3A
#define ST7789_Cmd_PTLON        0x12
#define ST7789_Cmd_NORON        0x13
#define ST7789_Cmd_INVOFF       0x20
#define ST7789_Cmd_INVON        0x21
#define ST7789_Cmd_GAMSET       0x26
#define ST7789_Cmd_DISPOFF      0x28
#define ST7789_Cmd_DISPON       0x29
#define ST7789_Cmd_CASET        0x2A
#define ST7789_Cmd_RASET        0x2B
#define ST7789_Cmd_RAMWR        0x2C
#define ST7789_Cmd_PTLAR        0x30
#define ST7789_Cmd_MADCTL       0x36
#define ST7789_Cmd_MADCTL_MY    0x80
#define ST7789_Cmd_MADCTL_MX    0x40
#define ST7789_Cmd_MADCTL_MV    0x20
#define ST7789_Cmd_MADCTL_ML    0x10
#define ST7789_Cmd_MADCTL_RGB   0x00
#define ST7789_Cmd_RDID1        0xDA
#define ST7789_Cmd_RDID2        0xDB
#define ST7789_Cmd_RDID3        0xDC
#define ST7789_Cmd_RDID4        0xDD
#define ST7789_MADCTL_MY        0x80
#define ST7789_MADCTL_MX        0x40
#define ST7789_MADCTL_MV        0x20
#define ST7789_MADCTL_ML        0x10
#define ST7789_MADCTL_BGR       0x08
#define ST7789_MADCTL_MH        0x04

#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF


#define ST7789_X_Start          0
#define ST7789_Y_Start          0
#define PIN_DISPLAY_CS_1 22
#define PIN_DISPLAY_CS_2 22
#define PIN_DISPLAY_CS_3 22

#define PIN_DISPLAY_CS_4 22
#define PIN_DISPLAY_CS_5 22
#define PIN_DISPLAY_CS_6 22
#define PIN_DISPLAY_CS_7 22
#define PIN_DISPLAY_CS_8 22
#define PIN_DISPLAY_DC 27
#define PIN_DISPLAY_RS 17

class DisplayTFT
{
public:
    DisplayTFT()

    {
//        Init(240,240);
//        for (int i = 0; i<8; i++)
//        {
            //SetCurrentDisplay(i);
            Init(240,240);
//        }
//        bcm2835_delay(100);
    }

    void SetCurrentDisplay (unsigned int display) {CS_current=CS_pins[display];}
    void DrawImage (uint16_t* fbuff={},int x0=0,int y0=0,int w=240, int h=240);
    void FillScreen(uint16_t color);
    void FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void SetWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
    inline void RamWrite(uint16_t *pBuff, uint16_t Len);
    void DrawRectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
    void DrawRectangleFilled(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t fillcolor);
    void DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
    void DrawPixel(int16_t x, int16_t y, uint16_t color);
    void DrawCircleFilled(int16_t x0, int16_t y0, int16_t radius, uint16_t fillcolor);
    void DrawCircle(int16_t x0, int16_t y0, int16_t radius, uint16_t color);


private:
    const unsigned int CS_pins[8]={PIN_DISPLAY_CS_1,PIN_DISPLAY_CS_2,PIN_DISPLAY_CS_3,PIN_DISPLAY_CS_4,
                             PIN_DISPLAY_CS_5,PIN_DISPLAY_CS_6,PIN_DISPLAY_CS_7,PIN_DISPLAY_CS_8,};
    unsigned int CS_current =PIN_DISPLAY_CS_1 ;
    void Init(int Width, int Height);
    void spiWrite (uint16_t* tbuf, uint32_t len);
    uint8_t ST7789_Width, ST7789_Height;
    void ST7789_ColumnSet(uint16_t ColumnStart, uint16_t ColumnEnd);
    void ST7789_RowSet(uint16_t RowStart, uint16_t RowEnd);
    void ST7789_SetBL(uint8_t Value);
    void ST7789_DisplayPower(uint8_t On);
    void ST7789_HardReset(void);
    void ST7789_SoftReset(void);
    inline void ST7789_SendCmd(uint8_t Cmd);
    inline void ST7789_SendData(uint8_t Data);
    void ST7789_SleepModeEnter(void);
    void ST7789_SleepModeExit(void);
    void ST7789_ColorModeSet(uint8_t ColorMode);
    void ST7789_MemAccessModeSet(uint8_t Rotation, uint8_t VertMirror, uint8_t HorizMirror, uint8_t IsBGR);
    void ST7789_InversionMode(uint8_t Mode);
    void SwapInt16Values(int16_t *pValue1, int16_t *pValue2);
    void ST7789_DrawLine_Slow(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
};

#endif
