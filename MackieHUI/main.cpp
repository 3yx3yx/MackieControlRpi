#include "mainwindow.h"
#include <QApplication>
#include <QtSerialPort/QSerialPort>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    if (!bcm2835_init()) {qDebug()<<"bcm fail";}

    MainWindow w;
    w.showFullScreen();

    return a.exec();
}
