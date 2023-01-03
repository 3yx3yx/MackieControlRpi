#include "midiprocess.h"
#include <unistd.h>
#include <QProcess>
#include <QFile>
#include <QDir>
#include "qdebug.h"
#include "qthread.h"

#define UART_RX_SIZE 100'000

midiprocess::midiprocess()
{
    serial = new QSerialPort(this);
    timer = new QTimer(this);
    serial->setReadBufferSize(UART_RX_SIZE);
    bool ok = initUart(uartSpeed::midiHighSpeed);
    if (!ok){qDebug()<<"error setting uart"; return;}

    while(true)
    {
        if (isUsbConnected())
        {
            nDevices = getDevicesCount();
#ifdef TEST
            nDevices = 3;
#endif
            qDebug()<<"nDevicesChained:"<<nDevices;
            connectionStatus = ("usb "+QString::number(nDevices));
            serial->close();
            serial->deleteLater();

            ok = initUsbMidi(nDevices);
            if (!ok) {qDebug()<<"error on initUsbMidi(nDevicesChained)";return;}

            try {
                midiin[0] = new RtMidiIn;
                midiout[0] = new RtMidiOut;
            }
            catch ( RtMidiError &error ) {
                error.printMessage();
                exit( EXIT_FAILURE );
            }

            qDebug()<<QString::fromStdString(midiin[0]->getPortName(0+1));
            unsigned int nPorts = midiin[0]->getPortCount();
            qDebug()<<"ports available:"<<nPorts;

            if (nPorts-1 != (unsigned int)nDevices)
            {
                ok = initUsbMidi(0);
                if (!ok) {qDebug()<<"error on initUsbMidi(0)";return;}
                ok = initUsbMidi(nDevices);
                if (!ok) {qDebug()<<"error on initUsbMidi(nDevicesChained)";return;}
            }

            midiin[0]->ignoreTypes(0,1,1);
            midiin[0]->openPort(1); // port 0 is thru port
            midiout[0]->openPort(1); // port 0 is thru port

            for (int i =1; i<nDevices; i++)
            {
                midiin[i] = new RtMidiIn;
                midiout[i] = new RtMidiOut;
                midiin[i]->ignoreTypes(0,1,1);
                midiin[i]->openPort(i+1); // port 0 is thru port
                midiout[i]->openPort(i+1); // port 0 is thru port
                qDebug()<<QString::fromStdString(midiin[0]->getPortName(i+1));
            }

            midireader = new UsbMidiReader(midiin,nDevices);
            repeater = new SerialRepeater (midiout, nDevices);

            connect (midireader,SIGNAL(usbMsgForUart(QByteArray,int)),
                     repeater, SLOT(sendToChain(QByteArray,int)));

            connect(midireader,SIGNAL(usbMsgForDevice(QByteArray)),
                    this, SLOT(parseMackie(QByteArray)));

            connect (this, SIGNAL(startReadingUsb()), midireader, SLOT(processInputData()));

            QThread* thread = new QThread;
            midireader->moveToThread(thread);
            thread->start(QThread::TimeCriticalPriority);

            QThread* threadRepeater = new QThread;
            repeater->moveToThread(threadRepeater);
            threadRepeater->start(QThread::TimeCriticalPriority);

            emit startReadingUsb();
            qDebug()<< "usb init ok";
            break;

        } // if usb connected

        else // if no usb connection
        {
            sendAskNumberSeq();
            connectionStatus =("serial detected");
            if (serial->bytesAvailable())
            {
                connect (timer, SIGNAL(timeout()),this,SLOT(timerSlot()));
                timer->setSingleShot(1);
                timer->start(1000);
                connect(serial,SIGNAL(readyRead()),this,SLOT(serialFilter()));
                break;
            }
            serial->waitForReadyRead(1000);
        }

        qDebug()<< "no connection midi";
        connectionStatus =("no connection");

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
        //qDebug()<< "fader pos "<< value;
        msg.push_back(midiDefs::faderFirst+channel);
        msg.push_back((unsigned char)(value & 0x7F));
        msg.push_back((unsigned char)(value >> 7));
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


    if (deviceOrderNumber)  // if device is in the chain
    {
        serial->putChar(0xF0 + deviceOrderNumber);
        serial->waitForBytesWritten();
    }  // if not skip this byte and write to uart

    for (int i=0; i< (int)msg.size(); i++)
    {
        serial->putChar((char)msg.at(i));
        serial->waitForBytesWritten();
    }

}

// slots // // // //
void midiprocess::onfaderMoved(int channel, int val)
{
    sendMackieMsg(channel, midiDefs::fader_pos, val);
}

void midiprocess::onrecArmPressed(int channel)
{
    sendMackieMsg(channel, midiDefs::btn_rec, 1);
}

void midiprocess::onsoloPressed(int channel)
{
    sendMackieMsg(channel, midiDefs::btn_solo, 1);
}

void midiprocess::onmutePressed(int channel)
{
    sendMackieMsg(channel, midiDefs::btn_mute, 1);
}

void midiprocess::onselectPressed(int channel)
{
    sendMackieMsg(channel, midiDefs::btn_sel, 1);
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
    bool deviceNreq=0;
    QByteArray msg;
    static int errorsCount = 0;

    if (serial->bytesAvailable()> 10'000 || serial->bytesAvailable()<0) {
        serial->clear(); // if too much bytes in queue
        qDebug()<< "ERROR -- too much bytes in queue UART";
        return;
    }

    QMutexLocker locker (&mutexSerial);
    forever
    {
        if (!serial->getChar(&c))
        {
            if (pos<msgLen)
            {
                if (!serial->waitForReadyRead(1000)) break;
                else
                    continue;
            }
            else break;
        }

        if (search)
        {
            if (c> 0xF7 || c == 0xAB) // first byte is the order number byte (custom protocol for chained devices)
            {
                msg.clear();
                search = 0;
                redirect = 0;
                pos=0;
                msgLen=0;
                deviceNreq=0;

                if (c == 0xAB)
                {
                    redirect = 1;
                    deviceNreq = 1;
                }
                else
                {
                    port = (c - 0xF7);
                    if (port != deviceOrderNumber)
                    {
                        redirect = 1;
                        pos = -1; // skip if(pos==0) ...
                    }
                    else
                        continue;
                }
            }
            else
            {
                errorsCount++;
                if (errorsCount >  100)
                {
                    serial->blockSignals(1);
                    serial->clear();
                    qDebug()<< " errors on serial port. selecting other baud rate";
                    initUart(31250);
                    baudChanged = 1;
                    disconnect(serial,SIGNAL(readyRead()),this,SLOT(serialFilter()));
                    connect(serial,SIGNAL(readyRead()),this,SLOT(serialGetSlowMidi()));
                    emit statusUpdated("din ");
                    sleep(1);
                    break;
                    return;
                }
                continue;
            }

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
                case midiDefs::displayBlockStart: msgLen = 15; break;
                case midiDefs::vuMeter: msgLen = 2; break;
                case 0xAB: msgLen=5; break;
                default: // unexpected msg or error
                    search =1;
                    errorsCount++;
                    continue;
                    break;
                }
            }

            errorsCount=0;
        }
        if (pos > 0 && c>127 && !(pos==14 && c==0xF7))
        {
            pos = 0;
            msgLen=0;
            msg.clear();
            qDebug()<< "msg clr";
            continue;
        }

        if (pos < msgLen)
        {
            msg.push_back(c);
            pos++;
        }

        if (pos == msgLen)
        {
            if (redirect)
            {
                if (deviceNreq) // if enumeration request
                {
                    const char askSeq[4] = {0xAB,0xCD,0xDE,0xEF};
                    if (msg.contains(askSeq))
                    {
                        if ((msg.at(4) != 0xAA))
                        {
                            int n = msg.at(4);
                            msg.chop(1);
                            msg.append(n+1);
                        }
                    }
                    else
                    {
                        search = 1;
                        continue;
                    }
                }

                serial->write(msg);
                serial->waitForBytesWritten();
            }
            else parseMackie(msg);

            search = 1;
            continue;
        }
    }
}

