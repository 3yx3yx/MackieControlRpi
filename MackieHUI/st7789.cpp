#include "mygpio.h"
#include <st7789.h>
#include <QDebug>
#include <stdlib.h>

void DisplayTFT::Init(int Width, int Height)
{
  ST7789_Width = Width;
  ST7789_Height = Height;
  qDebug()<<"init LCD";
  ST7789_HardReset();
  ST7789_SoftReset();
  ST7789_SleepModeExit();

  ST7789_ColorModeSet(ST7789_ColorMode_65K | ST7789_ColorMode_16bit);
 // bcm2835_delay(10);
  ST7789_MemAccessModeSet(4, 1, 1, 0);
//  bcm2835_delay(10);
  ST7789_InversionMode(1);
//  bcm2835_delay(10);
 // FillScreen(GREEN);
  ST7789_DisplayPower(1);
//  bcm2835_delay(10);
}

void DisplayTFT::ST7789_HardReset(void)
{
    bcm2835_gpio_write(PIN_DISPLAY_RS,LOW);
    bcm2835_delay(10);
    bcm2835_gpio_write(PIN_DISPLAY_RS,HIGH);
    bcm2835_delay(150);

}

void DisplayTFT::ST7789_SoftReset(void)
{
  ST7789_SendCmd(ST7789_Cmd_SWRESET);
  bcm2835_delay(130);
}

inline void DisplayTFT::ST7789_SendCmd(uint8_t Cmd)
{
    uint8_t cs = CS_current;
    bcm2835_gpio_write(cs,HIGH);
    bcm2835_gpio_write(PIN_DISPLAY_DC,LOW);
    bcm2835_gpio_write(cs,LOW);
    bcm2835_spi_transfer(Cmd);
    bcm2835_gpio_write(cs,HIGH);
}

inline void DisplayTFT::ST7789_SendData(uint8_t Data)
{
    uint8_t cs = CS_current;
    bcm2835_gpio_write(cs,HIGH);
    bcm2835_gpio_write(PIN_DISPLAY_DC,HIGH);
    bcm2835_gpio_write(cs,LOW);
    bcm2835_spi_transfer(Data);
    bcm2835_gpio_write(cs,HIGH);
}

void DisplayTFT::ST7789_SleepModeEnter( void )
{
    ST7789_SendCmd(ST7789_Cmd_SLPIN);
  bcm2835_delay(500);
}

void DisplayTFT::ST7789_SleepModeExit( void )
{
    ST7789_SendCmd(ST7789_Cmd_SLPOUT);
  bcm2835_delay(500);
}


void DisplayTFT::ST7789_ColorModeSet(uint8_t ColorMode)
{
  ST7789_SendCmd(ST7789_Cmd_COLMOD);
  ST7789_SendData(ColorMode & 0x77);
}

void DisplayTFT::ST7789_MemAccessModeSet(uint8_t Rotation, uint8_t VertMirror, uint8_t HorizMirror, uint8_t IsBGR)
{
  uint8_t Value;
  Rotation &= 7;

  ST7789_SendCmd(ST7789_Cmd_MADCTL);

  switch (Rotation)
  {
  case 0:
    Value = 0;
    break;
  case 1:
    Value = ST7789_MADCTL_MX;
    break;
  case 2:
    Value = ST7789_MADCTL_MY;
    break;
  case 3:
    Value = ST7789_MADCTL_MX | ST7789_MADCTL_MY;
    break;
  case 4:
    Value = ST7789_MADCTL_MV;
    break;
  case 5:
    Value = ST7789_MADCTL_MV | ST7789_MADCTL_MX;
    break;
  case 6:
    Value = ST7789_MADCTL_MV | ST7789_MADCTL_MY;
    break;
  case 7:
    Value = ST7789_MADCTL_MV | ST7789_MADCTL_MX | ST7789_MADCTL_MY;
    break;
  }

  if (VertMirror)
    Value = ST7789_MADCTL_ML;
  if (HorizMirror)
    Value = ST7789_MADCTL_MH;

  if (IsBGR)
    Value |= ST7789_MADCTL_BGR;

  ST7789_SendData(Value);
}

