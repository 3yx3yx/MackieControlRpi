#ifndef DISPLAYTHREAD_H
#define DISPLAYTHREAD_H
#include <QThread>
#include "st7789.h"
#include <cstring>
#include <memory.h>
#include <QRect>

class DisplayThread : public QThread
{
    Q_OBJECT
public:
    DisplayThread() {
        disp = new DisplayTFT;       
        updateRectNext.setRect(0,0,240,240);
        updateRectPrev.setRect(0,0,240,240);
    }
    ~DisplayThread(){
        delete disp;
    }
    DisplayTFT* disp;
    uint16_t (*frameBufferNext)[8][240*240];
    uint16_t (*frameBufferPrev)[8][240*240];
    uint16_t frameBuffer1[8][240*240] = {};
    uint16_t frameBuffer2[8][240*240] = {};

    QRect updateRectNext;
    QRect updateRectPrev;
    void update (int display=1);
    void run() override{
        update ();
    }

private:
    QRect _rect;
};

#endif // DISPLAYTHREAD_H