void midiprocess::serialGetSlowMidi()  // slot for general DIN MIDI connections
{
    char c=0;
    int msgLen=0;
    int pos=0;

    QByteArray msg;

    if (serial->bytesAvailable()> 10'000 || serial->bytesAvailable()<0) {
        serial->clear(); // if too much bytes in queue
        qDebug()<< "ERROR -- too much bytes in queue UART";
        return;
    }

    QMutexLocker locker (&mutexSerial);

    forever
    {
        if (!serial->getChar(&c))
        {
            if (pos<msgLen)
            {
                if (!serial->waitForReadyRead(1000)) break;
                else
                    continue;
            }
            else break;
        }


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
                case midiDefs::displayBlockStart: msgLen = 15; break;
                case midiDefs::vuMeter: msgLen =2; break;
                default: // unexpected msg or error
                    pos = 0;
                    continue;
                    break;
                }
            }
        }

        if (pos > 0 && c>127 && !(pos==14 && c==0xF7))
        {
            pos = 0;
            msgLen=0;
            msg.clear();
            qDebug()<< "msg clr";
            continue;
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
            msgLen=0;
        }
    }

}

bool midiprocess::isUsbConnected(void)
{
    QProcess process;
    if (startup_key_combo_detected == 1)
    {
        process.start("/bin/sh",QStringList()<<"-c"<<"sudo modprobe -r g_midi");
        process.waitForFinished(10000);
        process.start("/bin/sh",QStringList()<<"-c"<<"sudo modprobe g_ether");
        qDebug()<< "g_ether enabled for software update";
        exit(0);

        // NOTE the RaspberryPi is configured to have a static 192.168.7.2 IP address
    }
    if (startup_key_combo_detected == 2)
    {
        process.start("/bin/sh",QStringList()<<"-c"<<"sudo modprobe -r g_midi");
        process.waitForFinished(10000);
        process.start("/bin/sh",QStringList()<<"-c"<<"sudo modprobe g_serial");
        qDebug()<< "g_serial enabled for software update";
        exit(0);
    }
    QDir::setCurrent("/sys/class/udc/fe980000.usb/device/gadget");
    QFile file("suspended");
    usbStat = 0;
    if(!file.open(QFile::ReadOnly |
                  QFile::Text))
    {
        qDebug() << " Could not open file in /sys/class/udc";
        initUsbMidi(1);
    }
    QTextStream in(&file);
    QString s = in.readAll();
    qDebug() <<"is device suspended:"<< s;
    if(!s.toInt())
    {
        usbStat = 1;
    }

    process.start("/bin/sh",QStringList()<<"-c"<<"dmesg | grep \"dwc2 fe980000.usb: new address\"");
    process.waitForFinished(10000);
    s = process.readAllStandardOutput();
    qDebug() << s;
    if (s.isEmpty()) usbStat=0; // if nothing found in dmesg

    return usbStat;
}

