#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <unistd.h>
#include <chrono>

#define RGB565(r, g, b)         (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->activateWindow();
    gpio = new Gpio;
    QThread* threadGpio = new QThread;
    gpio->moveToThread(threadGpio);
    threadGpio->start(QThread::HighPriority);

    midip= new midiprocess;

    QThread* threadMidi = new QThread;
    midip->moveToThread(threadMidi);
    threadMidi->start();
    threadMidi->setPriority(QThread::TimeCriticalPriority);

    dispThread = new DisplayThread(gpio);
    timer = new QTimer(this);
    timer->setSingleShot(1);

    fillObjectVectors();

    for (int i=0; i<8; i++)
    {
        channelNumbers[i]->setText(QString::number(((midip->getDeviceOrderNumber())*8)+i+1));

        channelNames[i]->clear();
        muteButtons[i]->setVisible(0);
        soloButtons[i]->setVisible(0);
        recButtons[i]->setVisible(0);

        bars[i]->setValue(-60);
        sliders[i]->setValue(-60);
        labelBottom[i]->clear();
        labelTop[i]->clear();
        tftUI[i] = new infoDisplay;
        tftUI[i]->setChannelNumber(QString::number(((midip->getDeviceOrderNumber())*8)+i+1));
        imgPrev[i] = tftUI[i]->grab(QRect(0,0,240,240)).toImage();

        updateVu(i, 0);
    }

    connect (gpio, SIGNAL(faderMoved(int,int)), midip,SLOT(onfaderMoved(int,int)),Qt::QueuedConnection);
    connect (gpio, SIGNAL(recArmPressed(int)), midip, SLOT(onrecArmPressed(int)),Qt::QueuedConnection);
    connect (gpio, SIGNAL(soloPressed(int)),midip, SLOT(onsoloPressed(int)),Qt::QueuedConnection);
    connect (gpio, SIGNAL(mutePressed(int)), midip, SLOT(onmutePressed(int)),Qt::QueuedConnection);
    connect (gpio, SIGNAL(selectPressed(int)), midip, SLOT(onselectPressed(int)),Qt::QueuedConnection);

    connect (midip, SIGNAL(faderUpdated(int,int)), gpio, SLOT(setFader(int,int)),Qt::QueuedConnection);
    connect (midip, SIGNAL(RecArmed(int,bool)), gpio, SLOT(onRecArmed(int,bool)),Qt::QueuedConnection);
    connect (midip, SIGNAL(Soloed(int,bool)), gpio, SLOT(onSoloed(int,bool)),Qt::QueuedConnection);
    connect (midip, SIGNAL(Muted(int,bool)), gpio, SLOT(onMuted(int,bool)),Qt::QueuedConnection);
    connect (midip, SIGNAL(Selected(int,bool)), gpio, SLOT(onSelected(int,bool)),Qt::QueuedConnection);

    connect (midip, SIGNAL(RecArmed(int,bool)), this, SLOT(onRecArmed(int,bool)),Qt::QueuedConnection);
    connect (midip, SIGNAL(Soloed(int,bool)), this, SLOT(onSoloed(int,bool)),Qt::QueuedConnection);
    connect (midip, SIGNAL(Muted(int,bool)), this, SLOT(onMuted(int,bool)),Qt::QueuedConnection);
    connect (midip, SIGNAL(Selected(int,bool)), this, SLOT(onSelected(int,bool)),Qt::QueuedConnection);

    connect (midip, SIGNAL(VUupdated(int,int)), this, SLOT(updateVu(int,int)),Qt::QueuedConnection);
    connect (midip, SIGNAL(nameUpdated(int,QString)), this, SLOT(updateChannelName(int,QString)),Qt::QueuedConnection);
    connect  (midip, SIGNAL(deviceNumChanged()), this, SLOT(onDeviceNChanged()),Qt::QueuedConnection);
    connect  (midip, SIGNAL(statusUpdated(QString)), this, SLOT(updateStatusString(QString)),Qt::QueuedConnection);

    connect(dispThread, SIGNAL(ready()),this, SLOT(updateInterface()));

    updateInterface();

    connect (timer, SIGNAL(timeout()), this, SLOT(refreshVUbars()));
    timer->start(100);

    ui->statuslabel->setText(midip->connectionStatus);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fillObjectVectors()
{
    recButtons.push_back(ui->rec);
    recButtons.push_back(ui->rec_2);
    recButtons.push_back(ui->rec_3);
    recButtons.push_back(ui->rec_4);
    recButtons.push_back(ui->rec_5);
    recButtons.push_back(ui->rec_6);
    recButtons.push_back(ui->rec_7);
    recButtons.push_back(ui->rec_8);

    soloButtons.push_back(ui->solo);
    soloButtons.push_back(ui->solo_2);
    soloButtons.push_back(ui->solo_3);
    soloButtons.push_back(ui->solo_4);
    soloButtons.push_back(ui->solo_5);
    soloButtons.push_back(ui->solo_6);
    soloButtons.push_back(ui->solo_7);
    soloButtons.push_back(ui->solo_8);

    muteButtons.push_back(ui->mute);
    muteButtons.push_back(ui->mute_2);
    muteButtons.push_back(ui->mute_3);
    muteButtons.push_back(ui->mute_4);
    muteButtons.push_back(ui->mute_5);
    muteButtons.push_back(ui->mute_6);
    muteButtons.push_back(ui->mute_7);
    muteButtons.push_back(ui->mute_8);

    channelNames.push_back(ui->chname1);
    channelNames.push_back(ui->chname1_2);
    channelNames.push_back(ui->chname1_3);
    channelNames.push_back(ui->chname1_4);
    channelNames.push_back(ui->chname1_5);
    channelNames.push_back(ui->chname1_6);
    channelNames.push_back(ui->chname1_7);
    channelNames.push_back(ui->chname1_8);

    channelNumbers.push_back(ui->chnNumber);
    channelNumbers.push_back(ui->chnNumber_2);
    channelNumbers.push_back(ui->chnNumber_3);
    channelNumbers.push_back(ui->chnNumber_4);
    channelNumbers.push_back(ui->chnNumber_5);
    channelNumbers.push_back(ui->chnNumber_6);
    channelNumbers.push_back(ui->chnNumber_7);
    channelNumbers.push_back(ui->chnNumber_8);

    bars.push_back(ui->bar1);
    bars.push_back(ui->bar2);
    bars.push_back(ui->bar3);
    bars.push_back(ui->bar4);
    bars.push_back(ui->bar5);
    bars.push_back(ui->bar6);
    bars.push_back(ui->bar7);
    bars.push_back(ui->bar8);

    sliders.push_back(ui->slider1);
    sliders.push_back(ui->slider2);
    sliders.push_back(ui->slider3);
    sliders.push_back(ui->slider4);
    sliders.push_back(ui->slider5);
    sliders.push_back(ui->slider6);
    sliders.push_back(ui->slider7);
    sliders.push_back(ui->slider8);

    labelTop.push_back(ui->dbValTop);
    labelTop.push_back(ui->dbValTop_2);
    labelTop.push_back(ui->dbValTop_3);
    labelTop.push_back(ui->dbValTop_4);
    labelTop.push_back(ui->dbValTop_5);
    labelTop.push_back(ui->dbValTop_6);
    labelTop.push_back(ui->dbValTop_7);
    labelTop.push_back(ui->dbValTop_8);

    labelBottom.push_back(ui->dbValBotom);
    labelBottom.push_back(ui->dbValBotom_2);
    labelBottom.push_back(ui->dbValBotom_3);
    labelBottom.push_back(ui->dbValBotom_4);
    labelBottom.push_back(ui->dbValBotom_5);
    labelBottom.push_back(ui->dbValBotom_6);
    labelBottom.push_back(ui->dbValBotom_7);
    labelBottom.push_back(ui->dbValBotom_8);

}

