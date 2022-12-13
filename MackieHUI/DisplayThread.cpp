#include <stdint.h>
#include "DisplayThread.h"
#include "QDebug"
void DisplayThread::update (int display)
{
    int x0=0;
    int y0=0;
    int w=0;
    int h=0;

    disp->SetCurrentDisplay(display);

    updateRectPrev.getRect(&x0,&y0,&w,&h);

    disp->DrawImage((*frameBufferPrev)[display],x0,y0,w,h);

   // qDebug()<<"upd rect"<<x0<<y0<<w<<h;
}
