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


private slots:
    void updateInterface(void);
private:
    Ui::MainWindow *ui;
    QTimer* timer;
    midiprocess* midip;
    Gpio* gpio;
    DisplayThread* dispThread;
    infoDisplay* tftUI[8];

    QElapsedTimer eTimDigits;
    QElapsedTimer eTimSlider;

    void processPixels(int display);


    float rndDecimal (int seed);

    struct {
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

    QPushButton* recButtons[8] = {ui->rec,ui->rec_2,ui->rec_3,ui->rec_4,
                                 ui->rec_5,ui->rec_6,ui->rec_7,ui->rec_8};
    QPushButton* soloButtons[8] = {ui->solo, ui->solo_2, ui->solo_3, ui->solo_4,
                                   ui->solo_5, ui->solo_6, ui->solo_7, ui->solo_8};
    QToolButton* muteButtons[8] = {ui->mute, ui->mute_2,ui->mute_3,ui->mute_4,
                                  ui->mute_5,ui->mute_6,ui->mute_7,ui->mute_8};
    QLabel* channelNames[8] = {ui->chname1,ui->chname1_2,ui->chname1_3,ui->chname1_4,
                              ui->chname1_5,ui->chname1_6,ui->chname1_7,ui->chname1_8};

};
#endif // MAINWINDOW_H
