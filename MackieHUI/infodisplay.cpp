#include "infodisplay.h"
#include "ui_infodisplay.h"

infoDisplay::infoDisplay(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::infoDisplay)
{
    ui->setupUi(this);

    ui->L_Clip->setVisible(0);
}

QRect infoDisplay::getDbDigitGeom(void)
{
    return ui->lcdNumber->geometry();
}

QRect infoDisplay::getChannelLabelGeom()
{
    return ui->L_Name->geometry();
}

QRect infoDisplay::getVuBarGeom(void)
{
    return ui->VUbar->geometry();
}

QRect infoDisplay::getClipGeom()
{
    return ui->L_Clip->geometry();
}

void infoDisplay::setVU(int db)
{
    ui->VUbar->setValue(db);

    ui->lcdNumber->display(maxVu);
    if (db>-2) ui->L_Clip->setVisible(1);
    else ui->L_Clip->setVisible(0);
}

void infoDisplay::setChannelName(QString name)
{
    ui->L_Name->setText(name.remove(' '));
}

void infoDisplay::setChannelNumber(QString s)
{
    ui->label->setText(s);
}



infoDisplay::~infoDisplay()
{
    delete ui;
}
