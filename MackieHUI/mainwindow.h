#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "QTimer"
#include "QDebug"
#include "bcm2835.h"
#include "midiprocess.h"
#include "mygpio.h"
#include <st7789.h>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
     midiprocess* midip;
int byte=1;
private slots:
void timerSlot(void);

private:
    Ui::MainWindow *ui;
    QTimer* timer;
};
#endif // MAINWINDOW_H
