#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <unistd.h>

extern volatile int bytesCount[16];
#define RGB565(r, g, b)         (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

extern QString uartTXbuff;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    tftUI = new infoDisplay;
    tftUI->activateWindow();
    ui->setupUi(this);
    this->activateWindow();
    gpio = new Gpio;
    midip= new midiprocess;
    timer = new QTimer(this);
    dispThread = new DisplayThread;
    connect(timer, SIGNAL(timeout()),this, SLOT(updateInterface()));
    timer->start(50);
    midip->start();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete *midip->midiin;
    delete *midip->midiout;
}

void MainWindow::updateInterface(void)
{
    dispThread->wait();

    static int updCount = -1;
    static bool b=0;
    bcm2835_gpio_write(21, b);
    b = !b;

    int btn = gpio->shiftRead();
    if(btn) qDebug()<<"button:"<< btn;
    switch (btn)
    {
    case 0: break;
    case 1:gpio->shiftOut(4,gpio->toggle);
        uartTXbuff.append(QChar(0x0B));
        uartTXbuff.append(QChar(144));
        uartTXbuff.append(QChar(8));
        uartTXbuff.append(QChar(127));
        break;
    case 2:gpio->shiftOut(5,gpio->toggle);

        uartTXbuff.append(QChar(0x0B));
        uartTXbuff.append(QChar(144));
        uartTXbuff.append(QChar(16));
        uartTXbuff.append(QChar(127));
        break;
    case 3:gpio->shiftOut(2,gpio->toggle);
        uartTXbuff.append(QChar(0x0B));
        uartTXbuff.append(QChar(144));
        uartTXbuff.append(QChar(24));
        uartTXbuff.append(QChar(127));
        break;
    case 4:gpio->shiftOut(1,gpio->toggle);
        uartTXbuff.append(QChar(0x0B));
        uartTXbuff.append(QChar(144));
        uartTXbuff.append(QChar(16));
        uartTXbuff.append(QChar(0));
        break;
    default: break;
    }

    static bool bufferSwitch = 0;
    bufferSwitch =! bufferSwitch;
    if (bufferSwitch)
    {
        dispThread->frameBufferNext = &dispThread->frameBuffer1;
        dispThread->frameBufferPrev = &dispThread->frameBuffer2;
    }
    else
    {
        dispThread->frameBufferNext = &dispThread->frameBuffer2;
        dispThread->frameBufferPrev = &dispThread->frameBuffer1;
    }

    dispThread->updateRectPrev = dispThread->updateRectNext;

    dispThread->start();

    static int vu = -20;
    static bool dir = 1;
    static int clipFlag=0;
    if(dir)vu++;
    else vu--;
    if(vu>=0) dir = 0;
    if (vu<-55) dir =1;

    tftUI->setVU(vu);
    dispThread->updateRectNext = tftUI->getVuBarGeom(); // default
    if (updCount > (int)(1500/this->timer->interval()))// update digits
    {
        updCount = 0;
        this->tftUI->maxVu = vu;
        dispThread->updateRectNext =  tftUI->getDbDigitGeom();
    }
    if (vu > this->tftUI->maxVu)  dispThread->updateRectNext =  tftUI->getDbDigitGeom();

    if (vu>-6 && !clipFlag) clipFlag=1;
    if(clipFlag==2 && vu<-6) {
        dispThread->updateRectNext = tftUI->getClipGeom();
        clipFlag = 0;
    }
    if(clipFlag==1) {
        dispThread->updateRectNext = tftUI->getClipGeom();
        clipFlag = 2;
    }

    if (updCount==-1)
        dispThread->updateRectNext.setRect(0, 0, 240, 240);
    updCount++;

    QImage img = tftUI->grab(dispThread->updateRectNext).toImage();
    int w = dispThread->updateRectNext.width();
    int h = dispThread->updateRectNext.height();
    for (int y = 0; y< h; y++)
    {
        for (int x = 0; x< w; x++)
        {
            (*dispThread->frameBufferNext)[1][x+(w*y)] = (uint16_t)
                    RGB565( qRed(img.pixel(x,y)),
                            qGreen(img.pixel(x,y)),
                            qBlue(img.pixel(x,y)));
        }
    }


    this->midip->test();

    int adc = gpio->readADC(4);
    qDebug()<<"ADC Value:"<<adc;


    if (updCount%10 == 0){

    double vurnd = vu+(qrand()%10)/9.9;
    ui->dbValBotom_2->setText(QString::number(vurnd,' ',1));
    ui->dbValTop_2->setText(QString::number(vurnd,' ',1));
    }
    ui->progressBar->setValue(vu);

    if(vu>-2) ui->progressBar->setStyleSheet(stylesheetBarRed);
    if(vu>-12 && vu<-2) ui->progressBar->setStyleSheet(stylesheetBarYellow);
    else  ui->progressBar->setStyleSheet(stylesheetBarGreen);

    ui->verticalSlider->setSliderPosition(tftUI->maxVu);
    if(tftUI->maxVu<-12) ui->verticalSlider->setStyleSheet(stylesheetSliderGreen);
    if(tftUI->maxVu>-2) ui->verticalSlider->setStyleSheet(stylesheetSliderRed);
    if(tftUI->maxVu<-2 && tftUI->maxVu>-12) ui->verticalSlider->setStyleSheet(stylesheetSliderYellow);
}