void DisplayTFT::ST7789_InversionMode(uint8_t Mode)
{
  if (Mode)
    ST7789_SendCmd(ST7789_Cmd_INVON);
  else
    ST7789_SendCmd(ST7789_Cmd_INVOFF);
}

void DisplayTFT::FillScreen(uint16_t color)
{
  FillRect(0, 0,  ST7789_Width, ST7789_Height, color);
}

void DisplayTFT::FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  if ((x >= ST7789_Width) || (y >= ST7789_Height)) return;
  if ((x + w) > ST7789_Width) w = ST7789_Width - x;
  if ((y + h) > ST7789_Height) h = ST7789_Height - y;
  SetWindow(x, y, x + w - 1, y + h - 1);
  for (uint32_t i = 0; i < (uint32_t)(h * w); i++) RamWrite(&color, 1);
}

void DisplayTFT::SetWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
  ST7789_ColumnSet(x0, x1);
  ST7789_RowSet(y0, y1);
  ST7789_SendCmd(ST7789_Cmd_RAMWR);
}

inline void DisplayTFT::RamWrite(uint16_t *pBuff, uint16_t Len)
{
  while (Len--)
  {
    ST7789_SendData(*pBuff >> 8);
    ST7789_SendData(*pBuff & 0xFF);
  }
}
// with default values draws full screen image 240x240 px
void DisplayTFT::DrawImage(uint16_t* fbuff,int x0,int y0,int w, int h)
{
    uint8_t cs = CS_current;
    SetWindow(x0,y0,w+x0-1,y0+h-1);
    bcm2835_gpio_write(cs,HIGH);
    bcm2835_gpio_write(PIN_DISPLAY_DC,HIGH);
    bcm2835_gpio_write(cs,LOW);
    spiWrite(fbuff,w*h);
    bcm2835_gpio_write(cs,HIGH);
}



void DisplayTFT::spiWrite (uint16_t* tbuf, uint32_t len)
{
    while (len--)
    {
        ST7789_SendData(*tbuf >> 8);
        ST7789_SendData(*tbuf & 0xFF);
        tbuf++;
    }
}
void DisplayTFT::ST7789_ColumnSet(uint16_t ColumnStart, uint16_t ColumnEnd)
{
  if (ColumnStart > ColumnEnd)
    return;
  if (ColumnEnd > ST7789_Width)
    return;

  ColumnStart += ST7789_X_Start;
  ColumnEnd += ST7789_X_Start;

  ST7789_SendCmd(ST7789_Cmd_CASET);
  ST7789_SendData(ColumnStart >> 8);
  ST7789_SendData(ColumnStart & 0xFF);
  ST7789_SendData(ColumnEnd >> 8);
  ST7789_SendData(ColumnEnd & 0xFF);
}

void DisplayTFT::ST7789_RowSet(uint16_t RowStart, uint16_t RowEnd)
{
  if (RowStart > RowEnd)
    return;
  if (RowEnd > ST7789_Height)
    return;

  RowStart += ST7789_Y_Start;
  RowEnd += ST7789_Y_Start;

  ST7789_SendCmd(ST7789_Cmd_RASET);
  ST7789_SendData(RowStart >> 8);
  ST7789_SendData(RowStart & 0xFF);
  ST7789_SendData(RowEnd >> 8);
  ST7789_SendData(RowEnd & 0xFF);
}

void DisplayTFT::ST7789_SetBL(uint8_t Value)
{
  if (Value > 100)
    Value = 100;

#if (ST77xx_BLK_PWM_Used)

#endif
}

void DisplayTFT::ST7789_DisplayPower(uint8_t On)
{
  if (On)
    ST7789_SendCmd(ST7789_Cmd_DISPON);
  else
    ST7789_SendCmd(ST7789_Cmd_DISPOFF);
}

void DisplayTFT::DrawRectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
  DrawLine(x1, y1, x1, y2, color);
  DrawLine(x2, y1, x2, y2, color);
  DrawLine(x1, y1, x2, y1, color);
  DrawLine(x1, y2, x2, y2, color);
}

