#include "midiprocess.h"
#include <unistd.h>
#include <QProcess>
#include <QFile>
#include <QDir>
#include "qdebug.h"
#include "qthread.h"


midiprocess::midiprocess()
{
    serial = new QSerialPort;
    initUart(31250);

    while(true)
    {
        if (isUsbConnected())
        {
            nDevices = getDevicesCount();
            qDebug()<<"nDevicesChained:"<<nDevices;
            bool ok = initUart(uartSpeed::midiHighSpeed);
            if (!ok){qDebug()<<"error setting uart  speed"; return;}
            ok = initUsbMidi(nDevices);
            if (!ok) {qDebug()<<"error on initUsbMidi(nDevicesChained)";return;}

            for (int i =0; i<nDevices-1; i++)
            {
                midiin[i] = new RtMidiIn;
                midiout[i] = new RtMidiOut;
                midiin[i]->ignoreTypes(0,1,1);
                midiin[i]->openPort(i+1); // port 0 is thru port
            }

            unsigned int nPorts = midiin[0]->getPortCount();
            qDebug()<<"ports available:"<<nPorts;

            for(unsigned int i=0; i<nPorts; i++)
            {
                qDebug()<<QString::fromStdString(midiin[0]->getPortName(i));
                qDebug()<<QString::fromStdString(midiout[0]->getPortName(i));
            }

            midireader = new UsbMidiReader(midiin,nDevices);
            connect (midireader,SIGNAL(usbMsgForUart()), this, SLOT(sendUartToChain()));

            connect(midireader,SIGNAL(usbMsgForDevice(std::vector<unsigned char>)),
                    this, SLOT(parseMackie(std::vector<unsigned char>)));


            connect (this, SIGNAL(startReadingUsb()), midireader, SLOT(processInputData()));
            QThread* thread = new QThread;
            midireader->moveToThread(thread);
            thread->start();

            if(nDevices>1) {  // if we have chained devices
                connect(serial,SIGNAL(readyRead()),SLOT(serialToUsbRepeat()));
            }
            emit startReadingUsb();

            break;

        } // if usb connected

        else // if no usb connection
        {
            if (serial->bytesAvailable())
            {
                deviceOrderNumber = checkIfChained();
                if(deviceOrderNumber) // if device is in the chain
                {
                    QByteArray tx;
                    tx.append(255);
                    tx.append(255);
                    tx.append(deviceOrderNumber+1);
                    serial->write(tx);

                    bool ok = initUart(uartSpeed::midiHighSpeed);
                    if (!ok){qDebug()<<"error setting uart  speed"; return;}
                    connect(serial,SIGNAL(readyRead()),SLOT(serialFilter()));
                }
                else {// if device is connected directly to midi adapter/interface
                    connect(serial,SIGNAL(readyRead()),SLOT(serialGetSlowMidi()));
                }

                break;
            }
            serial->waitForReadyRead(1000);
        }

    } //while 1
}

midiprocess:: ~midiprocess()
{
    QProcess process;
    process.start("/bin/sh",QStringList()<<"-c"<<"sudo modprobe -r g_midi");
}

void midiprocess::sendMackieMsg(unsigned int channel, int type, int value)
{
    std::vector<unsigned char> msg;
    switch (type)
    {
    case midiDefs::fader_pos:
        msg.push_back(midiDefs::faderFirst+channel);
        msg.push_back((unsigned char)(value>>8));
        msg.push_back((unsigned char)(value & 0xFF));
        break;
    case midiDefs::btn_rec:
        msg.push_back(midiDefs::button);
        msg.push_back(channel);
        if(value)
            msg.push_back(127);
        else
            msg.push_back(0x00);
        break;
    case midiDefs::btn_solo:
        msg.push_back(midiDefs::button);
        msg.push_back(channel + 8);
        if(value)
            msg.push_back(127);
        else
            msg.push_back(0x00);
        break;
    case midiDefs::btn_mute:
        msg.push_back(midiDefs::button);
        msg.push_back(channel + 16);
        if(value)
            msg.push_back(127);
        else
            msg.push_back(0x00);
        break;
    case midiDefs::btn_sel:
        msg.push_back(midiDefs::button);
        msg.push_back(channel+ 24);
        if(value)
            msg.push_back(127);
        else
            msg.push_back(0x00);
        break;
    }

    if (getUsbStatus()) {
        midiout[0]->sendMessage(&msg);
        return;
    }

    lockSerial.lockForWrite();
    if (deviceOrderNumber)  // if device is in the chain
    {
        serial->putChar(0xF0 + deviceOrderNumber);
    }  // if not skip this byte and write to uart

    for (int i=0; i< (int)msg.size(); i++)
    {
        serial->putChar(msg.at(i));
    }
    lockSerial.unlock();


}

