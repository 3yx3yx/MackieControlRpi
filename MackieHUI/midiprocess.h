#ifndef MIDIPROCESS_H
#define MIDIPROCESS_H

#include <QMainWindow>
#include "RtMidi.h"

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


class midiprocess
{
public:
    midiprocess();
    ~midiprocess();

    RtMidiIn* midiin[16];
    RtMidiOut* midiout[16];

    bool getUsbStatus (void) {return usbStat;}

    std::vector<unsigned char> messageToSend;
    void sendUsbMidi(std::vector<unsigned char> message);

private:
    bool initUsbMidi (int nPorts);
    bool usbStat=0;
    bool isUsbConnected(void);
    int  initDinMidiConnection (void);
};
void sendUartMidi (std::vector<unsigned char> *message, int port);

void inputCallback  (double deltatime, std::vector< unsigned char > *message, void *);


#endif // MIDIPROCESS_H
