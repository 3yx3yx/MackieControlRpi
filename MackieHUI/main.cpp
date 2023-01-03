#include "mainwindow.h"
#include "qglobal.h"
#include <QApplication>
#include <QtSerialPort/QSerialPort>


int main(int argc, char *argv[])
{  
    Q_INIT_RESOURCE (images);
    QApplication::setAttribute(Qt::AA_UseOpenGLES);
    QApplication a(argc, argv);
    if (!bcm2835_init()) {qDebug()<<"bcm fail";}

    MainWindow w;
    w.showFullScreen();

    return a.exec();
}