// slots // // // //
void midiprocess::onfaderMoved(int channel, int val)
{
    sendMackieMsg(channel, midiDefs::fader_pos, val);
}

void midiprocess::onrecArmPressed(int channel)
{
    channels[channel].recArmed = !channels[channel].recArmed;
    sendMackieMsg(channel, midiDefs::btn_rec, channels[channel].recArmed);
    emit RecArmed(channel,  channels[channel].recArmed);
}

void midiprocess::onsoloPressed(int channel)
{
    channels[channel].solo = !channels[channel].solo;
    sendMackieMsg(channel, midiDefs::btn_solo, channels[channel].solo);
    emit Soloed(channel, channels[channel].solo );
}

void midiprocess::onmutePressed(int channel)
{
    channels[channel].mute = !channels[channel].mute;
    sendMackieMsg(channel, midiDefs::btn_mute, channels[channel].mute);
    emit Muted(channel, channels[channel].mute);
}

void midiprocess::onselectPressed(int channel)
{
    channels[channel].selected = !channels[channel].selected;
    sendMackieMsg(channel, midiDefs::btn_sel, channels[channel].selected);
    emit Selected(channel, channels[channel].selected);
}
// end slots // // // // // // // //

void midiprocess::serialFilter()
{
    int port=0;
    char c=0;
    bool search=1; // flag - searching for msg start byte
    bool redirect =0;
    int msgLen=0;
    int pos=0;
    std::vector<unsigned char> msg;

    lockSerial.lockForRead();
    while (!serial->atEnd())
    {
        serial->getChar(&c);

        if (c> 0xF0 && search) // the order number byte (custom protocol for chained devices)
        {
            search = 0;
            redirect = 0;
            port = (c & 0xF);
            if (port != deviceOrderNumber)
            {
                redirect = 1;
            }
            pos=0;
            msgLen=0;
            continue;
        }
        if (search) { // if message start is not found
            continue;
        }
        if (pos==0) // msg start byte was found
        {
            if (c>=midiDefs::faderFirst && c<=midiDefs::faderFirst+7)
            {
                msgLen = 3;
            }
            else
            {
                switch (c)
                {
                case midiDefs::button: msgLen = 3; break;
                //case midiDefs::vpot: msgLen = 3; break;
                case midiDefs::displayBlockStart: msgLen = 14; break;
                case midiDefs::vuMeter: msgLen = 2; break;
                default: // unexpected msg or error
                    search =1;
                    continue;
                    break;
                }
            }
        }

        if (redirect)
        {
            if (pos < msgLen)
            {
                serial->putChar(c);
                pos++;
            }
            if (pos == msgLen)
            {
                search = 1;
                continue;
            }
        }

        else
        {
            if (pos < msgLen)
            {
                msg.push_back(c);
                pos++;
            }
            if (pos == msgLen)
            {
                parseMackie(msg);
                search =1;
            }
        }

    }

    lockSerial.unlock();
}

