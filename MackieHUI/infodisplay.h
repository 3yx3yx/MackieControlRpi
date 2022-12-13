#ifndef INFODISPLAY_H
#define INFODISPLAY_H

#include <QWidget>

namespace Ui {
class infoDisplay;
}

class infoDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit infoDisplay(QWidget *parent = nullptr);
    ~infoDisplay();

    QRect getVuBarGeom(void);
    QRect getClipGeom(void);
    QRect getDbDigitGeom(void);
    QRect getChannelLabelGeom(void);

    int maxVu = 0;
    void setVU (int db);


private:
    Ui::infoDisplay *ui;
};

#endif // INFODISPLAY_H
