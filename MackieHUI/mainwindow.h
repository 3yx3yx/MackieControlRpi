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

#include "ui_mainwindow.h"

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

public slots:
    void updateVu(int channel, int step);
    //void updateChannelStatus(int channel, int status, bool state);
    void updateChannelName(int channel, QString name);
    void onRecArmed (int channel, bool state);
    void onSoloed (int channel, bool state);
    void onMuted (int channel, bool state);
    void onSelected (int channel, bool state);
    void refreshVUbars();
    void onDeviceNChanged();
    void updateStatusString(QString s);

private slots:
    void updateInterface(void);
private:
    Ui::MainWindow *ui;
    QTimer* timer;
    midiprocess* midip;
    Gpio* gpio;
    DisplayThread* dispThread;
    infoDisplay* tftUI[8];

    QImage imgPrev[8];

    void processPixels(int display);
    float rndDecimal (int seed);
    void updLabelColor (int channel);

    struct {
        const QString BarGray = "QProgressBar::chunk {background-color: gray;}"
                                "QProgressBar {background-color: rgba(0, 0, 0,0);}";
        const QString SliderGreen =
                "QSlider::groove:vertical {background: transparent;}QSlider::handle:vertical {height: 20px;background: green;margin: 0 -22px; /* expand outside the groove */ }QSlider::add-page:vertical {background-color: transparent; }QSlider::sub-page:vertical {background: transparent;}";
        const QString SliderYellow =
                "QSlider::groove:vertical {background: transparent;}QSlider::handle:vertical {height: 20px;background: yellow;margin: 0 -22px; /* expand outside the groove */ }QSlider::add-page:vertical {background-color: transparent; }QSlider::sub-page:vertical {background: transparent;}";
        const QString SliderRed =
                "QSlider::groove:vertical {background: transparent;}QSlider::handle:vertical {height: 20px;background: red;margin: 0 -22px; /* expand outside the groove */ }QSlider::add-page:vertical {background-color: transparent; }QSlider::sub-page:vertical {background: transparent;}";
        const QString BarRed =
                "QProgressBar {border: 0px solid white;border-radius: 4px;background-color: rgba(0, 0, 0,0);}QProgressBar::chunk {background-color: qlineargradient(spread:pad, x1:0.876552, y1:0.432, x2:1, y2:1, stop:0 rgba(255, 0, 0, 255), stop:0.671642 rgba(200, 100, 50, 255)); }" ;
        const QString BarYellow =
                "QProgressBar {border: 0px solid white;border-radius: 4px;background-color: rgba(0, 0, 0,0);}QProgressBar::chunk {background-color: qlineargradient(spread:pad, x1:0.876552, y1:0.432, x2:1, y2:1, stop:0 rgba(255, 253, 0, 255), stop:0.671642 rgba(87, 210, 0, 255)); }" ;
        const QString BarGreen =
                "QProgressBar {border: 0px solid white;border-radius: 4px;background-color: rgba(0, 0, 0,0);}QProgressBar::chunk { background-color: qlineargradient(spread:pad, x1:0.777, y1:0.352636, x2:0.79602, y2:0.847, stop:0.0547264 rgba(57, 113, 4, 255), stop:0.681592 rgba(124, 196, 17, 255));}";
    }stylesheets;

    QVector <QPushButton*> recButtons;
    QVector <QPushButton*> soloButtons;
    QVector <QToolButton*> muteButtons;
    QVector <QLabel*> channelNames;
    QVector <QLabel*> channelNumbers;
    QVector <QProgressBar*> bars;
    QVector <QSlider*> sliders;
    QVector <QLabel*> labelTop;
    QVector <QLabel*> labelBottom;

    void fillObjectVectors ();

};
#endif // MAINWINDOW_H
