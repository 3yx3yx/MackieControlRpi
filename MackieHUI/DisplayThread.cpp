#include <stdint.h>
#include "DisplayThread.h"
#include "QDebug"
DisplayThread::DisplayThread(Gpio *gpioInstance) {
    _gpio = gpioInstance;

    pixels.reserve(240*240*8);

    // init the lcds:
    for (int i=0; i<8; i++)
    {
        _gpio->SPIshiftOut(i,1);
    }

    disp = new DisplayTFT;
}

DisplayThread::~DisplayThread()
{
    delete disp;
}



void DisplayThread::update ()
{
    for (auto pix : pixels)
    {
        setCurrentDisplay(pix.display);
        disp->DrawPixel(pix.x,pix.y,pix.color);
    }
    pixels.clear();
}

void DisplayThread::setCurrentDisplay(int display)
{
    static int prevDisp=-1;
    if (display == prevDisp)
    {
        return;
    }
    _gpio->SPIshiftReset();
    _gpio->SPIshiftOut(display,1);
}


