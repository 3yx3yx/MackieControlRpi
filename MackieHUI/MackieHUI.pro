QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DEFINES += __LINUX_ALSA__
DEFINES += AVOID_TIMESTAMPING
#DEFINES += TEST

CONFIG += c++17

LIBS += -L/home/ilia/rpi-qt/sysroot/usr/include/alsa/asoundlib.h -lasound\
-L/home/ilia/rpi-qt/sysroot/usr/include/pthread.h -lpthread

QMAKE_CXXFLAGS+= -g

QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O4

QMAKE_LFLAGS_RELEASE -= -O4

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    DisplayThread.cpp \
    RtMidi.cpp \
    bcm2835.c \
    infodisplay.cpp \
    main.cpp \
    mainwindow.cpp \
    midiprocess.cpp \
    mygpio.cpp \
    st7789.cpp

HEADERS += \
    DisplayThread.h \
    RtMidi.h \
    bcm2835.h \
    infodisplay.h \
    mainwindow.h \
    midiprocess.h \
    mygpio.h \
    st7789.h

FORMS += \
    infodisplay.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /home/pi/$${TARGET}
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    images.qrc
