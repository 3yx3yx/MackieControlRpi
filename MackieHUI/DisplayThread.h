#ifndef DISPLAYTHREAD_H
#define DISPLAYTHREAD_H
#include <QThread>
#include "st7789.h"
#include <cstring>
#include <memory.h>
class DisplayThread : public QThread
{
public:
    DisplayThread(){
        display = new DisplayTFT;
        display->Init(240,240);
        memset(frameBuffer, WHITE, sizeof(frameBuffer));
    }
    ~DisplayThread(){
        delete display;
    }
    DisplayTFT* display;
    uint16_t frameBuffer[8][240*240] = {};


    void update (int display=1, uint16_t* data = {});
    void run(){
        update ();
    }
};

#endif // DISPLAYTHREAD_H
