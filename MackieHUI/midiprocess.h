#ifndef MIDIPROCESS_H
#define MIDIPROCESS_H


#include "RtMidi.h"
#include "qdebug.h"
#include "qthread.h"
#include <QtSerialPort/QSerialPort>



QT_BEGIN_NAMESPACE
namespace midi {
class midiprocess;
}
QT_END_NAMESPACE

struct midiDefs {
    enum { // midi and mackie protocol constants

        vpot = 176,
        /*vpot 3 bytes
     *  176
     *
     *  16..23 corresponding to channel
     *
     *  if up 1
     *  if down 65
     */
        button =144,
        /*button 3 bytes:
     * 144
     *
     * rec arm 0..7
     * solo 8..15
     * mute 16..23
     * select 24..31
     * play 94
     * stop 93
     * record 95
     * backwd 91
     * fwd 92
     *
     * 127 or 0 for on/off
     *
     * faders
     *  224..231
     *  position 1st byte
     *  position 2nd byte
     */
        faderFirst=224,
        vuMeter=208,
        /* vu meter 2 bytes:
     * 208
     * (channel N)*16 + value
     */
        displayBlockStart=240,
        /*the display data is
     * 240 00 102 20 18
     * channelN * 7   0, 7 .. 21, 28 ..
     * some fixed byte (?)
     * 6 characters
     * 247
     */
        twoByteMsg = 0x0D,
        fourByteMsg = 0x0B // start 4 byte message or sysex continue

    };
};


extern QString uartTXbuff;

class midiprocess : public QThread
{
    Q_OBJECT
public:
    midiprocess();
    ~midiprocess();

    RtMidiIn* midiin[8];
    RtMidiOut* midiout[8];

    void run() override {
        while (1) {
            if (serial->bytesToWrite() || uartTXbuff.isEmpty()) continue;
            qDebug()<<"writing to serial";
            serial->write("hello 111");
            uartTXbuff.clear();
            serial->waitForReadyRead(1000);
        }
    }

    bool getUsbStatus (void) {return usbStat;}
    int getDeviceOrderNumber(void) {return deviceOrderNumber;}
    std::vector<unsigned char> usbTXbuffer;
    void sendUsbMidi(std::vector<unsigned char> message, int port);
    void parseMackie(std::vector<unsigned char> message);

    void test(){
        uartTXbuff.append("TestStr123");
    }

    int checkIfChained (void);
public slots:

private slots:
    void serialGet(void);

private:
    QSerialPort* serial;

    bool initUsbMidi (int nPorts);
    bool usbStat=0;
    bool isUsbConnected(void);

    /*rpi default 3MHz uart clock needs to be adjusted to 3MHz x 31250 / 38400
        So add to /boot/config.txt init_uart_clock=2441406
        in this case 3000000 baud will be actually 2441406 and 38400 will be 31250*/
    typedef enum {
        midi38400=38400, midiHighSpeed=3000000
    }uartSpeed;

    bool  initUart (unsigned int baud = 38400);
    int getChainedCount(void);
    int deviceOrderNumber=0;


};

// must be non-static outside the class
void inputCallback  (double deltatime, std::vector< unsigned char > *message, void *);


#endif // MIDIPROCESS_H