void MainWindow::updateInterface(void)
{ 
    for (int chan = 0; chan<8; chan++)
    {
        processPixels(chan);
    }
    dispThread->start();
}


void MainWindow::processPixels(int display)
{    
    QImage img = tftUI[display]->grab(QRect(0,0,240,240)).toImage();

    int w = img.width();
    int h = img.height();
    QRgb pixel;
    DisplayThread::pixel pixelStruct;

    static bool first = 1;

    for (int y = 0; y< h; y++)
    {
        for (int x = 0; x< w; x++)
        {
            pixel = img.pixel(x,y);
            if ( pixel != imgPrev[display].pixel(x,y) || first)
            {
                pixelStruct.x =x;
                pixelStruct.y =y;
                pixelStruct.color =  RGB565(qRed(pixel),qGreen(pixel),qBlue(pixel));
                pixelStruct.display = display;
                dispThread->pixels.push_back(pixelStruct);
            }
        }
    }
    imgPrev[display] = img.copy();
    first = 0;
}

void MainWindow::updateVu(int chan, int step)
{
    static int maxVU[8] = {-60};
    static int target[8] = {-60};
    const int dbsteps [14] = {-60,-60,-55,-50,-45,-40,-35,-30,-25,-20,-15,-10,-5,0};
    static int inc[8] = {0}; // increment

    if (chan <0  || chan>7 || step>13) {return;}// skip master channel
    // if step == -1 keep updating to achieve target value
    if (step!=-1){
        target[chan] = dbsteps[step];
        inc[chan] = qRound(((float)bars[chan]->value()-target[chan])/5.00);
    }

    int db=0;
    if (bars[chan]->value() > target[chan])
    {
        db = bars[chan]->value() - abs(inc[chan]);
    }
    else{db = target[chan];}

    //qDebug()<< "step"<< step << "target" << target[chan] <<"db"<<db;

    tftUI[chan]->setVU(db);
    bars[chan]->setValue(db);

    if (db>-2 && (bars[chan]->styleSheet() != stylesheets.BarRed)){
    bars[chan]->setStyleSheet(stylesheets.BarRed);
    }
    if ((db<=-2 && db>=-12) && (bars[chan]->styleSheet() != stylesheets.BarYellow)){
    bars[chan]->setStyleSheet(stylesheets.BarYellow);
    }
    if(db<-12 && (bars[chan]->styleSheet() != stylesheets.BarGreen) && !muteButtons[chan]->isVisible()){
    bars[chan]->setStyleSheet(stylesheets.BarGreen);
    }
    if(muteButtons[chan]->isVisible() && (bars[chan]->styleSheet() != stylesheets.BarGray)){
    bars[chan]->setStyleSheet(stylesheets.BarGray);
    }

    static std::chrono::milliseconds lastSliderUpdate[8] = {std::chrono::milliseconds(0)};
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    auto from_lastSliderUpdate = millis - lastSliderUpdate[chan];

    if (from_lastSliderUpdate > std::chrono::milliseconds(2300)  || target[chan]>maxVU[chan])
    {
        lastSliderUpdate[chan] = millis;
        maxVU[chan] = db;
        sliders[chan]->setValue(db);
        if (db>-2){sliders[chan]->setStyleSheet(stylesheets.SliderRed);}
        if (db<=-2 && db>=-12){sliders[chan]->setStyleSheet(stylesheets.SliderYellow);}
        if(db<-12)  {  sliders[chan]->setStyleSheet(stylesheets.SliderGreen);}
        tftUI[chan]->maxVu = target[chan];
        float vuD =0;
        if (bars[chan]->value()!=target[chan])
            vuD= db + rndDecimal(abs(chan * step));
        else vuD= db;

        labelTop[chan]->setText(QString::number(vuD,' ',1));
        labelBottom[chan]->setText(QString::number(vuD,' ',1));
    }

}