void midiprocess::serialGetSlowMidi()  // slot for general DIN MIDI connections
{
    char c=0;
    int msgLen=0;
    int pos=0;
    std::vector<unsigned char> msg;

    lockSerial.lockForRead();

    while (!serial->atEnd())
    {
        serial->getChar(&c);

        if (pos==0)
        {
            if (c>=midiDefs::faderFirst && c<=midiDefs::faderFirst+7)
            {
                msgLen = 3;
            }
            else
            {
                switch (c)
                {
                case midiDefs::button: msgLen = 3; break;
                case midiDefs::vpot: msgLen = 3; break;
                case midiDefs::displayBlockStart: msgLen = 14; break;
                case midiDefs::vuMeter: msgLen =2; break;
                default: // unexpected msg or error
                    pos = 0;
                    continue;
                    break;
                }
            }
        }

        if (pos < msgLen)
        {
            msg.push_back(c);
            pos++;
        }
        if (pos == msgLen)
        {
            parseMackie(msg);
            msg.clear();
            pos = 0;
        }
    }

    lockSerial.unlock();
}

void midiprocess::serialToUsbRepeat()
{
    int port=0;
    char c=0;
    bool search=1; // flag - searching for msg start byte
    int msgLen=0;
    int pos=0;
    std::vector<unsigned char> msg;

    lockSerial.lockForRead();

    while (!serial->atEnd())
    {
        serial->getChar(&c);

        if (c> 0xF0 && search) // the order number byte (custom protocol for chained devices)
        {
            search = 0;
            port = (c & 0x0F) - 1; //last 4 bits // port = 0..7
            msg.clear();
            pos=0;
            msgLen=0;
            continue;
        }
        if (search) { // if message start is not found
            continue;
        }
        if (pos==0) // msg start byte was found
        {

            if (c ==  midiDefs::button ||
                    // c ==  midiDefs::vpot ||
                    (c>=midiDefs::faderFirst && c<=midiDefs::faderFirst+7 ))
            {
                msgLen = 3;
            }
            else {  // unexpected message or error
                search =1;
                continue;
            }
        }
        if (pos < msgLen)
        {
            msg.push_back(c);
            pos++;
        }
        if (pos == msgLen)
        {
            midiout[port]->sendMessage(& msg);
            search = 1;
        }

    }

    lockSerial.unlock();
}

void midiprocess::sendUartToChain()
{
    int msgLen  = midireader->curMsg.size();

    lockSerial.lockForWrite();

    serial->putChar(0xF0 | midireader->curDestination); // custom protocol for chained devices //cur dest = 1..8
    for (int i=0; i<msgLen; i++)
    {
        serial->putChar(midireader->curMsg.at(i));
    }

    lockSerial.unlock();

    emit startReadingUsb();
}

bool midiprocess::isUsbConnected(void)
{
    initUsbMidi(1); //insert device module
    QDir::setCurrent("/sys/class/udc/fe980000.usb/device/gadget");
    QFile file("suspended");
    if(!file.open(QFile::ReadOnly |
                  QFile::Text))
    {
        qDebug() << " Could not open file in /sys/class/udc";
        usbStat = 0;
        initUsbMidi(0);//eject
        return usbStat;
    }
    QTextStream in(&file);
    QString s = in.readAll();
    qDebug() <<"is device suspended:"<< s;
    if(!s.toInt())
        usbStat = 1;
    initUsbMidi(0);
    return usbStat;
}

bool midiprocess::initUsbMidi(int nPorts)
{
    QProcess process;
    QString stdout;
    if (!nPorts){
        process.start("/bin/sh",QStringList()<<"-c"<<"sudo modprobe -r g_midi");
        qDebug() << "removing gmidi";
        process.waitForFinished(10000);
        stdout = process.readAllStandardOutput();
        qDebug() << stdout;
        if (stdout.isEmpty()) return 1; // if no error message
    }

    QString s = "sudo modprobe g_midi in_ports="+QString::number(nPorts)+
            " out_ports="+QString::number(nPorts)+
            " iProduct=FaderBank";
    qDebug() <<s;
    process.start // insert g_midi module with parameters
            ("/bin/sh",QStringList()<<"-c"<<s);
    qDebug() << "insert gmidi";
    process.waitForFinished(10000);
    stdout = process.readAllStandardOutput();
    qDebug() << stdout;
    if (stdout.isEmpty()) return 1;

    return 0;
}

