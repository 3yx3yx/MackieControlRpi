#include "midiprocess.h"


#include <unistd.h>
#include <QProcess>
#include <QFile>
#include <QDir>


volatile int bytesCount[16]={0};//for bytes/sec calculation
extern RtMidiIn::RtMidiCallback cbkTable[7];

midiprocess::midiprocess()
{
    qDebug()<<"midii1212";
    serial= new QSerialPort;

    initUart(uartSpeed::midi38400);

    int nDevicesChained=0;



    connect(serial,SIGNAL(readyRead()),this,SLOT(serialGet()));
    return;

    while(true)
    {
        serial->waitForReadyRead(3000);
        if (serial->bytesAvailable())
        {
            deviceOrderNumber = checkIfChained();
            break;
        }

        if (isUsbConnected())
        {
            nDevicesChained = getChainedCount();
            qDebug()<<"nDevicesChained:"<<nDevicesChained;
            bool ok = initUart(uartSpeed::midiHighSpeed);
            if (!ok){qDebug()<<"error setting uart hi speed"; return;}
            ok = initUsbMidi(nDevicesChained);
            if (!ok) {qDebug()<<"error on initUsbMidi(nDevicesChained)";return;}

            midiin[0] = new RtMidiIn;
            midiout[0] = new RtMidiOut;
            midiin[0]->ignoreTypes(0,1,1);
            midiin[0]->setCallback(&inputCallback);
            midiin[0]->openPort(1);

            for (int i =1; i<nDevicesChained-1; i++)
            {
                midiin[i] = new RtMidiIn;
                midiout[i] = new RtMidiOut;
                midiin[i]->setCallback(cbkTable[i-1]) ;
                midiin[i]->ignoreTypes(0,1,1);
                midiin[i]->openPort(1+i);
            }

            unsigned int nPorts = midiin[0]->getPortCount();
            qDebug()<<"ports available:"<<nPorts;

            for(unsigned int i=0; i<nPorts; i++)
            {
                qDebug()<<QString::fromStdString(midiin[0]->getPortName(i));
                qDebug()<<QString::fromStdString(midiout[0]->getPortName(i));
            }

            break;
        } // if usb connected
    } //while 1


    if(!(getUsbStatus()==1 && nDevicesChained==1))
    {
        connect(serial,SIGNAL(readyRead()),SLOT(serialGet()));        
    }
}

midiprocess:: ~midiprocess()
{
    QProcess process;
    process.start("/bin/sh",QStringList()<<"-c"<<"sudo modprobe -r g_midi");
}

void midiprocess::serialGet(void)
{
qDebug()<<"from serial:"<<serial->readAll();

//while (!port.atEnd()) {
//        QByteArray data = port.read(100);
//        ....
//    }
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


void midiprocess::sendUsbMidi(std::vector<unsigned char> message, int port)
{
    //    message.push_back( 0xF6 ); // push back means append
    //    midiout->sendMessage( &message );
    //    unsigned int i, nBytes=8;
    //    for ( int n=0; n<2; n++ )
    //    {
    //        message.clear();
    //        message.push_back( 240 );
    //        for ( i=0; i<nBytes; i++ ) message.push_back( i % 128 );
    //        message.push_back( 247 );
    //        midiout->sendMessage( &message );
    //    }
}

bool midiprocess::initUart(unsigned int baud)
{   
    serial->setPortName("ttyS0");
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

int midiprocess::getChainedCount(void)
{
    QByteArray tx,rx;
    tx.append(0x00 | midiDefs::fourByteMsg);
    tx.append(255);
    tx.append(255);
    tx.append(1);
    serial->write(tx);

    serial->waitForReadyRead(30000);
    rx = serial->readAll();
    tx.chop(1);
    if (rx.contains(tx) && rx.size() == 4)
    {
        return (int)rx.back();
    }
    return 1;
}

int midiprocess::checkIfChained(void)
{
    QByteArray tx,rx;
    tx.append(0x00 | midiDefs::fourByteMsg);
    tx.append(255);
    tx.append(255);
    rx = serial->readAll();
    if (rx.contains(tx) && rx.size() == 4)
    {
        return (int)rx.back();
    }
    return 0;
}

void inputCallback(double deltatime, std::vector< unsigned char > *message, void *)
{  
    unsigned int nBytes = message->size();
    bytesCount[0]+=nBytes;

    int msgType = (int)message->at(0);

    switch (msgType)
    {
    case midiDefs::vpot: return; break; // we have no v-pots
    case midiDefs::button:
        break;
    case midiDefs::vuMeter:
        break;
    case midiDefs::displayBlockStart:
        break;
    default:
        for ( unsigned int i=0; i<nBytes; i++ )
            qDebug() << "Byte " << i << " = " << (int)message->at(i)
                     <<" - "<< QChar((int)message->at(i)) << ", ";
        break;
    }

    if (msgType>midiDefs::faderFirst && msgType<midiDefs::faderFirst+8)
    {
        // updateFader(1);
    }

    if ( nBytes > 0 )
    {
        qDebug()  << "# of bytes = " << nBytes << ", stamp = " << deltatime;
    }

}

