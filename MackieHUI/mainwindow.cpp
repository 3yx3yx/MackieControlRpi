#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <unistd.h>

#define RGB565(r, g, b)         (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->activateWindow();
    gpio = new Gpio;
    midip= new midiprocess;
    dispThread = new DisplayThread(gpio);

    for (int i=0; i<8; i++)
    {
        QLabel* channelNumbers[8] = {ui->chnNumber,ui->chnNumber_2,ui->chnNumber_3,ui->chnNumber_4,
                                     ui->chnNumber_5,ui->chnNumber_6,ui->chnNumber_7,ui->chnNumber_8,};
        channelNumbers[i]->setText(QString::number(((midip->getDeviceOrderNumber()+1)*8)+i));

        channelNames[i]->clear();
        muteButtons[i]->setVisible(0);
        soloButtons[i]->setVisible(0);
        recButtons[i]->setVisible(0);

        tftUI[i] = new infoDisplay;
        tftUI[i]->activateWindow();
    }

    connect (gpio, SIGNAL(faderMoved(int,int)), midip,SLOT(onfaderMoved(int,int)));
    connect (gpio, SIGNAL(recArmPressed(int)), midip, SLOT(onrecArmPressed(int)));
    connect (gpio, SIGNAL(soloPressed(int)),midip, SLOT(onsoloPressed(int)));
    connect (gpio, SIGNAL(mutePressed(int)), midip, SLOT(onmutePressed(int)));
    connect (gpio, SIGNAL(selectPressed(int)), midip, SLOT(onselectPressed(int)));

    connect (midip, SIGNAL(faderUpdated(int,int)), gpio, SLOT(setFader(int,int)));
    connect (midip, SIGNAL(RecArmed(int,bool)), gpio, SLOT(onRecArmed(int,bool)));
    connect (midip, SIGNAL(Soloed(int,bool)), gpio, SLOT(onSoloed(int,bool)));
    connect (midip, SIGNAL(Muted(int,bool)), gpio, SLOT(onMuted(int,bool)));
    connect (midip, SIGNAL(Selected(int,bool)), gpio, SLOT(onSelected(int,bool)));

    connect (midip, SIGNAL(RecArmed(int,bool)), this, SLOT(onRecArmed(int,bool)));
    connect (midip, SIGNAL(Soloed(int,bool)), this, SLOT(onSoloed(int,bool)));
    connect (midip, SIGNAL(Muted(int,bool)), this, SLOT(onMuted(int,bool)));

    connect (midip, SIGNAL(VUupdated(int,int)), this, SLOT(updateVu(int,int)));
    connect (midip, SIGNAL(nameUpdated(int,QString)), this, SLOT(updateChannelName(int,QString)));


    QThread* threadMidi = new QThread;
    midip->moveToThread(threadMidi);
    threadMidi->start();

    QThread* threadGpio = new QThread;
    midip->moveToThread(threadGpio);
    threadGpio->start();

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()),this, SLOT(updateInterface()));
    timer->setSingleShot(1);
    timer->start(50);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateInterface(void)
{ 
    dispThread->wait();
    for (int chan = 0; chan<8; chan++)
    {
        processPixels(chan);
    }
    dispThread->start();

    timer->start(30);
}


void MainWindow::processPixels(int display)
{
    QImage img = tftUI[display]->grab(QRect(0,0,240,240)).toImage();
    static  QImage imgPrev[8];
    static bool first =1;
    int w = img.width();
    int h = img.height();
    QRgb pixel;
    DisplayThread::pixel pixelStruct;
    for (int y = 0; y< h; y++)
    {
        for (int x = 0; x< w; x++)
        {
            pixel = img.pixel(x,y);
            if (pixel != imgPrev[display].pixel(x,y) || first)
            {
                pixelStruct.x =x;
                pixelStruct.y =y;
                pixelStruct.color =  RGB565(qRed(pixel),qGreen(pixel),qBlue(pixel));
                pixelStruct.display = display;
                dispThread->pixels.push_back(pixelStruct);
            }
        }
    }
    imgPrev[display] = img;
    first = 0;
}

