#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <unistd.h>

extern volatile int bytesCount[16];

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qDebug()<<"hello!!!";
    midip = new midiprocess;
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()),this, SLOT(timerSlot()));
    timer->start(1000);
}

MainWindow::~MainWindow()
{
    midip->~midiprocess();
    delete ui;
}

void MainWindow::timerSlot(void)
{
    static bool b=0;
    bcm2835_gpio_write(21, b);
    b = !b;

    ui->verticalSlider->setSliderPosition(abs(qrand()%20-ui->verticalSlider->value()));
}


