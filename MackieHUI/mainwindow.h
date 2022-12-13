#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "QTimer"
#include "QDebug"
#include "bcm2835.h"
#include "midiprocess.h"
#include "mygpio.h"
#include <st7789.h>
#include "infodisplay.h"
#include "DisplayThread.h"

#include <QImage>

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
    Gpio* gpio;
    DisplayThread* dispThread;
    infoDisplay* tftUI;

private slots:
    void updateInterface(void);

private:
    Ui::MainWindow *ui;
    QTimer* timer;

    QString stylesheetSliderGreen =
            "QSlider::groove:vertical {background: transparent;}QSlider::handle:vertical {height: 20px;background: green;margin: 0 -22px; /* expand outside the groove */ }QSlider::add-page:vertical {background-color: transparent; }QSlider::sub-page:vertical {background: transparent;}";
    QString stylesheetSliderYellow =
            "QSlider::groove:vertical {background: transparent;}QSlider::handle:vertical {height: 20px;background: yellow;margin: 0 -22px; /* expand outside the groove */ }QSlider::add-page:vertical {background-color: transparent; }QSlider::sub-page:vertical {background: transparent;}";
    QString stylesheetSliderRed =
            "QSlider::groove:vertical {background: transparent;}QSlider::handle:vertical {height: 20px;background: red;margin: 0 -22px; /* expand outside the groove */ }QSlider::add-page:vertical {background-color: transparent; }QSlider::sub-page:vertical {background: transparent;}";
    QString stylesheetBarRed =
            "QProgressBar {border: 0px solid white;border-radius: 4px;background-color: rgba(0, 0, 0,0);}QProgressBar::chunk {background-color: qlineargradient(spread:pad, x1:0.876552, y1:0.432, x2:1, y2:1, stop:0 rgba(255, 0, 0, 255), stop:0.671642 rgba(200, 100, 50, 255)); }" ;
    QString stylesheetBarYellow =
            "QProgressBar {border: 0px solid white;border-radius: 4px;background-color: rgba(0, 0, 0,0);}QProgressBar::chunk {background-color: qlineargradient(spread:pad, x1:0.876552, y1:0.432, x2:1, y2:1, stop:0 rgba(255, 253, 0, 255), stop:0.671642 rgba(87, 210, 0, 255)); }" ;
    QString stylesheetBarGreen =
            "QProgressBar {border: 0px solid white;border-radius: 4px;background-color: rgba(0, 0, 0,0);}QProgressBar::chunk { background-color: qlineargradient(spread:pad, x1:0.777, y1:0.352636, x2:0.79602, y2:0.847, stop:0.0547264 rgba(57, 113, 4, 255), stop:0.681592 rgba(124, 196, 17, 255));}";


};
#endif // MAINWINDOW_H