void midiprocess::sendAskNumberSeq()
{
    QMutexLocker locker(&mutexSerial);
    QByteArray tx;

    tx.append(0xAB);
    tx.append(0xCD);
    tx.append(0xDE);
    tx.append(0xEF);
    tx.append(0xAA);
    serial->write(tx);
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
    process.waitForFinished(10'000);
    stdout = process.readAllStandardOutput();
    qDebug() << stdout;
    if (stdout.isEmpty()) return 1;

    return 0;
}

void midiprocess::parseMackie(QByteArray msg)
{
    int type=0;
    int chn =0;
    int param =0;
    QString str;
    if (msg.at(0) >= midiDefs::faderFirst && msg.at(0) <= midiDefs::faderFirst+7)
    {
        chn = msg.at(0) - midiDefs::faderFirst;
        param = (((uint16_t)msg.at(2)<<7)&0x3F80) | (msg.at(1) & 0x7F);
        emit faderUpdated(chn,param);

    }
    else
    {
        switch (msg.at(0))
        {
        case (midiDefs::vpot): return;
            break;
        case (midiDefs::button):

            type = (int)(msg.at(1)/8);
            chn = msg.at(1)%8;

            switch (type)
            {
            case 0:
                param = (msg.at(2)!=0);
                emit RecArmed(chn, param);
                break;
            case 1:
                param = (msg.at(2)!=0);
                emit Soloed(chn, param);
                break;
            case 2:
                param = (msg.at(2)!=0);
                emit Muted(chn, param);
                break;
            case 3:
                param = (msg.at(2)!=0);
                emit Selected(chn, param);
                break;
            }

            break;
        case (midiDefs::displayBlockStart):

            if (msg.size()<14) {return;}

            if(msg.at(1) ==0 && msg.at(2) ==0 && msg.at(3) ==102 && msg.at(4) ==20 && msg.at(5)==18)// 240 00 102 20 18
            {
                chn = (int)(msg.at(6)/7);
            }
            else
            {
                return;
            }

            for (int i=0; i<6; ++i)
            {
                if (msg.at(7+i)<127)
                    str.append(msg.at(7+i));
            }

            emit nameUpdated(chn, str);
            break;

        case (midiDefs::vuMeter):
            chn = (msg.at(1)+1) / 16;
            param =  (msg.at(1))%16;

            emit VUupdated(chn, param);
            break;
        }
    }

}

void midiprocess::timerSlot()
{
    if (deviceOrderNumber == 0 && !baudChanged)
    {
        sendAskNumberSeq();
        if (isUsbConnected())
        {
            exit(1);
        }
        timer->start(1000);
    }
    else
    {
        disconnect (timer, SIGNAL(timeout()),this,SLOT(timerSlot()));
        timer->deleteLater();
        serial->clear();
        serial->blockSignals(0);
    }
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

    tx.append(0xAB);
    tx.append(0xCD);
    tx.append(0xDE);
    tx.append(0xEF);
    tx.append(1);

    serial->write(tx);
    tx.chop(1); // remove last byte to compare

    sleep(1);

    if (serial->bytesAvailable())
    {
        while (!serial->atEnd())
        {
            rx.append(serial->readAll());
        }
    }

    if (rx.contains(tx))
    {
        int n = rx.at(rx.indexOf(tx)+4);
        return n;
    }
    return 1;
}

UsbMidiReader::UsbMidiReader(RtMidiIn *ports[8], int ndevices)
{
    for (int i = 0; i< ndevices+1; i++)
    {
        _ports[i] = ports[i];
    }

    _nDevices = ndevices;
    curMsg.reserve(32);
}

void UsbMidiReader::processInputData()
{
    forever
    {
        bool have_msg = 0;

        for (int i = 0; i<_nDevices; i++)
        {
            _ports[i]->getMessage(&curMsg);

            if (!curMsg.empty())
            {
                have_msg = 1;
                if (curMsg.front() == midiDefs::vpot || curMsg.size()>15)
                {
                    continue; // skip v-pot messages and long info messages
                }
                curDestination = i;
                auto msg = QByteArray(reinterpret_cast<const char*>(curMsg.data()), curMsg.size());
                if (i == 0)
                {
                    emit usbMsgForDevice(msg);
                }
                else
                {
#ifdef TEST
                    // qDebug()<< "new msg for uart chain"<<(int)msg.at(0) << "i="<< i;
#endif
                    emit usbMsgForUart(msg, curDestination);
                }
            }
        }

        if (!have_msg)
        {
            usleep(5*1000);
        }

    }
}

SerialRepeater::SerialRepeater(RtMidiOut *ports[], int ndevices)
{
    for (int i = 0; i< ndevices+1; i++)
    {
        _ports[i] = ports[i];
    }
    _nDevices = ndevices;
    curMsg.reserve(32);

    serial = new QSerialPort(this);
    serial->setReadBufferSize(UART_RX_SIZE);
    connect(serial, SIGNAL(readyRead()), this, SLOT(process()));

    serial->setPortName("/dev/ttyS0");
    serial->open(QIODevice::ReadWrite);
    serial->setBaudRate(3'000'000);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);
    while(!serial->isOpen()) serial->open(QIODevice::ReadWrite);
    if (serial->isOpen() && serial->isWritable()){
        qDebug() << "Serial is open";
    }
    else qDebug() << "Serial is NOT open";



}