void midiprocess::parseMackie(std::vector<unsigned char> msg)
{

    int type=0;
    int chn =0;

    if (msg.front()>= midiDefs::faderFirst && msg.front()<= midiDefs::faderFirst+7)
    {
        chn = msg.front() - midiDefs::faderFirst;
        channels[chn].faderPosition = ((int)msg.at(1)<<8) | msg.at(2);
        emit faderUpdated(chn,channels[chn].faderPosition );
        qDebug()<<"fader position: "<<channels[chn].faderPosition;
    }
    else
    {
        switch (msg.front())
        {
        case (midiDefs::vpot): return;
            break;
        case (midiDefs::button):
            type = (int)(msg.at(1)/8);
            chn = msg.at(1)%8;

            switch (type)
            {
            case 0:
                channels[chn].recArmed = (msg.at(2)!=0);
                emit RecArmed(chn, channels[chn].recArmed);
                break;
            case 1:
                channels[chn].solo = (msg.at(2)!=0);
                emit Soloed(chn, channels[chn].solo);
                break;
            case 2:
                channels[chn].mute = (msg.at(2)!=0);
                emit Muted(chn, channels[chn].mute);
                break;
            case 3:
                channels[chn].selected = (msg.at(2)!=0);
                emit Selected(chn, channels[chn].selected);
                break;
            }

            break;
        case (midiDefs::displayBlockStart):

            if (msg.size()<14) {return;}

            if(msg.at(1) ==0 && msg.at(2) ==102 && msg.at(3) ==20 && msg.at(4) ==18)// 240 00 102 20 18
            {
                chn = (int)(msg.at(5)/7);
            }
            else
            {
                return;
            }
            for (int i=0; i<6; ++i)
            {
                channels[chn].name[i]=msg.at(7+i);
            }
            emit nameUpdated(chn, QString (channels[chn].name));
            break;

        case (midiDefs::vuMeter):
            chn = msg.at(1)/16;
            channels[chn].vuMeter =  msg.at(1)%16;
            emit VUupdated(chn, channels[chn].vuMeter);
            break;
        }
    }

    emit startReadingUsb();
}


bool midiprocess::initUart(unsigned int baud)
{   
    serial->setPortName("/dev/ttyS0");
    serial->open(QIODevice::ReadWrite);
    serial->setBaudRate(baud);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);
    while(!serial->isOpen()) serial->open(QIODevice::ReadWrite);
    if (serial->isOpen() && serial->isWritable()){
        qDebug() << "Serial is open";
        return 1;
    }
    else qDebug() << "Serial is NOT open";
    return 0;
}

int midiprocess::getDevicesCount(void)
{
    QByteArray tx,rx;

    tx.append(255);
    tx.append(255);
    tx.append(1);
    serial->write(tx);

    serial->waitForReadyRead(3000);

    while (rx.size()<3)
    {
        rx.append(serial->readAll());
    }

    tx.chop(1);
    if (rx.contains(tx) && rx.size() == 3)
    {
        return (int)rx.back();
    }
    return 1;
}

// returns the device ord number
int midiprocess::checkIfChained(void)
{
    QByteArray tx,rx;

    tx.append(255);
    tx.append(255);
    while (rx.size()<3)
    {
        rx.append(serial->readAll());
    }
    if (rx.contains(tx) && rx.size() == 3)
    {
        return (int)rx.back();
    }
    return 0;
}



UsbMidiReader::UsbMidiReader(RtMidiIn *ports[], int ndevices)
{
    _ports = *ports;
    _nDevices = ndevices;
    curMsg.reserve(32);
}

void UsbMidiReader::processInputData()
{
    for (int i = 0; i<_nDevices-1; i++)
    {
        _ports[i].getMessage(&curMsg);
        if (!curMsg.empty())
        {
            if (curMsg.front() == midiDefs::vpot || curMsg.size()>16)
            {
                continue; // skip v-pot messages and long info messages
            }
            curDestination = i+1;

            if (i == 0)
            {
                emit usbMsgForDevice(curMsg);
            }
            else
            {
                emit usbMsgForUart();
            }
        }
    }

}
