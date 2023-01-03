#ifndef DISPLAYTHREAD_H
#define DISPLAYTHREAD_H
#include <QThread>
#include "st7789.h"
#include <cstring>
#include <memory.h>
#include <QRect>
#include <mygpio.h>

class DisplayThread : public QThread
{
    Q_OBJECT
public:
    DisplayThread(Gpio* gpioInstance);
    ~DisplayThread();

    DisplayTFT* disp;

    typedef struct
    {
        uint8_t x;
        uint8_t y;
        uint16_t color;
        uint8_t display;
    } pixel;

    std::vector<pixel> pixels;

    void run() override
    {
      update ();
    }
signals:
    void ready();
private:
    Gpio* _gpio;
    void update ();


    void setCurrentDisplay(int display);
};

#endif // DISPLAYTHREAD_H
