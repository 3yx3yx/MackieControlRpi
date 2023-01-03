#ifndef MIDIPROCESS_H
#define MIDIPROCESS_H


#include "RtMidi.h"
#include "qmutex.h"
#include <QTimer>

#include <QtSerialPort/QSerialPort>

extern int startup_key_combo_detected;

QT_BEGIN_NAMESPACE
namespace midi {
class midiprocess;
}
QT_END_NAMESPACE

struct midiDefs {
    enum { // midi and mackie protocol constants

        btn_rec =0,
        btn_solo,
        btn_mute,
        btn_sel,
        fader_pos,

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
     * 240 00 00 102 20 18
     * channelN * 7   0, 7 .. 21, 28 ..
     * unused byte
     * 6 characters
     * 247
     */

    };
};

class UsbMidiReader;
class SerialRepeater;

class midiprocess : public QObject
{
    Q_OBJECT
public:
    midiprocess();
    ~midiprocess();


    QString connectionStatus;
    void sendMackieMsg (unsigned int channel, int type, int value);
    inline bool getUsbStatus (void) {return usbStat;}
    int getDeviceOrderNumber(void) {return deviceOrderNumber;}

signals:

    void startReadingUsb();

    void RecArmed (int channel, bool state);
    void Soloed (int channel, bool state);
    void Muted (int channel, bool state);
    void Selected (int channel, bool state);
    void VUupdated (int channel, int val);
    void nameUpdated(int channel, QString name);
    void faderUpdated (int channel, int val);
    void deviceNumChanged();
    void statusUpdated(QString s);

public slots:
    void onfaderMoved  (int channel, int val);
    void onrecArmPressed (int channel);
    void onsoloPressed (int channel);
    void onmutePressed (int channel);
    void onselectPressed (int channel);


private slots:
    void serialFilter(void);
    void serialGetSlowMidi(void);  
    void parseMackie(QByteArray msg);
    void timerSlot();

private:
    QRecursiveMutex mutexSerial;
    QSerialPort* serial;
    UsbMidiReader* midireader;
    RtMidiIn* midiin[8];
    RtMidiOut* midiout[8];
    SerialRepeater* repeater;

    QTimer* timer;

    int nDevices=0;
    bool initUsbMidi (int nPorts);
    bool usbStat=0;
    bool baudChanged = 0;
    bool isUsbConnected(void);

    void sendAskNumberSeq();

    typedef enum {
        midi31250=31250, midiHighSpeed=3000000
    }uartSpeed;

    bool  initUart (unsigned int baud = 31250);
    int getDevicesCount(void);
    int deviceOrderNumber=0;

};

class UsbMidiReader : public QObject {
    Q_OBJECT
public:
    UsbMidiReader(RtMidiIn *ports[8] , int ndevices);
    UsbMidiReader() = default;
    std::vector<unsigned char> curMsg;
    int curDestination=0;
public slots:
    void processInputData();
signals:
    void usbMsgForUart(QByteArray msg, int dest);
    void usbMsgForDevice(QByteArray msg);
    //void lookForNewMsg();
private:
    RtMidiIn* _ports[8];
    int _nDevices;
};

class SerialRepeater : public QObject {
    Q_OBJECT
public:
    SerialRepeater(RtMidiOut *ports[8], int ndevices);
    ~SerialRepeater();
public slots:
    void process();
    void sendToChain(QByteArray msg, int dest);
signals:

private:

    RtMidiOut* _ports[8];
    QSerialPort* serial;
    int _nDevices;
    std::vector<unsigned char> curMsg;

};

#endif // MIDIPROCESS_H
