#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    if (!bcm2835_init()) {qDebug()<<"bcm fail";}
    MainWindow w;
    w.show();
    return a.exec();
}