SerialRepeater::~SerialRepeater()
{
    serial->close();
    delete serial;
}

void SerialRepeater::process()
{
    QByteArray msg;
    int channel=0;
    const char askSeq[] = {0xAB,0xCD,0xDE,0xEF};

    msg = serial->readAll();
    if (msg.size()<4) return;

    if (msg.contains(askSeq))
    {
        if ((msg.indexOf(askSeq) + 4)>=msg.size()) return;
        if (msg.at(msg.indexOf(askSeq) + 4) == 0xAA)
            serial->write(askSeq + (char)0x01);
        else if (msg.at(msg.indexOf(askSeq) + 4) != _nDevices)  {exit(1);}
    }

    for (int i=0; i<msg.size(); i++) //expected mesages are 4 byte only  (fader and buttons)
    {
        if (msg.at(i) > 0xF7)
        {
            channel = msg.at(i) - 0xF7;

            if (i+3<msg.size())
            {
                if (msg.at(i+1) == midiDefs::button ||
                        (msg.at(i+1)>=midiDefs::faderFirst && msg.at(i+1) < midiDefs::faderFirst+7 ))
                {
                    curMsg.clear();
                    curMsg.push_back(msg.at(i+1));
                    curMsg.push_back(msg.at(i+2));
                    curMsg.push_back(msg.at(i+3));
#ifdef TEST
                    qDebug()<<"repeated "<<(int)msg.at(i+1)<<(int)msg.at(i+2)<<(int)msg.at(i+3)<<" chan "<< channel;
#else
                    _ports[channel]->sendMessage(&curMsg);
#endif
                }
                i+=3;
            }
        }
    }
}


void SerialRepeater::sendToChain(QByteArray msg, int dest)
{
#ifdef TEST
    if (msg.startsWith(240) || msg.startsWith(208)) return;
#endif
    msg.prepend(0xF7 + dest); // custom protocol for chained devices // dest = 1..8
    serial->write(msg);
    serial->waitForBytesWritten();
}
