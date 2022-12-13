#include <QDebug>
#include "MidiCallbacks.h"
#define VPOT_MSG 176

RtMidiIn::RtMidiCallback cbkTable[7] = {
    &inputCallbackRedirect1,
    &inputCallbackRedirect2,
    &inputCallbackRedirect3,
    &inputCallbackRedirect4,
    &inputCallbackRedirect5,
    &inputCallbackRedirect6,
    &inputCallbackRedirect7
};

QString uartTXbuff="";

QString convertToRawMidi(std::vector<unsigned char>* message, int port)
{
    QString s;

    return s;
}




void inputCallbackRedirect1(double deltatime, std::vector< unsigned char > *message, void *)
{
    uartTXbuff.append(convertToRawMidi(message,1));
    qDebug()<<"callback from port"<<2<<deltatime;   
}
void inputCallbackRedirect2(double deltatime, std::vector< unsigned char > *message, void *)
{
    convertToRawMidi (message,2);
    qDebug()<<"callback from port"<<3<<deltatime;
}
void inputCallbackRedirect3(double deltatime, std::vector< unsigned char > *message, void *)
{
    convertToRawMidi(message,3);
    qDebug()<<"callback from port"<<4<<deltatime;
}
void inputCallbackRedirect4(double deltatime, std::vector< unsigned char > *message, void *)
{
    convertToRawMidi (message,4);
    qDebug()<<"callback from port"<<5<<deltatime;
}
void inputCallbackRedirect5(double deltatime, std::vector< unsigned char > *message, void *)
{
    convertToRawMidi (message,5);
    qDebug()<<"callback from port"<<6<<deltatime;
}
void inputCallbackRedirect6(double deltatime, std::vector< unsigned char > *message, void *)
{
    convertToRawMidi (message,6);
    qDebug()<<"callback from port"<<7<<deltatime;
}
void inputCallbackRedirect7(double deltatime, std::vector< unsigned char > *message, void *)
{
    convertToRawMidi (message,7);
    qDebug()<<"callback from port"<<8<<deltatime;
}