void MainWindow::updateVu(int chan, int step)
{
    static int maxVU[8] = {0};
    static int target[8];
    const int smoothing = 4; // smoothing steps
    static QProgressBar* bars[8] = {ui->bar1,ui->bar2,ui->bar3,ui->bar4,
                                    ui->bar5,ui->bar6,ui->bar7,ui->bar8};
    static QSlider* sliders[8] = {ui->slider1,ui->slider2,ui->slider3,ui->slider4,
                                  ui->slider5,ui->slider6,ui->slider7,ui->slider8};
    static QLabel* labelTop[8] = {ui->dbValTop,ui->dbValTop_2,ui->dbValTop_3,ui->dbValTop_4,
                                  ui->dbValTop_5,ui->dbValTop_6,ui->dbValTop_7,ui->dbValTop_8};
    static QLabel* labelBottom[8] = {ui->dbValBotom, ui->dbValBotom_2,ui->dbValBotom_3,ui->dbValBotom_4,
                                     ui->dbValBotom_5,ui->dbValBotom_6,ui->dbValBotom_7,ui->dbValBotom_8};
    const int dbsteps [13] = {-60,-60,-50,-40,-30,-20,-14,-10,-8,-6,-4,-2,0};


    if (step<13)
    {
        target[chan] = dbsteps[step];
    }

    static int inc[8] = {0} ; // increment
    if (inc[chan] == 0)
    {
        inc[chan] = (target[chan]-bars[chan]->value()) / smoothing ;
    } // transit from current value to target value with n=smoothing steps

    int db = bars[chan]->value() + inc[chan];

    if ((inc[chan] > 0 && db >= target[chan])
            ||(inc[chan] < 0 && db <= target[chan]))// if target value achieved reset the increment
    {
        inc[chan] =0;
    }
    tftUI[chan]->setVU(db);
    bars[chan]->setValue(db);


    if (eTimSlider.elapsed()>1500 || db>maxVU[chan])
    {
        eTimSlider.start();
        maxVU[chan] = db;
        sliders[chan]->setValue(db);

        if (db>-2)
        {
            sliders[chan]->setStyleSheet(stylesheets.SliderRed);
        }
        else
            if (db<-2 && db>-12)
            {
                sliders[chan]->setStyleSheet(stylesheets.SliderYellow);
            }
            else
            {
                sliders[chan]->setStyleSheet(stylesheets.SliderGreen);
            }

        tftUI[chan]->maxVu = target[chan];

    }

    static int dbPrev = 0;

    if(muteButtons[chan]->isVisible())
    {
        bars[chan]->setStyleSheet("QProgressBar::chunk {background-color: gray;}");
    }
    else
        if (dbPrev<-2 && db>=-2)
        {
            bars[chan]->setStyleSheet(stylesheets.BarRed);
        }
        else
            if ((dbPrev>=-2 || dbPrev<-12) && (db<-2 && db>=-12))
            {
                bars[chan]->setStyleSheet(stylesheets.BarYellow);
            }
            else
                if(dbPrev>=-12 && db<-12)
                {
                    bars[chan]->setStyleSheet(stylesheets.BarGreen);
                }

    dbPrev = db;


    if (eTimDigits.elapsed()>500)
    {
        double vuD;
        if (inc[chan] != 0)
            vuD= db + rndDecimal(chan * step);
        else
            vuD = db;
        labelTop[chan]->setText(QString::number(vuD,' ',1));
        labelBottom[chan]->setText(QString::number(vuD,' ',1));

        eTimDigits.start();
    }
}

void MainWindow::updateChannelName(int channel, QString name)
{
    channelNames[channel]->setText(name);
}

void MainWindow::onRecArmed(int channel, bool state)
{
    recButtons[channel]->setVisible(state);
    if (state)
        channelNames[channel]->setStyleSheet("color: red");
    else
        channelNames[channel]->setStyleSheet("color: white");
}

void MainWindow::onSoloed(int channel, bool state)
{
    soloButtons[channel]->setVisible(state);
    if (state)
        channelNames[channel]->setStyleSheet("color: green");
    else
        channelNames[channel]->setStyleSheet("color: white");
}

void MainWindow::onMuted(int channel, bool state)
{
    muteButtons[channel]->setVisible(state);
    if (state)
        channelNames[channel]->setStyleSheet("color: gray");
    else
        channelNames[channel]->setStyleSheet("color: white");
}


float MainWindow::rndDecimal (int seed) // :-D
{
    static int cash [10] = {5,3,1,8,4,6,3,1,2,9};
    static int i = 0;
    int idxRet = seed % 10;

    while (seed>10)
    {
        seed >>= 1;
    }

    cash[i] = seed;
    i++;
    if (i>9) i=0;

    return (float)cash[idxRet]/10.0;
}