void MainWindow::updateChannelName(int channel, QString name)
{
    channelNames[channel]->setText(name);
    tftUI[channel]->setChannelName(name);

}

void MainWindow::updLabelColor (int channel)
{
    if ( recButtons[channel]->isVisible()) {
        channelNames[channel]->setStyleSheet("color: red");
        labelTop[channel]->setStyleSheet("color: red");
    }
    else
        if (soloButtons[channel]->isVisible()){
            channelNames[channel]->setStyleSheet("color: green");
            labelTop[channel]->setStyleSheet("color: green");
        }
        else
            if( muteButtons[channel]->isVisible()) {
                channelNames[channel]->setStyleSheet("color: gray");
                labelTop[channel]->setStyleSheet("color: gray");
            }
            else
            {
                channelNames[channel]->setStyleSheet("color: white");
                labelTop[channel]->setStyleSheet("color: white");
            }
}
void MainWindow::onRecArmed(int channel, bool state)
{
    recButtons[channel]->setVisible(state);
    updLabelColor(channel);
}

void MainWindow::onSoloed(int channel, bool state)
{
    soloButtons[channel]->setVisible(state);
    updLabelColor(channel);
}

void MainWindow::onMuted(int channel, bool state)
{
    muteButtons[channel]->setVisible(state);
    updLabelColor(channel);
}

void MainWindow::onSelected(int channel, bool state)
{
    if(state)
        channelNumbers[channel]->setStyleSheet("background-color: gray");
    else
        channelNumbers[channel]->setStyleSheet("color: white");
}

void MainWindow::refreshVUbars()
{

    for(int chan=0; chan<8; chan++)
        updateVu(chan,-1);

    timer->start(24);
}

void MainWindow::onDeviceNChanged()
{
    for(int i=0; i<8; i++) {
        channelNumbers[i]->setText(QString::number(((midip->getDeviceOrderNumber())*8)+i+1));
        tftUI[i]->setChannelNumber(QString::number(((midip->getDeviceOrderNumber())*8)+i+1));
    }
}

void MainWindow::updateStatusString(QString s)
{
    ui->statuslabel->setText(s);
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