void DisplayTFT::DrawRectangleFilled(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t fillcolor)
{
  if (x1 > x2)
    SwapInt16Values(&x1, &x2);
  if (y1 > y2)
    SwapInt16Values(&y1, &y2);
   FillRect(x1, y1, x2 - x1, y2 - y1, fillcolor);
}

void DisplayTFT::DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{

  if (x1 == x2)
  {

    if (y1 > y2)
      FillRect(x1, y2, 1, y1 - y2 + 1, color);
    else
      FillRect(x1, y1, 1, y2 - y1 + 1, color);
    return;
  }


  if (y1 == y2)
  {

    if (x1 > x2)
      FillRect(x2, y1, x1 - x2 + 1, 1, color);
    else
      FillRect(x1, y1, x2 - x1 + 1, 1, color);
    return;
  }

  ST7789_DrawLine_Slow(x1, y1, x2, y2, color);
}

void DisplayTFT::SwapInt16Values(int16_t *pValue1, int16_t *pValue2)
{
  int16_t TempValue = *pValue1;
  *pValue1 = *pValue2;
  *pValue2 = TempValue;
}

void DisplayTFT::ST7789_DrawLine_Slow(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
  const int16_t deltaX = abs(x2 - x1);
  const int16_t deltaY = abs(y2 - y1);
  const int16_t signX = x1 < x2 ? 1 : -1;
  const int16_t signY = y1 < y2 ? 1 : -1;

  int16_t error = deltaX - deltaY;

  DrawPixel(x2, y2, color);

  while (x1 != x2 || y1 != y2)
  {
    DrawPixel(x1, y1, color);
    const int16_t error2 = error * 2;

    if (error2 > -deltaY)
    {
      error -= deltaY;
      x1 += signX;
    }
    if (error2 < deltaX)
    {
      error += deltaX;
      y1 += signY;
    }
  }
}

void DisplayTFT::DrawPixel(int16_t x, int16_t y, uint16_t color)
{
  if ((x < 0) ||(x >= ST7789_Width) || (y < 0) || (y >= ST7789_Height))
    return;

  SetWindow(x, y, x, y);
  RamWrite(&color, 1);
}

void DisplayTFT::DrawCircleFilled(int16_t x0, int16_t y0, int16_t radius, uint16_t fillcolor)
{
  int x = 0;
  int y = radius;
  int delta = 1 - 2 * radius;
  int error = 0;

  while (y >= 0)
  {
    DrawLine(x0 + x, y0 - y, x0 + x, y0 + y, fillcolor);
    DrawLine(x0 - x, y0 - y, x0 - x, y0 + y, fillcolor);
    error = 2 * (delta + y) - 1;

    if (delta < 0 && error <= 0)
    {
      ++x;
      delta += 2 * x + 1;
      continue;
    }

    error = 2 * (delta - x) - 1;

    if (delta > 0 && error > 0)
    {
      --y;
      delta += 1 - 2 * y;
      continue;
    }

    ++x;
    delta += 2 * (x - y);
    --y;
  }
}

void DisplayTFT::DrawCircle(int16_t x0, int16_t y0, int16_t radius, uint16_t color)
{
  int x = 0;
  int y = radius;
  int delta = 1 - 2 * radius;
  int error = 0;

  while (y >= 0)
  {
    DrawPixel(x0 + x, y0 + y, color);
    DrawPixel(x0 + x, y0 - y, color);
    DrawPixel(x0 - x, y0 + y, color);
    DrawPixel(x0 - x, y0 - y, color);
    error = 2 * (delta + y) - 1;

    if (delta < 0 && error <= 0)
    {
      ++x;
      delta += 2 * x + 1;
      continue;
    }

    error = 2 * (delta - x) - 1;

    if (delta > 0 && error > 0)
    {
      --y;
      delta += 1 - 2 * y;
      continue;
    }

    ++x;
    delta += 2 * (x - y);
    --y;
  }
}
